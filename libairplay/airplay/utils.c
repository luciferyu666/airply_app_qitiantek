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
#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sockets.h"

#include <sys/select.h>
#include <net/if_arp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <android/log.h>
#include "list.h"
#define LOG_TAG "utils"
#include "utils.h"





// fix build error
void android_set_abort_message(const char* msg) {}

int __system_property_foreach(void (*propfn)(const void *pi, void *cookie),void *cookie)
{
    return 0;//foreach_property(0, propfn, cookie);
}


void ALOGD_EX(char *tag, char *buffer, int len)
{
#undef LOG_TAG
#define LOG_TAG tag
	char line[1024];
	char *p_start, *p_end;
	p_start = buffer;
	while(p_start) {
		p_end = strchr(p_start, '\n');
		if(p_end == NULL) {
			strncpy(line, p_start, strlen(p_start) <= len ? strlen(p_start) : len);
			ALOGD("%s", p_start);
			break;
		}
		memset(line, 0, sizeof(line));
		if(len < p_end - p_start + 1)
			strncpy(line, p_start, len);
		else
			strncpy(line, p_start, p_end - p_start);
		ALOGD("%s", line);
		len -= (p_end - p_start + 1);
		if(len <= 0) break;
		p_start = p_end + 1;
	}
#undef LOG_TAG
#define LOG_TAG "utils"
}

struct hls_param_s {
	struct list_head i_list;
	char * param;
};


char* bin2hex(const unsigned char *buf, int len)
{
	int i;
	char *hex = (char*)malloc(len*2+1);
	for (i=0; i<len*2; i++) {
		int val = (i%2) ? buf[i/2]&0x0f : (buf[i/2]&0xf0)>>4;
		hex[i] = (val<10) ? '0'+val : 'a'+(val-10);
	}
	hex[len*2] = 0;
	return hex;
}


char *hex2bin(const unsigned char *buf, int len)
{
    int i;
    char *bin = (char*)malloc(len/2+1);
    
    unsigned char highByte, lowByte;
    
    for (i = 0; i < len; i += 2)
    {
        highByte = toupper(buf[i]);
        lowByte  = toupper(buf[i + 1]);
        
        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;
        
        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;
        
        bin[i / 2] = (highByte << 4) | lowByte;
    }
    bin[len/2] = 0;
    
    return bin;
}

void* memdup(void* src, int size) 
{
	void* ret = 0;
	if (src && size) {
		if (ret=calloc(1, size)){
			ret = memcpy(ret, src, size);
		} 
	}
	return ret;
}

char *
utils_strsep(char **stringp, const char *delim)
{
	char *original;
	char *strptr;

	if (*stringp == NULL) {
		return NULL;
	}

	original = *stringp;
	strptr = strstr(*stringp, delim);
	if (strptr == NULL) {
		*stringp = NULL;
		return original;
	}
	*strptr = '\0';
	*stringp = strptr+strlen(delim);
	return original;
}

int
utils_read_file(char **dst, const char *filename)
{
	FILE *stream;
	int filesize;
	char *buffer;
	int read_bytes;

	/* Open stream for reading */
	stream = fopen(filename, "rb");
	if (!stream) {
		return -1;
	}

	/* Find out file size */
	fseek(stream, 0, SEEK_END);
	filesize = ftell(stream);
	fseek(stream, 0, SEEK_SET);

	/* Allocate one extra byte for zero */
	buffer = malloc(filesize+1);
	if (!buffer) {
		fclose(stream);
		return -2;
	}

	/* Read data in a loop to buffer */
	read_bytes = 0;
	do {
		int ret = fread(buffer+read_bytes, 1,
		                filesize-read_bytes, stream);
		if (ret == 0) {
			break;
		}
		read_bytes += ret;
	} while (read_bytes < filesize);

	/* Add final null byte and close stream */
	buffer[read_bytes] = '\0';
	fclose(stream);

	/* If read didn't finish, return error */
	if (read_bytes != filesize) {
		free(buffer);
		return -3;
	}

	/* Return buffer */
	*dst = buffer;
	return filesize;
}

int
utils_hwaddr_raop(char *str, int strlen, const char *hwaddr, int hwaddrlen)
{
	int i,j;

	/* Check that our string is long enough */
	if (strlen == 0 || strlen < 2*hwaddrlen+1)
		return -1;

	/* Convert hardware address to hex string */
	for (i=0,j=0; i<hwaddrlen; i++) {
		int hi = (hwaddr[i]>>4) & 0x0f;
		int lo = hwaddr[i] & 0x0f;

		if (hi < 10) str[j++] = '0' + hi;
		else         str[j++] = 'A' + hi-10;
		if (lo < 10) str[j++] = '0' + lo;
		else         str[j++] = 'A' + lo-10;
	}

	/* Add string terminator */
	str[j++] = '\0';
	return j;
}

int
utils_hwaddr_airplay(char *str, int strlen, const char *hwaddr, int hwaddrlen)
{
	int i,j;

	/* Check that our string is long enough */
	if (strlen == 0 || strlen < 2*hwaddrlen+hwaddrlen)
		return -1;

	/* Convert hardware address to hex string */
	for (i=0,j=0; i<hwaddrlen; i++) {
		int hi = (hwaddr[i]>>4) & 0x0f;
		int lo = hwaddr[i] & 0x0f;

		if (hi < 10) str[j++] = '0' + hi;
		else         str[j++] = 'a' + hi-10;
		if (lo < 10) str[j++] = '0' + lo;
		else         str[j++] = 'a' + lo-10;

		str[j++] = ':';
	}

	/* Add string terminator */
	if (j != 0) j--;
	str[j++] = '\0';
	return j;
}

int recv_wait(int sockt, void *buf, signed int len)
{
	signed int want_read; // r6@1
	int ret; // r0@2
	size_t read_size; // r2@6
	struct timeval time_out; // [sp+8h] [bp-A0h]@3
	fd_set nfds; // [sp+10h] [bp-98h]@3
	int v10; // [sp+90h] [bp-18h]@3

	want_read = len;
	if ( sockt == -1 )
		return 0;
	time_out.tv_usec = 5000;
	time_out.tv_sec = 1;
	FD_ZERO(&nfds);
	FD_SET(sockt, &nfds);
	ret = select(sockt + 1, &nfds, 0, 0, &time_out);
	if ( ret >= 0 )
	{
		if ( !ret )
			return 0;
		if ( want_read >= 0x8000 )
			read_size = 0x8000;
		else
			read_size = want_read;
		ret = recv(sockt, (char*)buf, read_size, 0);
		if ( !ret )
			ret = -1;
	}
	return ret;
}

#define MAX_LINE 2048
int getValue(char *str, char *param, char *value) {
	if(str == NULL || param == NULL || value == NULL) return -1;
	char *p_start , *p_end;
	char flag = 0;
	int paramLen = strlen(param);
	if(paramLen == 0) return -1;
	p_start = strstr(str, param);
	if(!p_start) return 0;
	p_start += paramLen;
	while(*p_start == '=' || *p_start == '\"' || *p_start == '\'') {
		if(*p_start == '\"' || *p_start == '\'') flag = *p_start;
		p_start++;
	}
	p_end = p_start;
	flag = flag != 0 ? flag : ',';
	while(*p_end != flag) p_end++;
	if(!p_end) return 0;
	strncpy(value, p_start, p_end - p_start);
	return strlen(value);
}

char *get_line(char *line, int size, char *src) {
	char *p = src;
	while(--size && *p != '\0' && *p != '\r' && *p != '\n') p++;
	if(p == src) return NULL;
	strncpy(line, src, p - src);
	while(*p == '\r' || *p == '\n') p++;
	return p;
}

int trans(char *data, char *dst, int isMaster, char *g_id_list, unsigned int list_size) {
	int fd = -1;
	char line[MAX_LINE];
	char line2[MAX_LINE];
	char buf[MAX_LINE*2];
	char url[MAX_LINE] = {0};
	char *p, *p_next;
	int flag = 0;
	int ret = 0;
	fd = open(dst, O_WRONLY|O_CREAT|O_TRUNC, 0755);
	if(fd < 0) goto end;
	p_next = data;
	if(isMaster) {
		unsigned long bandwidth;
		unsigned int frameRate;
		unsigned int width;
		unsigned int height;
		char codecs[128] = {0};
		char audio[32] = {0};
		unsigned int g_id[32] = {0};
		unsigned char g_id_select[32] = {0};
		if(p = strstr(data, "RESOLUTION=1280x")) {
			getValue(p, "AUDIO", audio);
		} else if(p = strstr(data, "RESOLUTION=854x")) {
			getValue(p, "AUDIO", audio);
		} else if(p = strstr(data, "RESOLUTION=640x")) {
			getValue(p, "AUDIO", audio);
		} else if(p = strstr(data, "RESOLUTION=426x")) {
			getValue(p, "AUDIO", audio);
		} else if(p = strstr(data, "RESOLUTION=")) {
			getValue(p, "AUDIO", audio);
		} else {
			return 0;
		}
		p = strstr(p, "mlhls://localhost/itag/");
		if(p == NULL) return 0;
		strncpy(g_id_select, p + strlen("mlhls://localhost/itag/"), 31);
		p = strchr(g_id_select, '/');
		if(p != NULL) *p = 0;
		if(strlen(audio) == 0 || strlen(g_id_select) == 0) return 0;
		snprintf(g_id_list, list_size, "%s#%s", g_id_select, audio);

		while(1) {
#define LOCAL_URL "localhost/itag/%s_mediadata.m3u8"
			memset(line, 0, MAX_LINE);
			p_next = get_line(line, MAX_LINE, p_next);
			if(p_next == NULL) break;
			if(strlen(line) == 0) continue;
			ALOGD("line:%s\n", line);
			if(!strncmp("#EXT-X-STREAM-INF", line, strlen("#EXT-X-STREAM-INF"))) {
				p_next = get_line(line2, MAX_LINE, p_next);
				if(p_next == NULL) break;
				if(strlen(line2) == 0) continue;
				char str1[128] = {0};
				sprintf(str1, "mlhls://localhost/itag/%s/mediadata.m3u8", g_id_select);
				if(strncmp(line2, str1, strlen(str1))) continue;
				snprintf(buf, MAX_LINE*2, "%s\nhttp://"LOCAL_URL"\n", line, g_id_select);
				ALOGD("buf:%s", buf);
				write(fd, buf, strlen(buf));
			} else if(!strncmp("#EXT-X-MEDIA", line, strlen("#EXT-X-MEDIA"))) {
				char type[32] = {0};
				//TYPE=AUDIO
				if(getValue(line, "TYPE", type) <= 0) continue;
				if(!strncmp(type, "AUDIO", strlen("AUDIO"))) {
					if(getValue(line, "GROUP-ID", g_id) <= 0) continue;
					if(strncmp(g_id, audio, strlen(audio))) continue;
					p = strstr(line, "URI=\"mlhls://");
					if(p == NULL) continue;
					char str1[128] = {0};
					char str2[MAX_LINE] = {0};
					strncpy(str1, line, p - line);
					p = strstr(p, "m3u8\"");
					if(p == NULL) continue;
					strcpy(str2, p + strlen("m3u8\""));
					sprintf(buf, "%sURI=\"http://"LOCAL_URL"\"%s\n", str1, audio, str2);
					ALOGD("buf:%s", buf);
					write(fd, buf, strlen(buf));
				} else {
					snprintf(buf, MAX_LINE*2, "%s\n", line);
					ALOGD("buf:%s", buf);
					write(fd, buf, strlen(buf));
				}
			} else if(!strncmp("mlhls:", line, strlen("mlhls:"))) {
				continue;
			} else if(!strncmp("#", line, strlen("#"))){
				snprintf(buf, MAX_LINE*2, "%s\n", line);
				ALOGD("buf:%s", buf);
				write(fd, buf, strlen(buf));
			}
		}
	} else {
		char prefix[32] = {0};
		char params[128] = {0};
		struct list_head hlsParamList;
		INIT_LIST_HEAD(&hlsParamList);
		while(1) {
			memset(line, 0, MAX_LINE);
			p_next = get_line(line, MAX_LINE, p_next);
			if(p_next == NULL) break;
			if(strlen(line) == 0) continue;
			//ALOGD("line:%s\n", line);
			if(!strncmp("#YT-EXT-CONDENSED-URL", line, strlen("#YT-EXT-CONDENSED-URL"))) {
				if(getValue(line, "BASE-URI", url) <= 0) continue;
				ALOGD("url=%s\n", url);
				if(getValue(line, "PARAMS", params) <= 0) continue;
				ALOGD("params=%s\n", params);
				p = strtok(params, ",");
				int count = 0;
				while(p != NULL) {
					ALOGD("param[%d]=%s", count++, p);
					struct hls_param_s *hls_param = (struct hls_param_s *)malloc(sizeof(*hls_param));
					hls_param->param = (char *)malloc(strlen(p)+1);
					strcpy(hls_param->param, p);
					list_add_tail(&hls_param->i_list, &hlsParamList);
					p = strtok(NULL, ",");
				}
				if(getValue(line, "PREFIX", prefix) <= 0) continue;
				ALOGD("prefix=%s\n", prefix);
				flag = 1;
				continue;
			} else if(strlen(prefix) && !strncmp(prefix, line, strlen(prefix))) {
				p = strtok(line, "/");
				struct list_head * pList;
				struct list_head * pListTemp;
				memset(buf, 0, sizeof(buf));
				strcpy(buf, url);
				list_for_each_safe(pList, pListTemp, &hlsParamList) {
					if (pList == NULL) break;
					struct hls_param_s *hls_param = list_entry(pList, struct hls_param_s, i_list);
					if(hls_param == NULL) break;
					if (hls_param->param == NULL) break;
					p = strtok(NULL, "/");
					if(p == NULL) break;
					sprintf(buf, "%s/%s/%s", buf, hls_param->param, p);
				}
				sprintf(buf, "%s\n", buf);
				//ALOGD("%s(%d) %s", __func__, __LINE__, buf);
				write(fd, buf, strlen(buf));
			} else if(!strncmp("#", line, strlen("#"))){
				snprintf(buf, MAX_LINE*2, "%s\n", line);
				//ALOGD("%s", buf);
				write(fd, buf, strlen(buf));
			}
		}
		struct list_head * pList;
		struct list_head * pListTemp;
		list_for_each_safe(pList, pListTemp, &hlsParamList) {
			if (pList == NULL) break;
			struct hls_param_s *hls_param = list_entry(pList, struct hls_param_s, i_list);
			if(hls_param == NULL) break;
			if (hls_param->param) free(hls_param->param);
			free(hls_param);
		}
		//isMaster==0 && flag==0，认定为广告，放弃该次推送，重新请求master.m3u8
		if(flag == 0) ret = -1;
	}
end:
	ALOGD("%s(%d) ret=%d", __func__,__LINE__, ret);
	if(fd >= 0) close(fd);
	return ret;
}


uint64_t bytes_to_little_long(unsigned char *bb, int index)
{
	return ((((uint64_t) bb[index + 7] & 0xff) << 56)
			| (((uint64_t) bb[index + 6] & 0xff) << 48)
			| (((uint64_t) bb[index + 5] & 0xff) << 40)
			| (((uint64_t) bb[index + 4] & 0xff) << 32)
			| (((uint64_t) bb[index + 3] & 0xff) << 24)
			| (((uint64_t) bb[index + 2] & 0xff) << 16)
			| (((uint64_t) bb[index + 1] & 0xff) << 8)
			| (((long long int) bb[index + 0] & 0xff) << 0));
}

uint64_t bytes_to_bigger_long(unsigned char *bb, int index)
{
	return ((((uint64_t) bb[index + 0] & 0xff) << 56)
			| (((uint64_t) bb[index + 1] & 0xff) << 48)
			| (((uint64_t) bb[index + 2] & 0xff) << 40)
			| (((uint64_t) bb[index + 3] & 0xff) << 32)
			| (((uint64_t) bb[index + 4] & 0xff) << 24)
			| (((uint64_t) bb[index + 5] & 0xff) << 16)
			| (((uint64_t) bb[index + 6] & 0xff) << 8)
			| (((long long int) bb[index + 7] & 0xff) << 0));
}

uint64_t ntp_to_pts(long long int ntp)
{
	uint64_t m;
	uint64_t l;
	m = (ntp >> 32) * 1000000;
	l = (ntp & 0xffffffffl);
	l = (l * 1000 / 4294967296l) * 1000;
	m = m + l;
	return m;
}

int64_t get_current_time_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void sleep_ms(int n)
{
    struct timeval delay;
    delay.tv_sec = n / 1000;
    delay.tv_usec = (n % 1000) * 1000;
    select(0, NULL, NULL, NULL, &delay);
}

