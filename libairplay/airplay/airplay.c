#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
 

#include "airplay.h"
#include "mycrypt.h"
#include "raop_rtp.h"
#include "digest.h"
#include "httpd.h"	
#include "sdp.h"
#include "global.h"
#include "utils.h"
#include "netutils.h"
#include "compat.h"
#include "plist.h"
#include <android/log.h>

//to do fairplay
//#include "li"

#define MAX_SIGNATURE_LEN 512

#define MAX_PASSWORD_LEN 64

/* MD5 as hex fits here */
#define MAX_NONCE_LEN 32

#define MAX_PACKET_LEN 4096
#define LOG_TAG "airplay"
#define DBG	0

struct airplay_conn_s {
	airplay_t *airplay;
	raop_rtp_t *airplay_rtp;

	unsigned char *local;
	int locallen;

	unsigned char *remote;
	int remotelen;

	char nonce[MAX_NONCE_LEN + 1];

	unsigned char aeskey[16];
	unsigned char iv[16];
	unsigned char buffer[MAX_PACKET_LEN];
	int pos;
};
typedef struct airplay_conn_s airplay_conn_t;

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK                  200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NEED_AUTH           401
#define AIRPLAY_STATUS_NOT_FOUND           404
#define AIRPLAY_STATUS_METHOD_NOT_ALLOWED  405
#define AIRPLAY_STATUS_PRECONDITION_FAILED 412
#define AIRPLAY_STATUS_NOT_IMPLEMENTED     501
#define AIRPLAY_STATUS_NO_RESPONSE_NEEDED  1000

#define EVENT_NONE     -1
#define EVENT_PLAYING   0
#define EVENT_PAUSED    1
#define EVENT_LOADING   2
#define EVENT_STOPPED   3
char deviceId[32] = {0};

const char *eventStrings[] = { "playing", "paused", "loading", "stopped" };

/* #define STREAM_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>width</key>\r\n"\
"<integer>1280</integer>\r\n"\
"<key>height</key>\r\n"\
"<integer>720</integer>\r\n"\
"<key>version</key>\r\n"\
"<string>110.92</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n" */

#define SET_PROPERTY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>errorCode</key>\r\n"\
"<integer>0</integer>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define PLAYBACK_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>duration</key>\r\n"\
"<real>%ld</real>\r\n"\
"<key>loadedTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%ld</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"<key>playbackBufferEmpty</key>\r\n"\
"<true/>\r\n"\
"<key>playbackBufferFull</key>\r\n"\
"<false/>\r\n"\
"<key>playbackLikelyToKeepUp</key>\r\n"\
"<true/>\r\n"\
"<key>position</key>\r\n"\
"<real>%ld</real>\r\n"\
"<key>rate</key>\r\n"\
"<real>%d</real>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<true/>\r\n"\
"<key>seekableTimeRanges</key>\r\n"\
"<array>\r\n"\
"\t\t<dict>\r\n"\
"\t\t\t<key>duration</key>\r\n"\
"\t\t\t<real>%ld</real>\r\n"\
"\t\t\t<key>start</key>\r\n"\
"\t\t\t<real>0.0</real>\r\n"\
"\t\t</dict>\r\n"\
"</array>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define PLAYBACK_INFO_NOT_READY  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>readyToPlay</key>\r\n"\
"<false/>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define SERVER_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>deviceid</key>\r\n"\
"<string>%s</string>\r\n"\
"<key>macAddress</key>\r\n"\
"<string>%s</string>\r\n"\
"<key>features</key>\r\n"\
"<integer>10879</integer>\r\n"\
"<key>model</key>\r\n"\
"<string>AppleTV3,1</string>\r\n"\
"<key>protovers</key>\r\n"\
"<string>1.0</string>\r\n"\
"<key>srcvers</key>\r\n"\
"<string>200.54</string>\r\n"\
"<key>osBuildVersion</key>\r\n"\
"<string>11D258</string>\r\n"\
"<key>vv</key>\r\n"\
"<integer>2</integer>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"

#define EVENT_INFO "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>category</key>\r\n"\
"<string>video</string>\r\n"\
"<key>sessionID</key>\r\n"\
"<integer>%d</integer>\r\n"\
"<key>state</key>\r\n"\
"<string>%s</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"\


#define EVENT_BACK "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\r\n"\
"<plist version=\"1.0\">\r\n"\
"<dict>\r\n"\
"<key>request</key>\r\n"\
"<dict>\r\n"\
"<key>FCUP_Response_ClientInfo</key>\r\n"\
"<integer>%d</integer>\r\n"\
"<key>FCUP_Response_ClientRef</key>\r\n"\
"<integer>%d</integer>\r\n"\
"<key>FCUP_Response_Headers</key>\r\n"\
"<dict>\r\n"\
"<key>X-Playback-Session-Id</key>\r\n"\
"<string>%s</string>\r\n"\
"</dict>\r\n"\
"<key>FCUP_Response_RequestID</key>\r\n"\
"<integer>%d</integer>\r\n"\
"<key>FCUP_Response_URL</key>\r\n"\
"<string>%s</string>\r\n"\
"<key>sessionID</key>\r\n"\
"<integer>0</integer>\r\n"\
"</dict>\r\n"\
"<key>sessionID</key>\r\n"\
"<integer>0</integer>\r\n"\
"<key>type</key>\r\n"\
"<string>unhandledURLRequest</string>\r\n"\
"</dict>\r\n"\
"</plist>\r\n"\

#define AUTH_REALM "AirPlay"
#define AUTH_REQUIRED "WWW-Authenticate: Digest realm=\""  AUTH_REALM  "\", nonce=\"%s\"\r\n"

char m_sessionId[128];
int rate;
int wait;
#define G_ID_LIST_SIZE	1024
char g_id_list[G_ID_LIST_SIZE];
double m_position;

static void *
conn_init(void *opaque, unsigned char *local, int locallen, unsigned char *remote, int remotelen)
{
	airplay_conn_t *conn;

	conn = calloc(1, sizeof(airplay_conn_t));
	if (!conn) {
		return NULL;
	}
	conn->airplay = opaque;
	conn->airplay_rtp = NULL;

	if (locallen == 4) {
		if(DBG) ALOGI("Local: %d.%d.%d.%d",
			local[0], local[1], local[2], local[3]);
	} else if (locallen == 16) {
		if(DBG) ALOGI("Local: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
			local[0], local[1], local[2], local[3], local[4], local[5], local[6], local[7],
			local[8], local[9], local[10], local[11], local[12], local[13], local[14], local[15]);
	}
	if (remotelen == 4) {
		if(DBG) ALOGI("Remote: %d.%d.%d.%d",
			remote[0], remote[1], remote[2], remote[3]);
	} else if (remotelen == 16) {
		if(DBG) ALOGI("Remote: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
			remote[0], remote[1], remote[2], remote[3], remote[4], remote[5], remote[6], remote[7],
			remote[8], remote[9], remote[10], remote[11], remote[12], remote[13], remote[14], remote[15]);
	}

	conn->local = malloc(locallen);
	assert(conn->local);
	memcpy(conn->local, local, locallen);

	conn->remote = malloc(remotelen);
	assert(conn->remote);
	memcpy(conn->remote, remote, remotelen);

	conn->locallen = locallen;
	conn->remotelen = remotelen;
	digest_generate_nonce(conn->nonce, sizeof(conn->nonce));
	return conn;
}

http_response_t * request_handle_pairverify_airplay(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	airplay_t *raop = conn->airplay;
	unsigned char ed_msg[64];
	unsigned char ed_sig[64];
	char key_salt[] = "Pair-Verify-AES-Key";
	char iv_salt[] = "Pair-Verify-AES-IV";
	unsigned char key_buf[64];
	unsigned char iv_buf[64];
	int body_size = 0;
	const char* body_data = http_request_get_data(request, &body_size);
	air_pair_t * cx = &raop->pair_data;
	if(DBG) ALOGD(" %s:%d\n", __func__, __LINE__);
	char* psend = 0;
	memcpy(cx->ed_pub, g_ed_public_key, 32);
	memcpy(cx->ed_pri, g_ed_private_key, 64);
	if (*body_data == 1) {
		memcpy(cx->cv_his, body_data+4, 32);
		memcpy(cx->ed_his, body_data+36, 32);
		ed25519_create_seed(cx->cv_pri);
		curve25519_donna(cx->cv_pub, cx->cv_pri, 0);
		curve25519_donna(cx->cv_sha, cx->cv_pri, cx->cv_his);
		// memcpy(conn->cv_sha, cx->cv_sha, 32);

		memcpy(&ed_msg[0], cx->cv_pub, 32);
		memcpy(&ed_msg[32], cx->cv_his, 32);
		ed25519_sign(ed_sig, ed_msg, 64, cx->ed_pub, cx->ed_pri);
		sha512msg((const unsigned char*)key_salt, strlen(key_salt), cx->cv_sha, 32, key_buf);
		sha512msg((const unsigned char*)iv_salt, strlen(iv_salt), cx->cv_sha, 32, iv_buf);
		memcpy(cx->ctr_key, key_buf, 16);
		memcpy(cx->ctr_iv, iv_buf, 16);
		cx->ctr_num = 0;
		memset(cx->ctr_ec, 0, 16);
		AES_set_encrypt_key(cx->ctr_key, 128, &cx->aes_key);
		new_AES_ctr128_encrypt(ed_sig, ed_sig, sizeof(ed_sig), &cx->aes_key, cx->ctr_iv, cx->ctr_ec, &cx->ctr_num);
		psend = (char*)calloc(1, 96);
		memcpy(psend, cx->cv_pub, 32);
		memcpy(psend+32, ed_sig, 64);
		*pResponseData = psend;
		*pResponseDataLen = 96;
		http_response_add_header(response, "Content-Type", "application/octet-stream");
	} else {
		memcpy(ed_sig, body_data+4, 64);
		new_AES_ctr128_encrypt(ed_sig, ed_sig, sizeof(ed_sig), &cx->aes_key, cx->ctr_iv, cx->ctr_ec, &cx->ctr_num);
		memcpy(&ed_msg[0], cx->cv_his, 32);
		memcpy(&ed_msg[32], cx->cv_pub, 32);
		if (!ed25519_verify(ed_sig, ed_msg, 64, cx->ed_his)) {
			http_response_add_header(response, "Connection", "close");
		}
		http_response_add_header(response, "Content-Type", "application/octet-stream");
	}
	return response;
}


http_response_t * request_handle_serverinfo(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	airplay_t *raop = conn->airplay;
	char* buffer = (char *)malloc(4096 * 4);
	char* p_bin = 0;
	uint32_t bin_size = 0;
	plist_t p_xml = 0;
	memset(buffer, 0, 4096 * 4);

	//memset(buffer, 0, sizeof(buffer));
	sprintf((char *)buffer,
			SERVER_INFO, deviceId, deviceId);
	// plist_from_xml(buffer, strlen(buffer), &p_xml);
	// plist_to_bin(p_xml, &p_bin, &bin_size);
	*pResponseData = buffer;//(char*)memdup(p_bin, bin_size);
	*pResponseDataLen = strlen(buffer);
	// plist_free(p_xml);
	return response;
}

http_response_t * request_handle_send_back(airplay_conn_t *conn, char *url) {
	char* buffer = (char *)malloc(4096 * 4);
	char *g_id = NULL;
	char i_g_id_list[G_ID_LIST_SIZE] = {0};
	if(url == NULL) {
		int wait_count = 10;
		for(;wait_count;wait_count--) {
			sleep(1);
			if(!strncmp("start#", g_id_list, strlen("start#"))) {
				strcpy(i_g_id_list, g_id_list);
				g_id = strtok(i_g_id_list, "#");
				ALOGD("%s(%d) g_id_list=%s", __func__, __LINE__, g_id_list);
				break;
			}
			ALOGD("%s(%d) waiting for master.m3u8. (%d)", __func__, __LINE__, 10 - wait_count);
		}
	}
	while(1) {
		memset(buffer, 0, 4096 * 4);
		if(url != NULL) {
			//system("rm /mnt/ram0/*.m3u8");
			sprintf((char *) buffer, EVENT_BACK, 0, 0, m_sessionId, 0,  url);
		} else {
			char dataUrl[128] = {0};
			g_id = strtok(NULL, "#");
			if(g_id == NULL) {
				break;
			}
			sprintf(dataUrl, "mlhls://localhost/itag/%s/mediadata.m3u8", g_id);
			sprintf((char *) buffer, EVENT_BACK, 0, 0, m_sessionId, 0, dataUrl);
		}
		if(DBG) ALOGD("1send_back:: %s", buffer);

		http_response_t *res = 0;
		res = http_response_init_ex("POST", "/event", "HTTP/1.1");
		http_response_add_header(res, "Content-Type", "text/x-apple-plist+xml");
		http_response_add_header(res,  "X-Apple-Session-ID", m_sessionId);
		http_response_add_header(res, "Server", "AirTunes/220.68");
		time_t timestamp = time(0);
		struct tm *t = gmtime(&timestamp);
		char* str_time = asctime(t);
		str_time[strlen(str_time) - 1] = 0;
		http_response_add_header(res, "Date", str_time);

		airplay_t *raop = conn->airplay;
		char* p_bin = 0;
		uint32_t bin_size = 0;
		plist_t p_xml = 0;

		//	*pResponseData = buffer;
		//	*pResponseDataLen = strlen(buffer);
		http_response_finish(res, buffer, strlen(buffer));
		//	unsigned short cport = 80;
		//	int use_ipv6 = 0;
		//	int eventSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		//	socklen_t saddrlen = sizeof(conn->remotelen);
		//	int datalen;
		//	const char *data = http_response_get_data(res, &datalen);
		//	char* buf_remote = (char*)malloc(128);
		//	sprintf(buf_remote, "%d.%d.%d.%d", conn->remote[0], conn->remote[1], conn->remote[2], conn->remote[3]);
		//	if(DBG) ALOGD("conn->remote: %s eventSocket:::%d\n", buf_remote,eventSocket);
		//	struct sockaddr_in ser_addr;
		//	ser_addr.sin_addr.s_addr = inet_addr("192.168.124.204");
		//	ser_addr.sin_port = htons(80);
		//    if(connect(eventSocket, (struct sockaddr *) &ser_addr, sizeof(ser_addr)) < 0) {
		//		if(DBG) ALOGD("can't connect to server");
		//        if(DBG) ALOGD("SOCKET_GET_ERROR():: %d", SOCKET_GET_ERROR());
		//    }
		//		const struct sockaddr *ser_addr;
		//		netutils_parse_address(AF_INET, buf_remote, &ser_addr, sizeof(ser_addr));
		//    int ret = sendto(eventSocket, (const char *)data, datalen, 0, &ser_addr, sizeof(ser_addr));
		int datalen, ret;
		/* Get response data and datalen */
		const char *data = http_response_get_data(res, &datalen);
		int written = 0;
		ALOGD("%s(%d) event_socket=%d", __func__, __LINE__, httpd_get_socket(conn->airplay->main_server));
		while (written < datalen) {
			ret = send(httpd_get_socket(conn->airplay->main_server), data+written, datalen-written, 0);
			if (ret == -1) {
				/* FIXME: Error happened */
				if(DBG) ALOGD("SOCKET_GET_ERROR():: %d", SOCKET_GET_ERROR());
				break;
			}
			written += ret;
		}
		if(url != NULL) break;
	}
	if(buffer != NULL) free(buffer);
	return NULL;
}

//TODO 播放
http_response_t * request_handle_play(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	const char* content_type = http_request_get_header(request, "Content-Type", 1);
	char *buffer = NULL;
	uint32_t size = 0;
	plist_t p_dict;
	plist_t type = NULL;
	plist_t host = NULL;
	plist_t path = NULL;
	plist_t position = NULL;
	plist_t url = NULL;
	char* typeStr = NULL;
	char* hostStr = NULL;
	char* pathStr = NULL;
	char* p_url_data = NULL;
	uint64_t p_url_size = 0;
	int data_size;
	const char* data = http_request_get_data(request, &data_size);
	char *fPositionStr = NULL;
	float fPosition = 0.0f;
	double dPosition = 0.0f;
	int isAdverts = 0; //过滤广告
 
	if(DBG) ALOGD("Content-Type = %s", content_type);
	if (content_type && !strcmp(content_type, "application/x-apple-binary-plist")) {
		plist_from_bin(data, data_size, &p_dict);
		plist_to_xml(p_dict,&buffer,&size);
		if(DBG) ALOGD_EX("buffer", buffer, size);
		type = plist_dict_get_item(p_dict, "itemType");
		host = plist_dict_get_item(p_dict, "host");
		path = plist_dict_get_item(p_dict, "path");
		url = plist_dict_get_item(p_dict, "Content-Location");
		position = plist_dict_get_item(p_dict, "Start-Position-Seconds");
		plist_get_string_val(type, &typeStr);
		plist_get_string_val(host, &hostStr);
		plist_get_string_val(path, &pathStr);
		plist_get_string_val(url, &p_url_data);
		plist_get_real_val(position, &dPosition);
		if(p_url_data != NULL) {
			if(strncmp(p_url_data, "http://127.0.0.1", strlen("http://127.0.0.1"))) {
				if(!strncmp(p_url_data + strlen(p_url_data) - 3, "m4v", 3) ||
						!strncmp(p_url_data + strlen(p_url_data) - 3, "mp4", 3)) {
					if(typeStr != NULL && !strncmp("purchased", typeStr, 9)) {
						isAdverts = 0;
					}
				}else if(!strncmp(p_url_data, "mlhls://", strlen("mlhls://"))) {
					memset(g_id_list, 0, G_ID_LIST_SIZE);
					request_handle_send_back(conn, p_url_data);
					m_position = dPosition;
				} else if(!strncmp(p_url_data, "nfhls://", strlen("nfhls://"))) {
					memset(g_id_list, 0, G_ID_LIST_SIZE);
					request_handle_send_back(conn, p_url_data);
					m_position = dPosition;
				} else {
					isAdverts = 0;
				}
			}
			if(!isAdverts) {
				conn->airplay->callbacks.AirPlayPlayback_Open(conn->airplay->callbacks.cls, p_url_data, 0, dPosition * 1000);
				rate = 1;
				wait = 1;
			}
		} else if(typeStr != NULL && !strncmp("purchased", typeStr, 9)) {
			if(hostStr != NULL && pathStr != NULL &&
					strncmp(hostStr, "127.0.0.1", strlen("127.0.0.1"))) {
				char *localUrl = (char *)malloc(strlen(hostStr) + strlen(pathStr) + 8);
				sprintf(localUrl, "http://%s%s", hostStr, pathStr);
				conn->airplay->callbacks.AirPlayPlayback_Open(conn->airplay->callbacks.cls, localUrl, 0, dPosition * 1000);
				rate = 1;
				wait = 1;
				//free(localUrl);
			}
		}
	}else{
		if(DBG) ALOGD_EX("data", data, data_size);
		p_url_data = data + 18;
		fPositionStr = strstr(p_url_data, "Start-Position") + 16;
		if(fPositionStr != NULL) fPosition = atof(fPositionStr);
		p_url_data = strtok(p_url_data, " \r\n");
		if(p_url_data != NULL) {
			if(strncmp(p_url_data, "http://127.0.0.1", strlen("http://127.0.0.1"))) {
				isAdverts = 0;
			}
		}
		if(!isAdverts) {
			conn->airplay->callbacks.AirPlayPlayback_Open(conn->airplay->callbacks.cls, p_url_data, fPosition, 0);
			rate = 1;
			wait = 1;
		}
	}
	return response;
}

struct airplay_m3u8_handle {
	char *g_id;
	char *m3u8_data;
	airplay_conn_t *conn;
};
void * airplay_m3u8_handle_thread(void *args) {
	struct airplay_m3u8_handle *handle = (struct airplay_m3u8_handle *)args;
	char localUrl[128] = {0};
	sprintf(localUrl, "/mnt/ram0/%s_mediadata.m3u8", handle->g_id);

	pthread_detach(pthread_self());
	if(!trans(handle->m3u8_data, localUrl, 0, NULL, 0)) {
		char *p_last = strrchr(g_id_list, '#') + 1;
		ALOGD("%s(%d) g_id_list=%s", __func__, __LINE__, g_id_list);
		ALOGD("%s(%d) handle->g_id=%s p_last=%s", __func__, __LINE__, handle->g_id, p_last);
		if(!strncmp(handle->g_id, p_last, strlen(p_last))) {
			handle->conn->airplay->callbacks.AirPlayPlayback_Open(handle->conn->airplay->callbacks.cls, "http://localhost/itag/master.m3u8", 0, m_position);
			rate = 1;
			wait = 1;
		}
	} else {
		request_handle_send_back(handle->conn, "mlhls://localhost/itag/master.m3u8");
	}
	free(handle->g_id);
	free(handle->m3u8_data);

	return NULL;
}

/**
 * add by cz
 * 1.解决国外镜像转投屏，action 中获取播放地址
 */
http_response_t * request_handle_action(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	const char* content_type = http_request_get_header(request, "Content-Type", 1);
	char *buffer = NULL;
	uint32_t size = 0;
	int data_size;
	plist_t p_dict;
	if (content_type && !strcmp(content_type, "application/x-apple-binary-plist")) {
		plist_t url = NULL;
		char* p_url_data = 0;
		uint64_t p_url_size = 0;

		const char* data = http_request_get_data(request, &data_size);
		plist_from_bin(data, data_size, &p_dict);
		plist_to_xml(p_dict,&buffer,&size);
        if(DBG) ALOGD("action buffer :: %s", buffer);
        if(p_dict == NULL){

        }
        char* typeStr = NULL;
        plist_t type = plist_dict_get_item(p_dict, "type");
        plist_get_string_val(type, &typeStr);
        if(DBG) ALOGD("action typeStr::::%s", typeStr);
		if(!strncmp("playlistRemove", typeStr, strlen("playlistRemove"))) {
		} else if(!strncmp("unhandledURLResponse", typeStr, strlen("unhandledURLResponse"))) {
        	plist_t paramsdata = plist_dict_get_item(p_dict, "params");
        	plist_t m3u8data = plist_dict_get_item(paramsdata, "FCUP_Response_Data");
        	char* p_ekey_data = 0;
        	uint64_t p_ekey_size = 0;
        	plist_get_data_val(m3u8data, &p_ekey_data, &p_ekey_size);
        	if(DBG) ALOGD("action buffer :: %s", p_ekey_data);
        	if(DBG) ALOGD("p_ekey_size: %d", p_ekey_size);
        	char* responseUrlStr = NULL;
        	plist_t responseUrl = plist_dict_get_item(paramsdata, "FCUP_Response_URL");
        	plist_get_string_val(responseUrl, &responseUrlStr);
        	if(DBG) ALOGD("action responseUrlStr::::%s", responseUrlStr);
			if(responseUrlStr != NULL) {
				char localUrl[128] = {0};
				if(strstr(responseUrlStr, "master.m3u8")) {
					strcpy(localUrl, "/mnt/ram0/master.m3u8");
					trans(p_ekey_data, localUrl, 1, g_id_list, G_ID_LIST_SIZE);
					ALOGD("%s(%d) g_id_list=%s", __func__, __LINE__, g_id_list);
					if(strlen(g_id_list) > 0) {
						char g_id[128] = {0};
						char dataUrl[256] = {0};
						strcpy(g_id, g_id_list);
						char *p = strchr(g_id, '#');
						if(p != NULL) *p = 0;
						sprintf(dataUrl, "mlhls://localhost/itag/%s/mediadata.m3u8", g_id);
						request_handle_send_back(conn, dataUrl);
					}
				} else if(strstr(responseUrlStr, "index.m3u8")) {
					strcpy(localUrl, "/mnt/ram0/index.m3u8");
					trans(p_ekey_data, localUrl, 1, g_id_list, G_ID_LIST_SIZE);
					ALOGD("%s(%d) g_id_list=%s", __func__, __LINE__, g_id_list);
					if(strlen(g_id_list) > 0) {
						char g_id[128] = {0};
						char dataUrl[256] = {0};
						strcpy(g_id, g_id_list);
						char *p = strchr(g_id, '#');
						if(p != NULL) *p = 0;
						sprintf(dataUrl, "mlhls://localhost/itag/%s/mediadata.m3u8", g_id);
						request_handle_send_back(conn, dataUrl);
					}
				} else if(strstr(responseUrlStr, "mediadata.m3u8")){
					ALOGD("%s(%d) g_id_list=%s", __func__, __LINE__, g_id_list);
					char *p_last = strrchr(g_id_list, '#');
					if(p_last != NULL) {
						char *p = strstr(responseUrlStr, "mlhls://localhost/itag/") + strlen("mlhls://localhost/itag/");
						if(p != NULL) {
							char *p_end = strchr(p, '/');
							if(p_end != NULL && p != p_end) {
								char g_id[128] = {0};
								char localUrl[256] = {0};
								strncpy(g_id, p, p_end - p);
								sprintf(localUrl, "/mnt/ram0/%s_mediadata.m3u8", g_id);
								if(!trans(p_ekey_data, localUrl, 0, NULL, 0)) {
									char *p_last = strrchr(g_id_list, '#') + 1;
									ALOGD("%s(%d) g_id_list=%s", __func__, __LINE__, g_id_list);
									ALOGD("%s(%d) g_id=%s p_last=%s", __func__, __LINE__, g_id, p_last);
									if(!strncmp(g_id, p_last, strlen(p_last))) {
										conn->airplay->callbacks.AirPlayPlayback_Open(conn->airplay->callbacks.cls, "http://localhost/itag/master.m3u8", 0, m_position);
										rate = 1;
										wait = 1;
									} else {
										char dataUrl[256] = {0};
										sprintf(dataUrl, "mlhls://localhost/itag/%s/mediadata.m3u8", p_last);
										request_handle_send_back(conn, dataUrl);
									}
								} else {
									request_handle_send_back(conn, "mlhls://localhost/itag/master.m3u8");
								}
							}
						}
					}
				}
			}
		}
	}else{

	}
	plist_free(p_dict);
    return response;
}

/**
 * add by cz
 * 1.解决国外镜像转投屏，action 中获取播放地址
 */
http_response_t * request_handle_setProperty(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	const char* content_type = http_request_get_header(request, "Content-Type", 1);
	char *buffer = NULL;
	uint32_t size = 0;
	int data_size;
	plist_t p_dict;
	if (content_type && !strcmp(content_type, "application/x-apple-binary-plist")) {
		plist_t url = NULL;
		char* p_url_data = 0;
		uint64_t p_url_size = 0;

		const char* data = http_request_get_data(request, &data_size);
		plist_from_bin(data, data_size, &p_dict);
		plist_to_xml(p_dict,&buffer,&size);
		if(DBG) ALOGD("setProperty buffer :: %s", buffer);


		airplay_t *raop = conn->airplay;
		char buffer[4096 * 4];
		char* p_bin = 0;
		uint32_t bin_size = 0;
		plist_t p_xml = 0;
		memset(buffer, 0, 4096 * 4);
 
		sprintf(
				(char *)buffer,
				SET_PROPERTY);
		if(DBG) ALOGD("setProperty back buffer :: %s", buffer);
		plist_from_xml(buffer, strlen(buffer), &p_xml);
		plist_to_bin(p_xml, &p_bin, &bin_size);

		*pResponseData = (char*)memdup(p_bin, bin_size);
		*pResponseDataLen = bin_size;
		plist_free(p_xml);
		return response;
	}else{

	}
    return response;
}

//TODO 播放停止
http_response_t * request_handle_stop(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	conn->airplay->callbacks.AirPlayPlayback_Stop(conn->airplay->callbacks.cls);
	return response;
}

//TODO 播放进度控制
http_response_t * request_handle_post_scrub(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	char *uri = http_request_get_url(request, 1);
	long value = atol(uri + 16) * 1000;
	conn->airplay->callbacks.AirPlayPlayback_Seek(conn->airplay->callbacks.cls,value);
	return response;
}

//TODO 播放进度获取
http_response_t * request_handle_get_scrub(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	long duration = conn->airplay->callbacks.AirPlayPlayback_GetDuration(conn->airplay->callbacks.cls)/1000;
	long position = conn->airplay->callbacks.AirPlayPlayback_GetPostion(conn->airplay->callbacks.cls)/1000;
	if(rate){position = position +1;}
	char *buffer = (char *)malloc(1024);;
	sprintf(buffer, "duration: %ld\r\nposition: %ld\r\n", duration, position);
	*pResponseData = buffer;
	*pResponseDataLen = strlen(buffer);

	return response;
}

//TODO 暂停和继续播放
http_response_t * request_handle_rate(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	//拒绝AirPlay推送的第一个STOP命令
	if(wait) {wait=0; return response;}
	char *uri = http_request_get_url(request, 1);
	rate = atoi(uri + 12);
	if(rate){
		conn->airplay->callbacks.AirPlayPlayback_Play(conn->airplay->callbacks.cls);
	}else{
		conn->airplay->callbacks.AirPlayPlayback_Pause(conn->airplay->callbacks.cls);
	}
	return response;
}

http_response_t * request_handle_playback_info(airplay_conn_t *conn, http_request_t *request, http_response_t *response, char **pResponseData, int *pResponseDataLen) {
	long duration = conn->airplay->callbacks.AirPlayPlayback_GetDuration(conn->airplay->callbacks.cls)/1000;
	long position = conn->airplay->callbacks.AirPlayPlayback_GetPostion(conn->airplay->callbacks.cls)/1000;
	if(rate){position = position +1;}

	char buffer[4096 * 4];
	char* p_bin = 0;
	uint32_t bin_size = 0;
	plist_t p_xml = 0;
	memset(buffer, 0, sizeof(buffer));
	sprintf((char *)buffer,PLAYBACK_INFO,duration,duration,position,rate,duration);
	// if(DBG) ALOGD("--- CCCC ::: %s \n",buffer);
	//if(DBG) ALOGD("info buffer CCCC :: %s\n", buffer);
	plist_from_xml(buffer, strlen(buffer), &p_xml);
	plist_to_bin(p_xml, &p_bin, &bin_size);
	//if(DBG) ALOGD("playback_info back buffer :: %s", buffer);
	*pResponseData = (char*)memdup(p_bin, bin_size);
	*pResponseDataLen = bin_size;
	plist_free(p_xml);
	//http_response_add_header(response, "Content-Type", "text/x-apple-plist+xml");
	return response;
}


//http_response_t * request_handle_send_back(airplay_conn_t *conn, http_request_t *request,http_response_t *response, char **pResponseData, int *pResponseDataLen,int index) {
//
//        http_response_t *res = 0;
//        res = http_response_init_ex("POST", "/event", "HTTP/1.1");
//        http_response_add_header(res, "Content-Type", "text/x-apple-plist+xml");
//        http_response_add_header(res,  "X-Apple-Session-ID", m_sessionId);
//        http_response_add_header(res, "Server", "AirTunes/220.68");
//        time_t timestamp = time(0);
//        struct tm *t = gmtime(&timestamp);
//        char* str_time = asctime(t);
//        str_time[strlen(str_time) - 1] = 0;
//        http_response_add_header(res, "Date", str_time);
//
//        airplay_t *raop = conn->airplay;
//        char* buffer = (char *)malloc(4096 * 4);
//        char* p_bin = 0;
//        uint32_t bin_size = 0;
//        plist_t p_xml = 0;
//        memset(buffer, 0, sizeof(buffer));
//        if(index == 0) {
//            sprintf((char *) buffer, EVENT_BACK, 0, 0, m_sessionId, 0, "mlhls://localhost/master.m3u8");
//        }else if(index == 1){
//            sprintf((char *) buffer, EVENT_BACK, 0, 0, m_sessionId, 0, "mlhls://localhost/itag/234/mediadata.m3u8");
//        } else{
//            sprintf((char *) buffer, EVENT_BACK, 0, 0, m_sessionId, 0, "mlhls://localhost/itag/232/mediadata.m3u8");
//        }
//        if(DBG) ALOGD("1send_back:: %s", buffer,10);
//	    *pResponseData = buffer;
//	    *pResponseDataLen = strlen(buffer);
//    return response;
//}

int eventIndex = 3;
/**
 *
 * modify by cz youtube 国外版
 * */
static void conn_request(void *ptr, http_request_t *request, http_response_t **response, int index) {
	const char realm[] = "airplay";
	airplay_conn_t *conn = ptr;
	airplay_t *airplay = conn->airplay;
	http_response_t *res = 0;

	const char *cseq;
	const char *challenge;
	int require_auth = 0;
	char responseHeader[4096];
	// char responseBody[4096];
	char *responseBody = 0;
	int responseLength = 0;

	const char *uri = http_request_get_url(request, index);
	const char *method = http_request_get_method(request);

	const char * contentType = http_request_get_header(request, "Content-Type", 1);
	const char * sessionId = http_request_get_header(request, "X-Apple-Session-ID", 1);
	const char * authorization = http_request_get_header(request, "Authorization", 1);
	const char * photoAction = http_request_get_header(request, "X-Apple-Assetaction", 1);
	const char * photoCacheId = http_request_get_header(request, "X-Apple-Assetkey", 1);
	if(DBG) ALOGD("contentType ::: %s \n",contentType);
	if(DBG) ALOGD("m_sessionId ::: %s \n",sessionId);
	if(DBG) ALOGD("authorization ::: %s \n",authorization);
	if(DBG) ALOGD("photoAction ::: %s \n",contentType);
	if(DBG) ALOGD("contentType ::: %s \n",contentType);
	if(DBG) ALOGD("contentType ::: %s \n",contentType);

	if(sessionId) {
		memset(m_sessionId, 0, sizeof(m_sessionId));
		strcpy(m_sessionId, sessionId);
	}

	int status = AIRPLAY_STATUS_OK;
	int needAuth = 0;

	if(DBG) ALOGD("1-----------------------------------------------------------------------------------");
	if(DBG) ALOGD("===================================================================================");

	if (!method) {
		return;
	}

	if(!strcmp(method,"POST") && !strcmp(uri,"/reverse")){
		res = http_response_init("HTTP/1.1", 101, "Switching Protocols");
	}else{
		res = http_response_init("HTTP/1.1", 200, "OK");
	}
	if(DBG) ALOGD("%s uri=%s\n", method, uri);


	const char *data;
	int len;
	data = http_request_get_data(request, &len);
	//if(DBG) ALOGD("data len:::: %d:%s\n", len, data);
    //ALOGE( "data len:::: %d:%s\n", len, bin2hex(data,len));
	if (!strcmp(method,"POST") && !strcmp(uri,"/pair-verify")) {
		res = request_handle_pairverify_airplay(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"GET") && !strcmp(uri,"/server-info")){
		res = request_handle_serverinfo(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"POST") && !strcmp(uri,"/play")){
		res = request_handle_play(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"POST") && !strcmp(uri,"/stop")){
		res = request_handle_stop(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"POST") && !strncmp(uri,"/scrub",6)){
		res = request_handle_post_scrub(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"GET") && !strncmp(uri,"/scrub",6)){
		res = request_handle_get_scrub(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"POST") && !strncmp(uri,"/rate",5)){
		res = request_handle_rate(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"GET") && !strcmp(uri,"/playback-info")){
		res = request_handle_playback_info(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"POST") && !strcmp(uri,"/action")){
		if(DBG) ALOGD("cz action");
		res = request_handle_action(conn, request, res, &responseBody, &responseLength);
	}else if(!strcmp(method,"PUT") && !strncmp(uri,"/setProperty",strlen("/setProperty"))){
		if(DBG) ALOGD("cz setProperty");
		if(strcmp(uri,"/setProperty?textMarkupArray")){
			res = request_handle_setProperty(conn, request, res, &responseBody, &responseLength);
			http_response_add_header(res, "Content-Type", "application/x-apple-binary-plist");
			if(strcmp(uri,"/setProperty?selectedMediaArray") && strlen(m_sessionId)){
				http_response_add_header(res, "X-Apple-Session-ID", m_sessionId);
			}
		}
	}
		/**
		 * youtube 相关
		 * HTTP/1.1 101 Switching Protocols
		 * Server: AirTunes/220.68
		 * Upgrade: PTTH/1.0
		 * Connection: Upgrade
		 * Content-Length: 0
		 * Date: Mon, 10 Dec 2018 06:17:52 GMT
		 *
		 *
		 * POST /event HTTP/1.1
		 * Content-Type: text/x-apple-plist+xml
		 * X-Apple-Session-ID: 88919a48-6b9e-41a1-b554-eb0074703fbf
		 * Content-Length: 810
		 * */
	else if(!strcmp(method,"POST") && !strcmp(uri,"/reverse")){
//		if(eventIndex == 3){
			http_response_add_header(res, "Upgrade", "PTTH/1.0");
			http_response_add_header(res, "Connection", "Upgrade");
			http_response_set_flag(res, eventIndex);
//		}else{
//			res = http_response_init_ex("POST", "/event", "HTTP/1.1");
//			http_response_add_header(res, "Content-Type", "text/x-apple-plist+xml");
//			http_response_add_header(res,  "X-Apple-Session-ID", m_sessionId);
//            request_handle_send_back(conn, request,res,&responseBody, &responseLength,eventIndex);
//		}
	}
    if(DBG) ALOGD("::::: %s:%d  airplay->main_server->server_fd4:::%d\n", __func__, __LINE__, httpd_get_socket(airplay->main_server));
	if (strcmp(uri,"/reverse")) {
		time_t timestamp = time(0);
		struct tm *t = gmtime(&timestamp);
		char* str_time = asctime(t);
		str_time[strlen(str_time) - 1] = 0;
		http_response_add_header(res, "Server", "AirTunes/220.68");
		http_response_add_header(res, "Date", str_time);
	}
	http_response_finish(res, responseBody, responseLength);

	if(DBG) ALOGD(" %s:%d   responseBody :::%s responseLength:::  %d\n", __func__, __LINE__, bin2hex((const unsigned char*)responseBody, responseLength),responseLength);
	*response = res;
	if(DBG) ALOGD("===================================================================================");
	if(DBG) ALOGD("-----------------------------------------------------------------------------------");



}



static void
conn_destroy(void *ptr)
{
	airplay_conn_t *conn = ptr;
	if (conn->airplay_rtp) {
		raop_rtp_destroy(conn->airplay_rtp);
	}
	free(conn->local);
	free(conn->remote);
	free(conn);
}

static void
conn_datafeed(void *ptr, unsigned char *data, int len)
{
	int size;
	unsigned short type;
	unsigned short type1;

	airplay_conn_t *conn = ptr;
	size = *(int*)data;
	type = *(unsigned short*)(data + 4);
	type1 = *(unsigned short*)(data + 6);

	if(DBG) ALOGD("Add data size=%d type %2x %2x", size, type, type1);
}

airplay_t *
airplay_init(int max_clients, raop_callbacks_t *callbacks, const char *pemkey, int *error)
{
	airplay_t *airplay;
	httpd_t *httpd;
	rsakey_t *rsakey;
	httpd_callbacks_t httpd_cbs;

	assert(callbacks);
	assert(max_clients > 0);
	assert(max_clients < 100);
	assert(pemkey);

	if (netutils_init() < 0) {
		return NULL;
	}

	if (!callbacks->audio_init||
		!callbacks->audio_process||
		!callbacks->audio_destroy)
	{
		return NULL;
	}

	airplay = calloc(1, sizeof(airplay_t));
	if (!airplay) {
		return NULL;
	}

	memset(&httpd_cbs, 0, sizeof(httpd_cbs));
	httpd_cbs.opaque = airplay;
	httpd_cbs.conn_init = &conn_init;
	httpd_cbs.conn_request = &conn_request;
	httpd_cbs.conn_destroy = &conn_destroy;
	httpd_cbs.conn_datafeed = &conn_datafeed;

	httpd = httpd_init(&httpd_cbs, max_clients, 0);
	if (!httpd) {
		free(airplay);
		return NULL;
	}
	airplay->main_server = httpd;

	httpd = httpd_init(&httpd_cbs, max_clients, 1);
	if (!httpd) {
		free(airplay->main_server);
		free(airplay);
		return NULL;
	}
	airplay->mirror_server = httpd;

	httpd = httpd_init(&httpd_cbs, max_clients, 2);
	if (!httpd) {
		free(airplay->mirror_server);
		free(airplay->main_server);
		free(airplay);
		return NULL;
	}
	airplay->event_server = httpd;

	memcpy(&airplay->callbacks, callbacks, sizeof(raop_callbacks_t));

	/* Initialize RSA key handler */
	rsakey = rsakey_init_pem(pemkey);
	if (!rsakey) {
		free(airplay->event_server);
		free(airplay->mirror_server);
		free(airplay->main_server);
		free(airplay);
		return NULL;
	}

	airplay->rsakey = rsakey;
	return airplay;
}

airplay_t *
airplay_init_from_keyfile(int max_clients, raop_callbacks_t *callbacks, const char *keyfile, int *error)
{
	airplay_t *airplay;
	char *pemstr;

	if (utils_read_file(&pemstr,keyfile) < 0) {
		return NULL;
	}
	airplay = airplay_init(max_clients, callbacks, pemstr, error);
	free(pemstr);
	return airplay;
}

void airpaly_destroy(airplay_t *airplay)
{
	ALOGE("%s(%d)", __func__, __LINE__);
	if (airplay) {
		airplay_stop(airplay);
		httpd_destroy(airplay->main_server);
		httpd_destroy(airplay->mirror_server);
		httpd_destroy(airplay->event_server);
		httpd_destroy(airplay->es1);
		httpd_destroy(airplay->es2);
		httpd_destroy(airplay->es3);
		rsakey_destroy(airplay->rsakey);
		free(airplay);
		netutils_cleanup();
	}
}

int
airplay_is_running(airplay_t *airplay)
{
	assert(airplay);
	return httpd_is_running(airplay->main_server);
}

int airplay_start(airplay_t *airplay, unsigned short *port, const char *hwaddr, int hwaddrlen, const char *password)
{
	int ret;
	unsigned short mirror_port = 53291, event_port = 55556, ep1, ep2, ep3;    //7100 -> 53291
	assert(airplay);
	assert(port);
	assert(hwaddr);
	int hw[6] = {0};
	int i;

	for(i = 0; i < 6; i++) hw[i] = hwaddr[i]&0xff;
	sprintf(deviceId, "%02X:%02X:%02X:%02X:%02X:%02X", hw[0], hw[1], hw[2], hw[3], hw[4], hw[5]);
	if(DBG) ALOGD("cz %s:%d deviceId=%s", __func__, __LINE__, deviceId);

	if (g_port_seted) {
		event_port = 55557;
		g_event_port = 55557;
		g_port_seted = 0;
	} else {
		event_port = 55556;
		g_event_port = 55556;
		g_port_seted = 1;
	}
	ep1 = 55558;
	ep2 = 55559;
	ep3 = 55560;

	if (hwaddrlen > MAX_HWADDR_LEN) {
		return -1;
	}

	memset(airplay->password, 0, sizeof(airplay->password));
	if (password) {
		if (strlen(password) > MAX_PASSWORD_LEN) {
			return -1;
		}
		strncpy(airplay->password, password, MAX_PASSWORD_LEN);
	}

	memcpy(airplay->hwaddr, hwaddr, hwaddrlen);
	airplay->hwaddrlen = hwaddrlen;

	ret = httpd_start(airplay->mirror_server, &mirror_port);
	if (ret < 0) return ret;
	ret = httpd_start(airplay->event_server, &event_port);
	if (ret < 0) return ret;
	// ret = httpd_start(airplay->es1, &ep1);
	// ret = httpd_start(airplay->es2, &ep2);
	// ret = httpd_start(airplay->es3, &ep3);
	ret = httpd_start(airplay->main_server, port);
	if (ret < 0) return ret;

	// ret = httpd_start(airplay->event_server, &event_port);
	//if (ret != 1) return ret;

	//ret = httpd_start(airplay->es1, &ep1);
	//if (ret != 1) return ret;

	//ret = httpd_start(airplay->es2, &ep2);
	//if (ret != 1) return ret;

	//ret = httpd_start(airplay->es3, &ep3);
	//if (ret != 1) return ret;

	return ret;//httpd_start(airplay->main_server, port);
}

void airplay_stop(airplay_t *airplay)
{
	assert(airplay);
	ALOGE("%s(%d)", __func__, __LINE__);
	httpd_stop(airplay->main_server);
	httpd_stop(airplay->mirror_server);
	httpd_stop(airplay->event_server);
}
