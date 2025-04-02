#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include <sys/utsname.h>
#include <pthread.h>

#define LOG_TAG "language.cgi"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "../wifi/list_network.h"
#include "../wifi/lollipop_netTools.h"
#include "../utility/string_ext.h"
#include "../utility/list.h"
#include "../wifi/operate.h"
#include "../socket_ipc/lollipop_socket_ipc.h"

void parse_ver(char *str, char * ver) {?
	int i=0;
	char * p = str;
	char *last;

	while(p != NULL) {
		if(i++ == 0) {
			p = strtok_r(p, "&", &last);
		} else {
			p = strtok_r(NULL, "&", &last);
		}

		if (p == NULL) break;
		if(str_startsWith(p, "ios_ver=")) {
			strcpy(ver, p+strlen("ios_ver="));
		}
	}

	ALOGD("*** parse: ver=%s", ver);
}
//===========================================
//return 0 -- ok
//return 1 -- err
//===========================================
int file_write_buf(char* fn, int offset, void* buf, int len)
{
	int fd = -1;
	int ret;
	
	if(!fn || !buf)
	{
		return 1;
	}

	fd = open(fn, O_CREAT | O_RDWR | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd < 0)
	{
		ALOGD("open err, xxxfile = %s\n", fn);
		return 1;
	}
	if(offset > 0)
	{
		ret = lseek(fd, offset, SEEK_SET);
		if(ret < 0)
		{
			ALOGD("xxxseek err!\n");
			close(fd);
			return 1;
		}
	}
	ret = write(fd, buf, len);
	if(ret != len)
	{
		ALOGD("xxxwrite err, ret=%d, len=%d\n", ret, len);
		close(fd);
		return 1;
	}

	close(fd);
	return 0;
}
int main(void)
{
	char * lenstr;
	char input[256];
	long len;
	char version[PROPERTY_VALUE_MAX] = {0};
	
	ALOGD("=== set IOS Version! ===\n");

	printf("Content-Type:text/html\n\n");

	printf("<TITLE>set password</TITLE>\n");
	lenstr = getenv("CONTENT_LENGTH");

	if(lenstr == NULL || sscanf(lenstr,"%ld",&len)!=1 ) {
		//printf("<P> wrong");
		ALOGD("read argument err!\n");
	} else {
		int i=0;
		fgets(input, len+1, stdin);
		ALOGD("recive post: %s\n", input);
		parse_ver(input, version);

		//设置 ios_ver 这个属性的值;
		//那么, airplay 启动的时候, 获取这个属性的值, 设置是否可以推送;

		char* fn = "/data/ios_ver.txt";
		//不用在判断与文件中存储的值是否一样, 直接更新;
		//这个IO操作不是很频繁;
		if(0 == memcmp(version, "IOS10", strlen("IOS10")))
		{
			file_write_buf(fn, 0, "IOS10", strlen("IOS10"));
		}
		else
		{
			file_write_buf(fn, 0, "IOS9", strlen("IOS9"));
		}
	
	}
	
	printf("<meta HTTP-EQUIV=refresh Content=\'0;url=settings.cgi\'>");
	return 0;
}
