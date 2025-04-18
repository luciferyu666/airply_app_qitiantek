#ifndef __MEDIASERVER_H__
#define __MEDIASERVER_H__

#if defined (WIN32) && defined(DLL_EXPORT)
# define AIR_API __declspec(dllexport)
#else
# define AIR_API
#endif

#define MEDIA_RENDER_CTL_MSG_BASE                       		 (0x100)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_DURATION            (MEDIA_RENDER_CTL_MSG_BASE+0)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_POSITION            (MEDIA_RENDER_CTL_MSG_BASE+1)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_PLAYINGSTATE        (MEDIA_RENDER_CTL_MSG_BASE+2)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_MUTE                (MEDIA_RENDER_CTL_MSG_BASE+3)
#define MEDIA_RENDER_TOCONTRPOINT_SET_CACHEPOSITION             (MEDIA_RENDER_CTL_MSG_BASE+4)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_CACHE               (MEDIA_RENDER_CTL_MSG_BASE+5)

struct airplay_callbacks_s {
	void *cls;
	/* Compulsory callback functions */
	void(*AirPlayPlayback_Open) (void *cls, char *url, float fPosition, double dPosition);
	void(*AirPlayPlayback_Play) (void *cls);
	void(*AirPlayPlayback_Pause)(void *cls);
	void(*AirPlayPlayback_Stop) (void *cls);
	void(*AirPlayPlayback_Seek)(void *cls, long fPosition);
	void(*AirPlayPlayback_SetVolume)(void *cls, int volume);
	void(*AirPlayPlayback_ShowPhoto)(void *cls, unsigned char *data, long long size);
	long(*AirPlayPlayback_GetDuration)(void *cls);
	long(*AirPlayPlayback_GetPostion)(void *cls);
	int(*AirPlayPlayback_IsPlaying)(void *cls);
	int(*AirPlayPlayback_IsPaused)(void *cls);

	void(*AirPlayAudio_Init)(void *cls, int bits, int channels, int samplerate, int isaudio, const uint8_t* remote);
	void(*AirPlayAudio_Process)(void *cls, const void *buffer, int buflen, uint64_t timestamp, uint32_t seqnum, const uint8_t* remote);
	void(*AirPlayAudio_destroy)(void *cls, const uint8_t* remote);
	void(*AirPlayAudio_SetVolume)(void *cls, int volume, const uint8_t* remote);//1-100
	void(*AirPlayAudio_SetMetadata) (void *cls, const void *buffer, int buflen);
	void(*AirPlayAudio_SetCoverart)(void *cls, const void *buffer, int buflen);
	void(*AirPlayAudio_Flush)(void *cls, const uint8_t* remote);

	void(*AirPlayMirroring_Play)(void *cls, int width, int height, const void *buffer, int buflen, int payloadtype, uint64_t timestamp, const uint8_t* remote);
	void(*AirPlayMirroring_Process)(void *cls, const void *buffer, int buflen, int payloadtype, uint64_t timestamp, const uint8_t* remote);
	void(*AirPlayMirroring_Stop)(void *cls, const uint8_t* remote);
	void(*AirPlayMirroring_Live)(void *cls);
	int(*AirPlayMirroring_Running)(void *cls, const uint8_t* remote);
};
typedef struct airplay_callbacks_s airplay_callbacks_t;

#ifdef __cplusplus
extern "C" {
#endif

int AIR_API startMediaServer(unsigned short *raop_port, unsigned short *airplay_port, char *friendname, int width, int height, airplay_callbacks_t *cb, char*mac , int maxfps);
void AIR_API stopMediaServer();
void forceStopDlna(void);
void forceStopAirplay(int id);

#ifdef __cplusplus
}
#endif

#endif
