/**
 *  Copyright (C) 2011-2012  Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "httpd.h"
#include "netutils.h"
#include "http_request.h"
#include "compat.h"
#include "raop.h"
#include "airplay.h"
#include "utils.h"
#include "android/log.h"
//#include "lollipop_socket_ipc.h"

#define LOG_TAG "httpd"
#define DBG	0

// #define ONE_MIRROR_LIMIT 1




unsigned short g_port_seted = 0;
unsigned short g_event_port = 0;

typedef unsigned char       BYTE;
int event_socket;

const int MAX_BUF_SIZE = 4 * 1024 * 1024;
struct http_connection_s {
	// for multi client
	int set_codec_flag;
	char* p_buffer;
	char buffer[1024 * 4];  
	char headdata[128];
	void* aes_key_context;
	// end for multi client

	int connected;
	int socket_fd;
	unsigned char remote[64];
	void *user_data;
	http_request_t *request;
};
typedef struct http_connection_s http_connection_t;

struct httpd_s {
	httpd_callbacks_t callbacks;

	int max_connections;
	int open_connections;
	http_connection_t *connections;

	/* These variables only edited mutex locked */
	int m_running;
	int m_joined;
	int m_id;
	thread_handle_t thread;
	mutex_handle_t run_mutex;

	/* Server fds for accepting connections */
	int server_fd4;
	int server_fd6;
};

static void httpd_remove_connection(httpd_t *httpd, http_connection_t *connection);

static httpd_t *mHttpd[4] = {0};

httpd_t *
httpd_init(httpd_callbacks_t *callbacks, int max_connections, int id)
{
	httpd_t *httpd;

	assert(callbacks);
	assert(max_connections > 0);

	/* Allocate the httpd_t structure */
	httpd = calloc(1, sizeof(httpd_t));
	if (!httpd) {
		return NULL;
	}

	httpd->max_connections = max_connections;
	httpd->connections = calloc(max_connections, sizeof(http_connection_t));
	if (!httpd->connections) {
		free(httpd);
		return NULL;
	}

	/* Save callback pointers */
	memcpy(&httpd->callbacks, callbacks, sizeof(httpd_callbacks_t));

	ALOGE("%s (line %d)", __func__, __LINE__);
	/* Initial status joined */
	httpd->m_running = 0;
	httpd->m_joined = 1;
	httpd->m_id = id;

    httpd->server_fd4 = -1;
    httpd->server_fd6 = -1;
	if(DBG) ALOGD("cz %s:%d id=%d", __func__, __LINE__, id);

	mHttpd[id+1] = httpd;

	return httpd;
}

void
httpd_destroy(httpd_t *httpd)
{
	ALOGE("%s(%d)", __func__, __LINE__);
	mHttpd[httpd->m_id+1] = NULL;
	if (httpd) {
		httpd_stop(httpd);

		free(httpd->connections);
		free(httpd);
	}
}

void forceStopDlna(void) {
//	if(is_dlna_running) {
//		if(lollipop_socket_client_send(SOCK_FILE_DLNA1, STOP_DLNA) == 0) {
//			while(is_dlna_running) {
//				if(DBG) ALOGI("cz %s:%d airplay=%d dlna=%d", __func__, __LINE__, is_airplay_running, is_dlna_running);
//				sleep(1);
//			}
//		}
//	}
}

void forceStopAirplay(int id) {
	int i, j;
	if(id >= -1 && id <= 2) {
		for (i = 0; mHttpd[id+1] != NULL && i < mHttpd[id+1]->max_connections; i++) {
			http_connection_t *connection = &mHttpd[id+1]->connections[i];
			if (!connection->connected) {
				continue;
			}
			httpd_remove_connection(mHttpd[id+1], connection);//断开指定id端口的连接
		}
	} else if(id == -2) {
		//g_set_codec = 0;//复位镜像初始化标志
	} else {
		for (j = 0; j < 4; j++) {
			for (i = 0; mHttpd[j] != NULL && i < mHttpd[j]->max_connections; i++) {
				http_connection_t *connection = &mHttpd[j]->connections[i];
				if (!connection->connected) {
					continue;
				}
				httpd_remove_connection(mHttpd[j], connection);//断开所有Airplay设备的连接
			}
		}
	}
}

static int
httpd_add_connection(httpd_t *httpd, int fd, unsigned char *local, int local_len, unsigned char *remote, int remote_len)
{
	int i, j;

	if(httpd->m_id == -1) {
		forceStopDlna();
		for (j = 0; j < 4; j++) {
			for (i = 0; mHttpd[j] != NULL && i < mHttpd[j]->max_connections; i++) {
				http_connection_t *connection = &mHttpd[j]->connections[i];
				if (!connection->connected) {
					continue;
				}
				if(remote[remote_len-1] != connection->remote[remote_len-1]) {//比较IP最后一位
					if(DBG) ALOGI("cz(%d) remove connection fd=%d", __LINE__, connection->socket_fd);
#if 0
					httpd_remove_connection(mHttpd[j], connection); // multi device must comment 
#endif
				}
			}
		}
		//g_set_codec = 0;//复位镜像初始化标志
 
	}
	if(DBG) ALOGI("cz(%d) add connection remote: %d.%d.%d.%d m_id=%d fd=%d", __LINE__,
			remote[0], remote[1], remote[2], remote[3], httpd->m_id, fd);

	for (i = 0; i < httpd->max_connections; i++) {
		if (!httpd->connections[i].connected) {
			break;
		}
	}
	if (i == httpd->max_connections) {
		/* This code should never be reached, we do not select server_fds when full */
		if(DBG) ALOGI("Max connections reached");
		shutdown(fd, SHUT_RDWR);
		closesocket(fd);
		return -1;
	}

	httpd->connections[i].user_data = httpd->callbacks.conn_init(httpd->callbacks.opaque, local, local_len, remote, remote_len);
	if(!httpd->connections[i].user_data) {
		if(DBG) ALOGI("cz %s(%d) Error initializing HTTP request handler", __func__, __LINE__);
		return -1;
	}
	httpd->open_connections++;
	httpd->connections[i].socket_fd = fd;
	httpd->connections[i].connected = 1;
	httpd->connections[i].set_codec_flag = 0;
	httpd->connections[i].p_buffer = (char*)malloc(MAX_BUF_SIZE);
	httpd->connections[i].aes_key_context = NULL;
	memcpy(httpd->connections[i].remote, remote, remote_len);

	return 0;
}

static int
httpd_accept_connection(httpd_t *httpd, int server_fd, int is_ipv6)
{
	struct sockaddr_storage remote_saddr;
	socklen_t remote_saddrlen;
	struct sockaddr_storage local_saddr;
	socklen_t local_saddrlen;
	unsigned char *local, *remote;
	int local_len, remote_len;
	int ret, fd;

	remote_saddrlen = sizeof(remote_saddr);
	fd = accept(server_fd, (struct sockaddr *)&remote_saddr, &remote_saddrlen);
	if (fd == -1) {
		/* FIXME: Error happened */
		return -1;
	}

	local_saddrlen = sizeof(local_saddr);
	ret = getsockname(fd, (struct sockaddr *)&local_saddr, &local_saddrlen);
	if (ret == -1) {
		shutdown(fd, SHUT_RDWR);
		closesocket(fd);
		return 0;
	}
 
 

	if(DBG) ALOGI("Accepted %s client on socket %d remote=%s:%d", (is_ipv6 ? "IPv6"  : "IPv4"), fd,
			inet_ntoa(((struct sockaddr_in *)&remote_saddr)->sin_addr), ((struct sockaddr_in *)&remote_saddr)->sin_port);
	local = netutils_get_address(&local_saddr, &local_len);
	remote = netutils_get_address(&remote_saddr, &remote_len);

#if defined(ONE_MIRROR_LIMIT)  
	// 导致浏览器内视频不能全屏播放
	airplay_t* airplay = (airplay_t*)httpd->callbacks.opaque;
	if (airplay->callbacks.mirroring_running(airplay->callbacks.cls, remote) ) {
		ALOGE("client!!! ");
		shutdown(fd, SHUT_RDWR);
		closesocket(fd);
		return 0;
	}
#endif

#if 1 // 允许抢占式镜像
	airplay_t* airplay = (airplay_t*)httpd->callbacks.opaque;
	if (airplay->callbacks.mirroring_running(airplay->callbacks.cls, remote) ) {
		ALOGE("new client!!! ");
		airplay->callbacks.mirroring_stop(airplay->callbacks.cls, 0);
		forceStopAirplay(httpd->m_id);
		sleep(1); 
	}
#endif 

	

	ret = httpd_add_connection(httpd, fd, local, local_len, remote, remote_len);
	if (ret == -1) {
		shutdown(fd, SHUT_RDWR);
		closesocket(fd);
		return 0;
	}

	return 1;
}

static void
httpd_remove_connection(httpd_t *httpd, http_connection_t *connection)
{
	char *remote = connection->remote;
	if(DBG) ALOGI("cz(%d) remove connection remote: %d.%d.%d.%d m_id=%d fd=%d", __LINE__,
			remote[0], remote[1], remote[2], remote[3], httpd->m_id, connection->socket_fd);
	if (connection->request) {
		http_request_destroy(connection->request);
		connection->request = NULL;
	}
	httpd->callbacks.conn_destroy(connection->user_data);
	shutdown(connection->socket_fd, SHUT_WR);
	closesocket(connection->socket_fd);
	connection->connected = 0;
	connection->set_codec_flag = 0;
	free(connection->p_buffer);
	connection->p_buffer = NULL;
	free(connection->aes_key_context);
	connection->aes_key_context = NULL;

	httpd->open_connections--;
	if(httpd->m_id == -1) {
 
	}
}

static THREAD_RETVAL
httpd_thread(void *arg)
{
	httpd_t *httpd = (httpd_t*)arg;
	airplay_t* airplay = 0;
	//char buffer[1024 * 4]; //1024:error
	//char headdata[128];
	int i;
	//const int MAX_BUF_SIZE = 4*1024*1024; 
	//char* p_buffer = (char*)malloc(MAX_BUF_SIZE);

	//assert(p_buffer);
	assert(httpd);

	while (1) {
		fd_set rfds;
		struct timeval tv;
		int nfds = 0;
		int ret;

		MUTEX_LOCK(httpd->run_mutex);
		if (!httpd->m_running) {
			MUTEX_UNLOCK(httpd->run_mutex);
			ALOGE("%s(%d)", __func__, __LINE__);
			break;
		}
		MUTEX_UNLOCK(httpd->run_mutex);

		/* Set timeout value to 5ms */
		tv.tv_sec = 1;
		tv.tv_usec = 5000;

		/* Get the correct nfds value and set rfds */
		FD_ZERO(&rfds);
		if (httpd->open_connections < httpd->max_connections) {
			if (httpd->server_fd4 != -1) {
				FD_SET(httpd->server_fd4, &rfds);
				if (nfds <= httpd->server_fd4) {
					nfds = httpd->server_fd4+1;
				}
			}
			if (httpd->server_fd6 != -1) {
				FD_SET(httpd->server_fd6, &rfds);
				if (nfds <= httpd->server_fd6) {
					nfds = httpd->server_fd6+1;
				}
			}
		}
		for (i = 0; i < httpd->max_connections; i++) {
			int socket_fd;
			if (!httpd->connections[i].connected) {
				continue;
			}
			socket_fd = httpd->connections[i].socket_fd;
			FD_SET(socket_fd, &rfds);
			if (nfds <= socket_fd) {
				nfds = socket_fd+1;
			}
		}

		ret = select(nfds, &rfds, NULL, NULL, &tv);
		if (ret == 0) {
			/* Timeout happened */
			continue;
		} else if (ret == -1) {
			/* FIXME: Error happened */
			if(DBG) ALOGI("Error in select");
			break;
		}

		if (httpd->open_connections < httpd->max_connections &&
		    httpd->server_fd4 != -1 && FD_ISSET(httpd->server_fd4, &rfds)) {
			ret = httpd_accept_connection(httpd, httpd->server_fd4, 0);
			if (ret == -1) {
			ALOGE("%s(%d)", __func__, __LINE__);
				break;
			} else if (ret == 0) {
				continue;
			}
		}
		if (httpd->open_connections < httpd->max_connections &&
		    httpd->server_fd6 != -1 && FD_ISSET(httpd->server_fd6, &rfds)) {
			ret = httpd_accept_connection(httpd, httpd->server_fd6, 1);
			if (ret == -1) {
			ALOGE("%s(%d)", __func__, __LINE__);
				break;
			} else if (ret == 0) {
				continue;
			}
		}
		for (i = 0; i < httpd->max_connections; i++) {
			http_connection_t *connection = &httpd->connections[i];
			if (!connection->connected) {
				continue;
			}
			if (!FD_ISSET(connection->socket_fd, &rfds)) {
				continue;
			}
			/* If not in the middle of request, allocate one */
			if (!connection->request) {
				connection->request = http_request_init();
				assert(connection->request);
			}
            //...
			if (httpd->m_id != 1) { //http数据
				//接收数据
				if(DBG) ALOGD("cz Receiving on socket %d, http-id:%d", connection->socket_fd, httpd->m_id);
				//memset(buffer, 0, 1024); //调试完成后注释掉
				ret = recv(connection->socket_fd, connection->buffer, sizeof(connection->buffer), 0);
				if (ret <= 0) {
					if(DBG) ALOGI("%s(%d) Connection closed for socket %d", __func__, __LINE__, connection->socket_fd);
					httpd_remove_connection(httpd, connection);
					continue;
				}
				connection->buffer[ret] = 0;
				//if(DBG) ALOGD("get====================================\n%s\n=======================================\n", buffer);
				if(!strncmp("HTTP/1.1 200 OK", connection->buffer, strlen("HTTP/1.1 200 OK"))) {
					if(DBG) ALOGD("get====================================\n%s\n=======================================\n", connection->buffer);
					continue;
				}

				//添加数据
				/* Parse HTTP request from data read from connection */
				http_request_add_data(connection->request, connection->buffer, ret);
				if (http_request_has_error(connection->request)) {
					if(DBG) ALOGD("get====================================\n%s\n=======================================\n", connection->buffer);
					char *err = http_request_get_error_name(connection->request);
					if(DBG) ALOGI("Error in parsing: %s", err);
					if(!strncmp(err, "HPE_INVALID_METHOD", strlen("HPE_INVALID_METHOD"))) {
						http_request_destroy(connection->request);
						connection->request = NULL;
					} else {
						httpd_remove_connection(httpd, connection);
						if(connection->set_codec_flag == 1) {
							airplay = (airplay_t*)httpd->callbacks.opaque;
							airplay->callbacks.mirroring_stop(airplay->callbacks.cls, connection->remote);
						}
					}
					continue;
				}
				//完成处理，或，继续接收
				/* If request is finished, process and deallocate */
				if (http_request_is_complete(connection->request)) {
					http_response_t *response;
					int index;
					int count = http_request_get_request_count(connection->request);
					for(index = 1; index <= count; index++) {
						//if(DBG) ALOGD("%s(%d)==================================================================%d", __func__, __LINE__, index);
						response = NULL;
						httpd->callbacks.conn_request(connection->user_data, connection->request, &response, index);
						//...
						if (response) {
							int datalen, ret;
							if(http_response_get_flag(response) != 1) {
								event_socket = connection->socket_fd;
								ALOGD("%s(%d) event_socket=%d", __func__, __LINE__, event_socket);
							}
							/* Get response data and datalen */
							const char *data = http_response_get_data(response, &datalen);
							int written = 0;
							while (written < datalen) {
								ret = send(connection->socket_fd, data+written, datalen-written, 0);
								if (ret == -1) {
									/* FIXME: Error happened */
									if(DBG) ALOGI("Error in sending data");
									break;
								}
								written += ret;
							}
							//if(DBG) ALOGD("send===================================\n%s\n=======================================\n", data);
							if (http_response_get_disconnect(response)) {
								if(DBG) ALOGI("Disconnecting on software request");
								httpd_remove_connection(httpd, connection);
							}
						} else {
							if(DBG) ALOGI("Didn't get response");
						}
						http_response_destroy(response);
						//if(DBG) ALOGD("%s(%d)==================================================================%d", __func__, __LINE__, index);
					}
					http_request_destroy(connection->request);
					connection->request = NULL;
				} else {
					//if(DBG) ALOGD("Request not complete, waiting for more data...");
				}
			} else { //mirror数据
				int left = 128;
				unsigned int d_width = 0, d_height = 0, d_size = 0, d_type = 0;
				uint64_t timestamp;
				memset(connection->headdata, 0, sizeof(connection->headdata));
				do {
					ret = recv_wait(connection->socket_fd, &connection->headdata[128-left], left);
					if (ret <= 0) {
						ALOGE("%s(%d)", __func__, __LINE__);
						break;
					}
					left -= ret;
				} while (left > 0);
                //...
				if (ret <= 0) {
					if(DBG) ALOGI("%s(%d) Connection closed for socket %d", __func__, __LINE__, connection->socket_fd);
					httpd_remove_connection(httpd, connection);
					connection->set_codec_flag = 0;
					continue;
				}
				//d_width  = 0; //*(DWORD*)&headdata[16];   //headdata[16] | (headdata[17] << 8) | (headdata[18] << 16) | (headdata[19] << 24);
				//d_height = 0; //*(DWORD*)&headdata[20];   // headdata[20] | (headdata[21] << 8) | (headdata[22] << 16) | (headdata[23] << 24);
				d_size = *(int32_t*)&connection->headdata[0]; //headdata[0] | (headdata[1] << 8)  | (headdata[2] << 16)  | (headdata[3] << 24);
				d_type = *(BYTE*)&connection->headdata[4];  //& 0x3;
				//if(DBG) ALOGD("width=%d, height=%d, size=%d, type=%d", d_width, d_height, d_size, d_type);
				timestamp = bytes_to_little_long((uint8_t*)connection->headdata, 8);
				if (d_size > 0 && d_size <= MAX_BUF_SIZE) {
					//(char*)malloc(4 * ((d_size + 3) >> 2));
					unsigned int v36 = d_size;
					do {
						ret = recv_wait(connection->socket_fd, &connection->p_buffer[d_size] - v36, v36);
						if (ret < 0) {
							if(DBG) ALOGD("=================reader packet(close)================");
							break;
						}
						v36 -= ret;
					}
					while (v36 > 0);
                    //...
					if (ret <= 0) {
						if(DBG) ALOGI("%s(%d) Connection closed for socket %d", __func__, __LINE__, connection->socket_fd);
						httpd_remove_connection(httpd, connection);
						connection->set_codec_flag = 0;
						continue;
					}
					airplay = (airplay_t*)httpd->callbacks.opaque;
				 
					if (d_type == 1) {
						if (connection->set_codec_flag) {
							airplay->callbacks.mirroring_process(airplay->callbacks.cls, connection->p_buffer, d_size, d_type, timestamp, connection->remote);
						} else {
							
							if (DBG) ALOGD("  %s:%d %s === =mirroring_play==================", __func__, __LINE__, "buf_remote");
							airplay->callbacks.mirroring_play(airplay->callbacks.cls, 0, 0, connection->p_buffer, d_size+1, d_type, timestamp, connection->remote);
							connection->aes_key_context = g_aes_key_ctx; //在此之前, iphone 已经把key 送过来, 所以这里没问题
							connection->set_codec_flag = 1;
						}
					} else if (d_type == 0) {
						//if (DBG) ALOGI("socket_fd %d  d_type=%d g_set_codec=%d", connection->socket_fd, d_type, connection->set_codec_flag);
						g_aes_ctr_encrypt((unsigned char*)connection->p_buffer, d_size, connection->aes_key_context );
						airplay->callbacks.mirroring_process(airplay->callbacks.cls, connection->p_buffer, d_size, d_type, timestamp, connection->remote);
					} else if (d_type == 9) {
						airplay->callbacks.mirroring_process(airplay->callbacks.cls, connection->p_buffer, d_size, 0, timestamp, connection->remote);
					} else if (d_type == 102) {	//0x66
						airplay->callbacks.mirroring_stop(airplay->callbacks.cls, connection->remote);
					} else {
						//if(DBG) ALOGI("data type is %d, data:%s", d_type, p_buffer);
					}
				}
			}
		}
	}

	/* Remove all connections that are still connected */
	for (i = 0; i < httpd->max_connections; i++) {
		http_connection_t *connection = &httpd->connections[i];
		if (!connection->connected) {
			continue;
		}
		if(DBG) ALOGI("Removing connection for socket %d", connection->socket_fd);
		httpd_remove_connection(httpd, connection);
	}

	/* Close server sockets since they are not used any more */
	if (httpd->server_fd4 != -1) {
		shutdown(httpd->server_fd4, SHUT_RDWR);
		closesocket(httpd->server_fd4);
		httpd->server_fd4 = -1;
	}
	if (httpd->server_fd6 != -1) {
		shutdown(httpd->server_fd6, SHUT_RDWR);
		closesocket(httpd->server_fd6);
		httpd->server_fd6 = -1;
	}

	if(DBG) ALOGI("Exiting HTTP thread");
	//free(p_buffer);
	return 0;
}

int
httpd_start(httpd_t *httpd, unsigned short *port)
{
	/* How many connection attempts are kept in queue */
	int backlog = 5;

	assert(httpd);
	assert(port);

	MUTEX_LOCK(httpd->run_mutex);
	if (httpd->m_running || !httpd->m_joined) {
		MUTEX_UNLOCK(httpd->run_mutex);
		return 0;
	}

	httpd->server_fd4 = netutils_init_socket(port, 0, 0);
	if (httpd->server_fd4 == -1) {
		if(DBG) ALOGE("port %d Error initialising socket %d", *port, SOCKET_GET_ERROR());
		forceStopAirplay(httpd->m_id);
		MUTEX_UNLOCK(httpd->run_mutex);
		return -1;
	}
	//httpd->server_fd6 = netutils_init_socket(port, 1, 0);
	//if (httpd->server_fd6 == -1) {
	//	if(DBG) ALOGW("Error initialising IPv6 socket %d", SOCKET_GET_ERROR());
	//	if(DBG) ALOGW("Continuing without IPv6 support");
	//}

	if (httpd->server_fd4 != -1 && listen(httpd->server_fd4, backlog) == -1) {
		if(DBG) ALOGE("Error listening to IPv4 socket");
		closesocket(httpd->server_fd4);
		closesocket(httpd->server_fd6);
		MUTEX_UNLOCK(httpd->run_mutex);
		return -2;
	}
	//if (httpd->server_fd6 != -1 && listen(httpd->server_fd6, backlog) == -1) {
	//	if(DBG) ALOGE("Error listening to IPv6 socket");
	//	closesocket(httpd->server_fd4);
	//	closesocket(httpd->server_fd6);
	//	MUTEX_UNLOCK(httpd->run_mutex);
	//	return -2;
	//}
	if(DBG) ALOGI("Initialized server socket(%d) on port(%d)", httpd->server_fd4, *port);

	/* Set values correctly and create new thread */
	httpd->m_running = 1;
	httpd->m_joined = 0;
	THREAD_CREATE(httpd->thread, httpd_thread, httpd);
	MUTEX_UNLOCK(httpd->run_mutex);

	return 1;
}

int
httpd_is_running(httpd_t *httpd)
{
	int running;

	assert(httpd);

	MUTEX_LOCK(httpd->run_mutex);
	running = httpd->m_running || !httpd->m_joined;
	MUTEX_UNLOCK(httpd->run_mutex);

	return running;
}

void
httpd_stop(httpd_t *httpd)
{
	ALOGE("%s(%d)", __func__, __LINE__);
	assert(httpd);

	MUTEX_LOCK(httpd->run_mutex);
	if (!httpd->m_running || httpd->m_joined) {
		MUTEX_UNLOCK(httpd->run_mutex);
		return;
	}
	httpd->m_running = 0;
	MUTEX_UNLOCK(httpd->run_mutex);

	THREAD_JOIN(httpd->thread);

	MUTEX_LOCK(httpd->run_mutex);
	httpd->m_joined = 1;
	MUTEX_UNLOCK(httpd->run_mutex);
}

int
httpd_get_socket(httpd_t *httpd)
{
	return event_socket;
}
