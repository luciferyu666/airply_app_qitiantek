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

#ifndef UTILS_H
#define UTILS_H

#include <android/log.h>



#if 0 // enable log
#define ALOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#else
#define ALOGD(...) do{}while(0)
#define ALOGI(...) do{}while(0)
#define ALOGE(...) do{}while(0)
#define ALOGW(...) do{}while(0)
#endif

extern int dnsProxyResponderFd;


char* bin2hex(const unsigned char *buf, int len);
void* memdup(void* src, int size); 
char *utils_strsep(char **stringp, const char *delim);
int utils_read_file(char **dst, const char *pemstr);
int utils_hwaddr_raop(char *str, int strlen, const char *hwaddr, int hwaddrlen);
int utils_hwaddr_airplay(char *str, int strlen, const char *hwaddr, int hwaddrlen);
int recv_wait(int sockt, void *buf, signed int len);
int trans(char *data, char *dst, int isMaster, char *g_id_list, unsigned int list_size);
uint64_t bytes_to_little_long(unsigned char *bb, int index);
uint64_t bytes_to_bigger_long(unsigned char *bb, int index);
uint64_t ntp_to_pts(long long int ntp);
int64_t get_current_time_ms();
void sleep_ms(int n);

char* bin2hex(const unsigned char *buf, int len);
char *hex2bin(const unsigned char *buf, int len);
void ALOGD_EX(char *tag, char *buffer, int len);

#endif
