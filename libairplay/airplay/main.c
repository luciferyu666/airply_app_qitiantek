#define _CRT_SECURE_NO_WARNINGS
#include "dnssd.h"
#include "raop.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "airplay.h"
#include "mediaserver.h"
//#include "../libcutils/loghack.h"
#include "utils.h"
#define LOG_TAG "airplay_main"
#define DBG	1


typedef struct {
	char apname[56];
	char password[56];
	unsigned short port_raop;
	unsigned short port_airplay;
	char hwaddr[6];

	char ao_driver[56];
	char ao_devicename[56];
	char ao_deviceid[16];
	int	 enable_airplay;
} shairpaly_options_t;

static int running;



#include  <signal.h>


const char* g_pem_key = 
	" -----BEGIN RSA PRIVATE KEY-----MIIEpQIBAAKCAQEA59dE8qLieItsH1WgjrcFRKj6eUWqi+bGLOX1HL3U3GhC/j0Qg90u3sG/1CUtw"
	"C5vOYvfDmFI6oSFXi5ELabWJmT2dKHzBJKa3k9ok+8t9ucRqMd6DZHJ2YCCLlDRKSKv6kDqnw4UwPdpOMXziC/AMj3Z/lUVX1G7WSHCAWKf1z"
	"NS1eLvqr+boEjXuBOitnZ/bDzPHrTOZz0Dew0uowxf/+sG+NCK3eQJVxqcaJ/vEHKIVd2M+5qL71yJQ+87X6oV3eaYvt3zWZYD6z5vYTcrtij"
	"2VZ9Zmni/UAaHqn9JdsBWLUEpVviYnhimNVvYFZeCXg/IdTQ+x4IRdiXNv5hEewIDAQABAoIBAQDl8Axy9XfWBLmkzkEiqoSwF0PsmVrPzH9K"
	"snwLGH+QZlvjWd8SWYGN7u1507HvhF5N3drJoVU3O14nDY4TFQAaLlJ9VM35AApXaLyY1ERrN7u9ALKd2LUwYhM7Km539O4yUFYikE2nIPscE"
	"sA5ltpxOgUGCY7b7ez5NtD6nL1ZKauw7aNXmVAvmJTcuPxWmoktF3gDJKK2wxZuNGcJE0uFQEG4Z3BrWP7yoNuSK3dii2jmlpPHr0O/KnPQtz"
	"I3eguhe0TwUem/eYSdyzMyVx/YpwkzwtYL3sR5k0o9rKQLtvLzfAqdBxBurcizaaA/L0HIgAmOit1GJA2saMxTVPNhAoGBAPfgv1oeZxgxmot"
	"iCcMXFEQEWflzhWYTsXrhUIuz5jFua39GLS99ZEErhLdrwj8rDDViRVJ5skOp9zFvlYAHs0xh92ji1E7V/ysnKBfsMrPkk5KSKPrnjndMoPde"
	"vWnVkgJ5jxFuNgxkOLMuG9i53B4yMvDTCRiIPMQ++N2iLDaRAoGBAO9v//mU8eVkQaoANf0ZoMjW8CN4xwWA2cSEIHkd9AfFkftuv8oyLDCG3"
	"ZAf0vrhrrtkrfa7ef+AUb69DNggq4mHQAYBp7L+k5DKzJrKuO0r+R0YbY9pZD1+/g9dVt91d6LQNepUE/yY2PP5CNoFmjedpLHMOPFdVgqDzD"
	"FxU8hLAoGBANDrr7xAJbqBjHVwIzQ4To9pb4BNeqDndk5Qe7fT3+/H1njGaC0/rXE0Qb7q5ySgnsCb3DvAcJyRM9SJ7OKlGt0FMSdJD5KG0XP"
	"IpAVNwgpXXH5MDJg09KHeh0kXo+QA6viFBi21y340NonnEfdf54PX4ZGS/Xac1UK+pLkBB+zRAoGAf0AY3H3qKS2lMEI4bzEFoHeK3G895pDa"
	"K3TFBVmD7fV0Zhov17fegFPMwOII8MisYm9ZfT2Z0s5Ro3s5rkt+nvLAdfC/PYPKzTLalpGSwomSNYJcB9HNMlmhkGzc1JnLYT4iyUyx6pcZB"
	"mCd8bD0iwY/FzcgNDaUmbX9+XDvRA0CgYEAkE7pIPlE71qvfJQgoA9em0gILAuE4Pu13aKiJnfft7hIjbK+5kyb3TysZvoyDnb3HOKvInK7vX"
	"bKuU4ISgxB2bB3HcYzQMGsz1qJ2gG0N5hvJpzwwhbhXqFKA4zaaSrw622wDniAK5MlIE0tIAKKP4yxNGjoD2QYjhBGuhvkWKY=-----END RS"
	"A PRIVATE KEY-----";

shairpaly_options_t options;
dnssd_t *dnssd = NULL;
raop_t *raop = NULL;
raop_callbacks_t raop_cbs;
airplay_t *airplay = NULL;
const char default_hwaddr[] = { 0x00, 0x24, 0xd7, 0xb2, 0x2e, 0x60 };
char *password = NULL;

int getWifiMac(char * mac) {
	int ret;
	char buffer[32] = {0};
	FILE *fp = fopen("/sys/class/net/wlan0/address", "r");
	if (mac == NULL) return -1;
	if (fp == NULL) return -1;
	fread(buffer, 1, 32, fp);
	fclose(fp);
	buffer[strlen(buffer)-1] = '\0';
	if(DBG) ALOGD("/sys/class/net/wlan0/address=%s", buffer);
	ret = sscanf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
			&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	if(ret == 6)
		return 0;
	else
		return -1;
}

int destoryMdns(void) {
	//__system_property_set("ctl.stop", "mdnsd");
	return 0;
}
int initMdns(void) {
	//destoryMdns();
	//__system_property_set("ctl.start", "mdnsd");
	return 0;
}

/*
int destoryMdns(void)
{
        int pid;
        char value[256] = {0};
        property_get("pid.mdnsd", value, NULL);
        pid = atoi(value);
        if(pid > 0)
        {
                kill(value, SIGTERM);
                property_set("pid.mdnsd", "0");
#if 0
                LOG("=== kill mdnsd, pid = %d ===\n", pid);
#endif
        }
        return 0;
}

int initMdns(void)
{
        int pid;
        char value[256] = {0};
        //================
        //ÏÈÍ£Ö¹ mdnsd ·þÎñ;
        //================
        destoryMdns();
        //================
        //´´½¨×Ó½ø³Ì, ÓÉ×Ó½ø³ÌÀ´Æô¶¯ mdnsd ·þÎñ;
        //================
        pid = fork();
        if(pid < 0)
        {
                ALOGD("ERR, fork err!\n");
                exit(0);
        }
        else if(pid != 0)
        {
                sprintf(value, "%d", pid);
                property_set("pid.mdnsd", value);
                //LOG("mdnsd pid = %s\n", value);
                sleep(1);       //µÈ´ýmdnsdÏÈÆô¶¯;
        }
        else
        {
#if 0
                char *str[2];
                str[0] = "/airplay/bin/mdnsd";
                str[1] = NULL;
                if (execv(str[0], str) < 0 ){
                        ALOGD("cz %s:%d mdnsd error", __func__, __LINE__);
                }
                exit(0);
#else
                char* cmd = "/system/bin/setprop ctl.start mdnsd";
                system(cmd);
#if 1
                ALOGD("exec cmd = %s\n", cmd);
#endif
                exit(0);        //×Ó½ø³ÌÆô¶¯ÁË mdnsd Ö®ºóÍË³öÔËÐÐ;
#endif          
        }
        return 0;
}
*/
// #define PORT_RAOP	53180	         //5000
// #define PORT_AIRPLAY	54180		//7000
int startMediaServer(unsigned short *raop_port, unsigned short *airplay_port, char *friendname, int width, int height, airplay_callbacks_t *cb, char* defHwaddr, int maxfps)
{
	int i, error, ret;
	char hwaddr[6] = {0};

	initMdns();
	memset(&options, 0, sizeof(options));
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);
	strncpy(options.apname, friendname, sizeof(options.apname) - 1);
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);
	
	if(getWifiMac(options.hwaddr)) {
        int ssret = sscanf(defHwaddr, "%02x:%02x:%02x:%02x:%02x:%02x",
            &options.hwaddr[0], &options.hwaddr[1], &options.hwaddr[2], &options.hwaddr[3], &options.hwaddr[4], &options.hwaddr[5]);
	    if(ssret != 6)
		    memcpy(options.hwaddr, default_hwaddr, sizeof(options.hwaddr)); 

		//memcpy(options.hwaddr, default_hwaddr, sizeof(options.hwaddr));
		ALOGD("func %s line %d  %s \n", __FUNCTION__,__LINE__, defHwaddr);
	}
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);
	
	options.enable_airplay = 1;
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);
	
	//raop
	memset(&raop_cbs, 0, sizeof(raop_cbs));
	raop_cbs.cls = cb->cls;

	raop_cbs.audio_init = cb->AirPlayAudio_Init;
	raop_cbs.audio_process = cb->AirPlayAudio_Process;
	raop_cbs.audio_destroy = cb->AirPlayAudio_destroy;
	raop_cbs.audio_set_volume = cb->AirPlayAudio_SetVolume;
	raop_cbs.audio_set_metadata = cb->AirPlayAudio_SetMetadata;
	raop_cbs.audio_set_coverart = cb->AirPlayAudio_SetCoverart;
	raop_cbs.audio_flush = cb->AirPlayAudio_Flush;

	raop_cbs.mirroring_play = cb->AirPlayMirroring_Play;
	raop_cbs.mirroring_process = cb->AirPlayMirroring_Process;
	raop_cbs.mirroring_stop = cb->AirPlayMirroring_Stop;
	raop_cbs.mirroring_live = cb->AirPlayMirroring_Live;
	raop_cbs.mirroring_running = cb->AirPlayMirroring_Running;
	
	raop_cbs.AirPlayPlayback_Open          = cb->AirPlayPlayback_Open;
    raop_cbs.AirPlayPlayback_Play          = cb->AirPlayPlayback_Play;
    raop_cbs.AirPlayPlayback_Pause         = cb->AirPlayPlayback_Pause;
    raop_cbs.AirPlayPlayback_Stop          = cb->AirPlayPlayback_Stop;
    raop_cbs.AirPlayPlayback_Seek    		 = cb->AirPlayPlayback_Seek;
    raop_cbs.AirPlayPlayback_SetVolume     = cb->AirPlayPlayback_SetVolume;
	raop_cbs.AirPlayPlayback_ShowPhoto	 = cb->AirPlayPlayback_ShowPhoto;
	raop_cbs.AirPlayPlayback_GetDuration   = cb->AirPlayPlayback_GetDuration;
	raop_cbs.AirPlayPlayback_GetPostion    = cb->AirPlayPlayback_GetPostion;
	raop_cbs.AirPlayPlayback_IsPlaying     = cb->AirPlayPlayback_IsPlaying;
	raop_cbs.AirPlayPlayback_IsPaused      = cb->AirPlayPlayback_IsPaused;
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);
	
	options.port_raop = *raop_port;
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);
	raop = raop_init(10, &raop_cbs, g_pem_key, NULL);
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);
	for(i = 0, ret = -1; i < 10 && ret < 0; i++) {
		ret = raop_start(raop, &options.port_raop, options.hwaddr, sizeof(options.hwaddr), password, width, height, maxfps );
	}
	if(ret < 0) return ret;

	//airplay
	options.port_airplay = *airplay_port;
	airplay = airplay_init(10, &raop_cbs, g_pem_key, NULL);
	for(i = 0, ret = -1; i < 10 && ret < 0; i++) {
		ret = airplay_start(airplay, &options.port_airplay, options.hwaddr, sizeof(options.hwaddr), password);
	}
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);
	if(ret < 0) return ret;


	//dnssd
	error = 0;
	dnssd = dnssd_init(&error);
	ALOGD("func %s line %d\n", __FUNCTION__,__LINE__);

	if (error) {
		if(DBG) ALOGD("%s:%d dnssd_init %d", error);
		raop_destroy(raop);
		//airplay_destroy(airplay);
		return -1;
	}

#if EMBEDDED_MDNS
	int raopinit = dnssd_register_raop(dnssd, options.apname, options.port_raop, options.hwaddr, sizeof(options.hwaddr), password);
	if(DBG) ALOGD("raopinit %d",raopinit);
	if(raopinit) return -1;
	int airplayinit = dnssd_register_airplay(dnssd, options.apname, options.port_airplay, options.hwaddr, sizeof(options.hwaddr));
	if(DBG) ALOGD("airplayinit %d",airplayinit);
	if(airplayinit) return -1;
#endif	

	*raop_port = options.port_raop ;
	*airplay_port = options.port_airplay ;
	return 0;
}

void stopMediaServer() {
	if(dnssd){
#if EMBEDDED_MDNS
		dnssd_unregister_raop(dnssd);
		dnssd_unregister_airplay(dnssd);
#endif
		dnssd_destroy(dnssd);
		destoryMdns();
                dnssd = NULL;
	}

	if(raop){
		raop_stop(raop);
		raop_destroy(raop);
                raop = NULL;
	}
	// audio_shutdown();
	if(airplay){
		ALOGE("%s(%d)", __func__, __LINE__);
		airplay_stop(airplay);
                airplay = NULL;
	}

	return;
}

