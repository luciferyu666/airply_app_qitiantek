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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "http_request.h"
#include "http_parser.h"
#include "android/log.h"
#define LOG_TAG "http_request"
#define DBG	0

struct http_request_s {
	http_parser parser;
	http_parser_settings parser_settings;

	const char *method;
	char *url;

	char **headers;
	int headers_size;
	int headers_index;

	char *data;
	int datalen;

	int complete;
};

static int
on_url(http_parser *parser, const char *at, size_t length)
{
	http_request_t *request = parser->data;
	int urllen = request->url ? strlen(request->url) : 0;

	if(urllen) {
		request->url = realloc(request->url, urllen+length+2);
		assert(request->url);
		request->url[urllen] = '\n';
		request->url[urllen+1] = '\0';
		strncat(request->url, at, length);
	} else {
		request->url = (char *)malloc(length+1);
		assert(request->url);
		memset(request->url, 0, length+1);
		strncpy(request->url, at, length);
	}
	return 0;
}

static int
on_header_field(http_parser *parser, const char *at, size_t length)
{
	http_request_t *request = parser->data;

	/* Check if our index is a value */
	if (request->headers_index%2 == 1) {
		request->headers_index++;
	}

	/* Allocate space for new field-value pair */
	if (request->headers_index == request->headers_size) {
		request->headers_size += 2;
		request->headers = realloc(request->headers,
		                           request->headers_size*sizeof(char*));
		assert(request->headers);
		request->headers[request->headers_index] = NULL;
		request->headers[request->headers_index+1] = NULL;
	}

	/* Allocate space in the current header string */
	if (request->headers[request->headers_index] == NULL) {
		request->headers[request->headers_index] = calloc(1, length+1);
	} else {
		request->headers[request->headers_index] = realloc(
			request->headers[request->headers_index],
			strlen(request->headers[request->headers_index])+length+1
		);
	}
	assert(request->headers[request->headers_index]);

	strncat(request->headers[request->headers_index], at, length);
	return 0;
}

static int
on_header_value(http_parser *parser, const char *at, size_t length)
{
	http_request_t *request = parser->data;

	/* Check if our index is a field */
	if (request->headers_index%2 == 0) {
		request->headers_index++;
	}

	/* Allocate space in the current header string */
	if (request->headers[request->headers_index] == NULL) {
		request->headers[request->headers_index] = calloc(1, length+1);
	} else {
		request->headers[request->headers_index] = realloc(
			request->headers[request->headers_index],
			strlen(request->headers[request->headers_index])+length+1
		);
	}
	assert(request->headers[request->headers_index]);

	strncat(request->headers[request->headers_index], at, length);
	return 0;
}

static int
on_body(http_parser *parser, const char *at, size_t length)
{
	http_request_t *request = parser->data;

	request->data = realloc(request->data, request->datalen+length);
	assert(request->data);

	memcpy(request->data+request->datalen, at, length);
	request->datalen += length;
	return 0;
}

static int
on_message_complete(http_parser *parser)
{
	http_request_t *request = parser->data;

	request->method = http_method_str(request->parser.method);
	request->complete = 1;
	return 0;
}

http_request_t *
http_request_init(void)
{
	http_request_t *request;

	request = calloc(1, sizeof(http_request_t));
	if (!request) {
		return NULL;
	}
	http_parser_init(&request->parser, HTTP_REQUEST);
	request->parser.data = request;

	request->parser_settings.on_url = &on_url;
	request->parser_settings.on_header_field = &on_header_field;
	request->parser_settings.on_header_value = &on_header_value;
	request->parser_settings.on_body = &on_body;
	request->parser_settings.on_message_complete = &on_message_complete;

	return request;
}

void
http_request_destroy(http_request_t *request)
{
	int i;

	if (request) {
		free(request->url);
		for (i=0; i<request->headers_size; i++) {
			free(request->headers[i]);
		}
		free(request->headers);
		free(request->data);
		free(request);
	}
}

int
http_request_add_data(http_request_t *request, const char *data, int datalen)
{
	int ret;

	assert(request);

	ret = http_parser_execute(&request->parser,
	                          &request->parser_settings,
	                          data, datalen);
	char *contentLengthStr = http_request_get_header(request, "Content-Length", 1);
	if(contentLengthStr)
		if(DBG) ALOGD("%s(%d) Content-Length=%s datalen=%d", __func__, __LINE__, contentLengthStr, request->datalen);
	return ret;
}

int
http_request_is_complete(http_request_t *request)
{
	assert(request);
	return request->complete;
}

int
http_request_has_error(http_request_t *request)
{
	assert(request);
	return (HTTP_PARSER_ERRNO(&request->parser) != HPE_OK);
}

const char *
http_request_get_error_name(http_request_t *request)
{
	assert(request);
	return http_errno_name(HTTP_PARSER_ERRNO(&request->parser));
}

const char *
http_request_get_error_description(http_request_t *request)
{
	assert(request);
	return http_errno_description(HTTP_PARSER_ERRNO(&request->parser));
}

const char *
http_request_get_method(http_request_t *request)
{
	assert(request);
	return request->method;
}

const char *
http_request_get_url(http_request_t *request, int index)
{
	assert(request);
	int urllen = strlen(request->url) + 1;
	char *url = (char *)malloc(urllen);
	strcpy(url, request->url);
	url[urllen-1] = 0;
	url = strtok(url, "\n");
	index--;
	for(; index>0; index--) {
		url = strtok(NULL, "\n");
	}

	return url;
}

const char *
http_request_get_header(http_request_t *request, const char *name, int index)
{
	int i;
	char *h_ret = NULL;

	assert(request);

	for (i=0; i<request->headers_size; i+=2) {
		if(index != 1) {
			if (!strcmp(request->headers[i], "CSeq")) {
				index--;
			}
		} else {
			if (!strcmp(request->headers[i], name)) {
				h_ret = request->headers[i+1];
				break;
			}
		}
	}
	return h_ret;
}

const char *
http_request_get_iheader(http_request_t *request, const char *name, int index)
{
	int i;
	char *h_ret = NULL;

	assert(request);

	for (i=0; i<request->headers_size; i+=2) {
		if(index != 1) {
			if (!strcasecmp(request->headers[i], "CSeq")) {
				index--;
			}
		} else {
			if (!strcasecmp(request->headers[i], name)) {
				h_ret = request->headers[i+1];
				break;
			}
		}
	}
	return h_ret;
}

const char *
http_request_get_data(http_request_t *request, int *datalen)
{
	assert(request);

	if (datalen) {
		*datalen = request->datalen;
	}
	return request->data;
}
void
http_request_dump_headers(http_request_t *request)
{
	int i;

	assert(request);

	for (i = 0; i<request->headers_size; i += 2) {
		if(DBG) ALOGD("cz %s:%s\n", request->headers[i], request->headers[i + 1]);
	}
}

int
http_request_get_request_count(http_request_t *request)
{
	int i, ret = 0;
	assert(request);

	for (i=0; i<request->headers_size; i+=2) {
		if (!strcasecmp(request->headers[i], "CSeq")) {
			ret++;
		}
	}
	ret = ret ? ret : 1;
	return ret;
}
