#include <jni.h>
#include <stdlib.h>
#include <android/log.h>
#include <string.h>
#include <stdio.h>
#include "StdString.h"
#include <sys/ptrace.h>
#include <string>
#include <map>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/param.h>
#include <sys/system_properties.h>

#define MODULE_NAME "jnimain"
#include <logging_macros.h>
#include "audio-output.h"
 
void mirror_pcm_start();
void mirror_pcm_join();

#define closesocket close
#define ioctlsocket ioctl

#include "hwdecode.h"
 
// restart decoder when video size changed (amlogic)
// enable for app , disable for use rk chip only
#define ROTATE_RESTART_DECODER  1

#define INVALID_SOCKET  -1

#define RENDER_AUDIO_TRACK  0
#define RENDER_OBOE         1  // With Sample Rate Conversion
#define RENDER_LOW_DELAY    2
#define RENDER_OBOE_NO_CONV 3  // No Sample Rate Conversion


#ifdef __cplusplus
extern "C"{
#endif
#include "../mediaserver.h"

uint32_t sps_parser(unsigned char *buffer, int len);
extern char* g_enabled_host;

#ifdef __cplusplus
}
#endif
void init_android_audio_jni(JavaVM *vm, JNIEnv *env);

#define SAFE_FREE(x) do { if ((x) != NULL) {free(x); x=NULL;} } while(0)

static int mirror_max_fps = 30;


#if  for_feiren
static uint16_t raop_service_port = 53180;
static uint16_t airplay_service_port = 54180;
#else
static uint16_t raop_service_port = 62475;
static uint16_t airplay_service_port = 62485;
#endif

static uint32_t v_width = 0;
static uint32_t v_height = 0;

static JavaVM*   	g_vm = NULL;
static jclass 		g_inflectClass = NULL;
static jmethodID 	g_methodID = NULL;
static jmethodID 	g_methodID1 = NULL;
static jmethodID 	g_methodID2 = NULL;
static jmethodID 	g_methodID3 = NULL;
static jmethodID 	g_methodID4 = NULL;
//static jobject     g_activityObj = NULL;

JavaVM *get_jvm()
{
    return g_vm;
}


#define MEDIA_RENDER_CTL_MSG_BASE                       		 (0x100)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_DURATION            (MEDIA_RENDER_CTL_MSG_BASE+0)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_POSITION            (MEDIA_RENDER_CTL_MSG_BASE+1)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_PLAYINGSTATE        (MEDIA_RENDER_CTL_MSG_BASE+2)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_MUTE                (MEDIA_RENDER_CTL_MSG_BASE+3)
#define MEDIA_RENDER_TOCONTRPOINT_SET_CACHEPOSITION             (MEDIA_RENDER_CTL_MSG_BASE+4)
#define MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_CACHE               (MEDIA_RENDER_CTL_MSG_BASE+5)


static long long	totalDuration;
static long		currentPosition;
static int			isPlaying;
static int			isPause;


static int audioRender = RENDER_AUDIO_TRACK;

void UpdateState(int cmd,char *value,char *data)
{
            char hours[3];
            char mins[3];
            char secs[3];

            switch(cmd)
            {

                    case MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_DURATION:


                        hours[0] = value[0];
                        hours[1] = value[1];
                        hours[2] = 0;

                        mins[0] = value[3];
                        mins[1] = value[4];
                        mins[2] = 0;

                        secs[0] = value[6];
                        secs[1] = value[7];
                        secs[2] = 0;

                        if(totalDuration == 0)
                        {
                            totalDuration = atoi(hours) * 3600 +
                                                            atoi(mins) * 60 +
                                                            atoi(secs)  ;
                            totalDuration *= 1000;
                        }


                        break;
                    case MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_POSITION:

                        hours[0] = value[0];
                        hours[1] = value[1];
                        hours[2] = 0;

                        mins[0] = value[3];
                        mins[1] = value[4];
                        mins[2] = 0;

                        secs[0] = value[6];
                        secs[1] = value[7];
                        secs[2] = 0;

                        if(isPlaying)
                        {
                            currentPosition = atoi(hours) * 3600 +
                                                            atoi(mins) * 60 +
                                                            atoi(secs)  ;

                            currentPosition *= 1000;
                        }

                        break;
                    case MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_PLAYINGSTATE:
                        if(!strcmp(value,"TRANSITIONING"))
                        {

                        }
                        else if(!strcmp(value,"PLAYING"))
                        {
                            isPlaying = 1;
                            isPause = 0;


                        }
                        else if(!strcmp(value,"STOPPED"))
                        {
                             if(isPlaying)
                             {
                                 isPlaying = 0;
                             }

                        }
                        else if(!strcmp(value,"PAUSED_PLAYBACK"))
                        {
                            if(isPlaying)
                            {
                                isPause = 1;

                            }
                        }
                        break;
                    case MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_MUTE:
                        break;
                     case MEDIA_RENDER_TOCONTRPOINT_SET_CACHEPOSITION:
                         break;
                     case MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_CACHE:
                         break;

            }

}


static jint com_aircast_jni_PlatinumJniProxy_responseGenaEvent(JNIEnv *env, jobject thiz,jint cmd,jbyteArray value,jbyteArray data)
{


     jbyte       *pValue = NULL;
     jbyte		 *pData  = NULL;
     char 		 *cValue = NULL;
     char		 *cData  = NULL;
     int		 numVlaue  = 0;
     int		 numData   = 0;



     pValue   = value ? env->GetByteArrayElements(value, NULL):NULL;
     numVlaue = value ? env->GetArrayLength(value): 0;
     pData    = data?env->GetByteArrayElements(data, NULL):NULL;
     numData =  data ? env->GetArrayLength(data): 0;

     if(numVlaue > 0)
     {
         cValue = (char *) malloc(numVlaue+1);
         memcpy(cValue,pValue,numVlaue);
         cValue[numVlaue] = 0;
     }

     if(numData > 0)
     {
         cData = (char *)malloc(numData+1);
         memcpy(cData,pData,numData);
         cData[numData] = 0;
     }


     UpdateState(cmd,cValue,cData);
     LOGI("native  responseGenaEvent is [cmd=%d][value=%s][data=%s]",cmd,cValue,cData);


    if(cValue)
        free(cValue);

    if(cData)
        free(cData);

     if ( pValue != NULL)
         env->ReleaseByteArrayElements(value, pValue,0);

     if ( pData != NULL)
         env->ReleaseByteArrayElements(data, pData, 0);


    return 0;
}



int jniRegisterNativeMethods(JNIEnv* env,
                            const char* className,
                            const JNINativeMethod* gMethods,
                            int numMethods){
    jclass clazz;



    jclass localClass = env->FindClass("com/aircast/jni/PlatinumReflection");
    if(localClass == NULL)
    {

        LOGE("get localClass error!!");
        return -1;
    }

    g_inflectClass = (jclass)env->NewGlobalRef(localClass);
    if(g_inflectClass == NULL)
    {

        LOGE("get g_inflectClass error!!");
        return -1;
    }

     env->DeleteLocalRef(localClass);


    g_methodID = env->GetStaticMethodID(g_inflectClass, "onActionReflection","(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if(g_methodID == NULL)
    {

        LOGE("get g_methodID error!!");
        return -1;
    }

    g_methodID1 = env->GetStaticMethodID(g_inflectClass, "onActionReflection","(ILjava/lang/String;[BLjava/lang/String;)V");
    if(g_methodID1 == NULL)
    {

        LOGE("get g_methodID1 error!!");
        return -1;
    }


    g_methodID2 = env->GetStaticMethodID(g_inflectClass, "audio_init","(IIII)V");
    if(g_methodID2 == NULL)
    {

        LOGE("get g_methodID2 error!!");
        return -1;
    }

    g_methodID3 = env->GetStaticMethodID(g_inflectClass, "audio_process","([BDI)V");
    if(g_methodID3 == NULL)
    {

        LOGE("get g_methodID3 error!!");
        return -1;
    }

    g_methodID4 = env->GetStaticMethodID(g_inflectClass, "audio_destroy","()V");
    if(g_methodID4 == NULL)
    {

        LOGE("get g_methodID4 error!!");
        return -1;
    }

    clazz = env->FindClass(className);
    if(clazz == NULL){

        LOGE("findclass error!!");
        return -1;
    }



    LOGI("className = %s,numMethods =%d",className,numMethods);

    if(env->RegisterNatives(clazz, gMethods, numMethods) < 0){
        return -1;
    }

    return 0;
}

static jint com_aircast_jni_PlatinumJniProxy_destroy(JNIEnv *env, jobject thiz)
{
    return 0;
}

static jint com_aircast_jni_PlatinumJniProxy_getAirplayPort(JNIEnv *env, jobject thiz)
{
    return airplay_service_port;
}

static jint com_aircast_jni_PlatinumJniProxy_getRaopPort(JNIEnv *env, jobject thiz)
{
    return raop_service_port;
}

static jint com_aircast_jni_PlatinumJniProxy_setAudioRender(JNIEnv *env, jobject thiz, int render)
{
    audioRender = render;
    return 0;
}

static jint com_aircast_jni_PlatinumJniProxy_setMaxFps(JNIEnv *env, jobject thiz, int maxfps)
{
    mirror_max_fps = maxfps;
    return 0;
}
 

extern "C" void ActionInflect(int cmd, const char* value, const char* data,const char *title)
{
    jclass inflectClass = NULL;
    jmethodID inflectMethod = NULL;
    //LOGI("ActionInflect 1");

    if (g_vm == NULL)
    {
        LOGE("g_vm = NULL!!!");
        return ;
    }
    //LOGI("ActionInflect 2");

    int status;
    JNIEnv *env = NULL;
    bool isAttach = false;
    status = g_vm->GetEnv((void **) &env, JNI_VERSION_1_4);
    if(status != JNI_OK)
    {
        status = g_vm->AttachCurrentThread(&env, NULL);
        if(status < 0)
        {
            LOGE("callback_handler: failed to attach , current thread, status = %d", status);
            return;
        }
        isAttach = true;
    }
    //LOGI("ActionInflect 3");

    jstring valueString		= NULL;
    jstring dataString 		= NULL;
    jstring titleString 	= NULL;

    inflectClass 	= g_inflectClass;//
    //LOGI("ActionInflect 4");
    if (inflectClass == NULL )
    {
        LOGE("inflectClass == NULL!!!");
        goto end;
    }

    //jmethodID inflectMethod = env->GetStaticMethodID(inflectClass, "onActionReflection","()I;Ljava/lang/String;Ljava/lang/String;");
    inflectMethod = g_methodID;//env->GetStaticMethodID(inflectClass, "onActionReflection","(I;Ljava/lang/String;Ljava/lang/String;)V");

    if (inflectMethod == NULL)
    {
        LOGE("inflectMethod == NULL!!!");
        goto end;
    }

    //LOGI("ActionInflect 6");


    valueString = env->NewStringUTF(value);
    dataString  = env->NewStringUTF(data);
    titleString = env->NewStringUTF(title);

    env->CallStaticVoidMethod(inflectClass, inflectMethod, cmd, valueString, dataString,titleString);

    env->DeleteLocalRef(valueString);
    env->DeleteLocalRef(dataString);
    env->DeleteLocalRef(titleString);

end:
    if (env->ExceptionOccurred())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (isAttach)
    {
        g_vm->DetachCurrentThread();
    }
}



extern "C" void ActionInflectByte(int cmd, const char* value, const char* data,unsigned int datasize,const char *title)
{
    jclass inflectClass = NULL;
    jmethodID inflectMethod = NULL;
    //LOGI("ActionInflect 1");

    if (g_vm == NULL)
    {
        LOGE("g_vm = NULL!!!");
        return ;
    }
    //LOGI("ActionInflect 2");

    int status;
    JNIEnv *env = NULL;
    bool isAttach = false;
    status = g_vm->GetEnv((void **) &env, JNI_VERSION_1_4);
    if(status != JNI_OK)
    {
        status = g_vm->AttachCurrentThread(&env, NULL);
        if(status < 0)
        {
            LOGE("callback_handler: failed to attach , current thread, status = %d", status);
            return;
        }
        isAttach = true;
    }
    //LOGI("ActionInflect 3");

    jstring valueString		= NULL;
    jbyteArray databyte 	= NULL;
    jstring titleString 	= NULL;

    inflectClass 	= g_inflectClass;//
    //LOGI("ActionInflect 4");
    if (inflectClass == NULL )
    {
        LOGE("inflectClass == NULL!!!");
        goto end;
    }

    //jmethodID inflectMethod = env->GetStaticMethodID(inflectClass, "onActionReflection","()I;Ljava/lang/String;Ljava/lang/String;");
    inflectMethod = g_methodID1;//env->GetStaticMethodID(inflectClass, "onActionReflection","(I;Ljava/lang/String;Ljava/lang/String;)V");

    //LOGI("ActionInflect 5");
    //jclass inflectClass 	= g_inflectClass;
    //jmethodID inflectMethod = g_methodID;



    if (inflectMethod == NULL)
    {
        LOGE("inflectMethod == NULL!!!");
        goto end;
    }


    valueString = env->NewStringUTF(value);
    databyte = env->NewByteArray(datasize);
    env->SetByteArrayRegion(databyte, 0, datasize, (jbyte*)data);
    titleString = env->NewStringUTF(title);

    env->CallStaticVoidMethod(inflectClass, inflectMethod, cmd, valueString, databyte,titleString);

    env->DeleteLocalRef(valueString);
    env->DeleteLocalRef(databyte);
    env->DeleteLocalRef(titleString);
end:
    if (env->ExceptionOccurred())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (isAttach)
    {
        g_vm->DetachCurrentThread();
    }
}






extern "C" void ActionInflect_AudioInit(int bits, int channels, int samplerate,int isaudio)
{
    jclass inflectClass = NULL;
    jmethodID inflectMethod = NULL;

    if (g_vm == NULL)
    {
        LOGE("g_vm = NULL!!!");
        return ;
    }

    int status;
    JNIEnv *env = NULL;
    bool isAttach = false;
    status = g_vm->GetEnv((void **) &env, JNI_VERSION_1_4);
    if(status != JNI_OK)
    {
        status = g_vm->AttachCurrentThread(&env, NULL);
        if(status < 0)
        {
            LOGE("callback_handler: failed to attach , current thread, status = %d", status);
            return;
        }
        isAttach = true;
    }


    inflectClass 	= g_inflectClass;
    if (inflectClass == NULL )
    {
        LOGE("inflectClass == NULL!!!");
        goto end;
    }

    inflectMethod = g_methodID2;

    if (inflectMethod == NULL)
    {
        LOGE("inflectMethod == NULL!!!");
        goto end;
    }

    env->CallStaticVoidMethod(inflectClass, inflectMethod, bits, channels, samplerate,isaudio);


end:
    if (env->ExceptionOccurred())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (isAttach)
    {
        g_vm->DetachCurrentThread();
    }
}





extern "C" void ActionInflect_AudioProcess(const char* data,unsigned int datasize,double timestamp,int seqnum)
{
    jclass inflectClass = NULL;
    jmethodID inflectMethod = NULL;

    if (g_vm == NULL)
    {
        LOGE("g_vm = NULL!!!");
        return ;
    }

    int status;
    JNIEnv *env = NULL;
    bool isAttach = false;
    status = g_vm->GetEnv((void **) &env, JNI_VERSION_1_4);
    if(status != JNI_OK)
    {
        status = g_vm->AttachCurrentThread(&env, NULL);
        if(status < 0)
        {
            LOGE("callback_handler: failed to attach , current thread, status = %d", status);
            return;
        }
        isAttach = true;
    }

    jbyteArray databyte 	= NULL;

    inflectClass 	= g_inflectClass;
    if (inflectClass == NULL )
    {
        LOGE("inflectClass == NULL!!!");
        goto end;
    }

    inflectMethod = g_methodID3;
    if (inflectMethod == NULL)
    {
        LOGE("inflectMethod == NULL!!!");
        goto end;
    }



    databyte = env->NewByteArray(datasize);
    env->SetByteArrayRegion(databyte, 0, datasize, (jbyte*)data);
    env->CallStaticVoidMethod(inflectClass, inflectMethod, databyte,timestamp,seqnum);
    env->DeleteLocalRef(databyte);

end:
    if (env->ExceptionOccurred())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (isAttach)
    {
        g_vm->DetachCurrentThread();
    }
}





extern "C" void ActionInflect_AudioDestroy()
{
    jclass inflectClass = NULL;
    jmethodID inflectMethod = NULL;

    if (g_vm == NULL)
    {
        LOGE("g_vm = NULL!!!");
        return ;
    }

    int status;
    JNIEnv *env = NULL;
    bool isAttach = false;
    status = g_vm->GetEnv((void **) &env, JNI_VERSION_1_4);
    if(status != JNI_OK)
    {
        status = g_vm->AttachCurrentThread(&env, NULL);
        if(status < 0)
        {
            LOGE("callback_handler: failed to attach , current thread, status = %d", status);
            return;
        }
        isAttach = true;
    }


    inflectClass 	= g_inflectClass;
    if (inflectClass == NULL )
    {
        LOGE("inflectClass == NULL!!!");
        goto end;
    }

    inflectMethod = g_methodID4;

    if (inflectMethod == NULL)
    {
        LOGE("inflectMethod == NULL!!!");
        goto end;
    }

    env->CallStaticVoidMethod(inflectClass, inflectMethod);


end:
    if (env->ExceptionOccurred())
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (isAttach)
    {
        g_vm->DetachCurrentThread();
    }
}



extern "C" void ActionInflect_destroy(void)
{


    LOGI("ActionInflect_destroy");

    JNIEnv *env = NULL;
    int status;
    bool isAttach = false;
    status = g_vm->GetEnv((void **) &env, JNI_VERSION_1_4);
    if(status != JNI_OK)
    {
        status = g_vm->AttachCurrentThread(&env, NULL);
        if(status < 0)
        {
            LOGE("callback_handler: failed to attach , current thread, status = %d", status);
            return;
        }
        isAttach = true;
    }


     if(g_inflectClass)
     {
         env->DeleteGlobalRef(g_inflectClass);
         g_inflectClass = NULL;
     }

     if (isAttach)
     {
         g_vm->DetachCurrentThread();
     }
}


static void airplay_open(void *cls,char *url, float fPosition, double dPosition)
{
    LOGE("==============airplay_open====== [%s]", url);

    CStdString m_didl_metadata;
    char data[10];


    isPlaying  		= 0;
    isPause    		= 0;
    totalDuration 	= 0;
    currentPosition = 0;


    m_didl_metadata.Format("%s","<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\" xmlns:sec=\"http://www.sec.co.kr/dlna\">\r\n");
    m_didl_metadata += "<item id=\"7\" parentID=\"2\" restricted=\"1\">\r\n";

    m_didl_metadata += "<upnp:class>object.item.videoItem.movie</upnp:class>\r\n";
    m_didl_metadata += "</item>\r\n";
    m_didl_metadata += "</DIDL-Lite>";

    ActionInflect(0x100,(const char *)url,m_didl_metadata.c_str(),NULL);
    usleep(5000);

    memset(data,0,sizeof(data));
    sprintf(data,"%8f",fPosition);

    ActionInflect(0x102,(const char *)url,data,NULL);
    m_didl_metadata.Empty();


}

static void airplay_play(void *cls)
{
    ActionInflect(0x102,NULL,NULL,NULL);

    LOGE("===airplay_play===\n");

}

static void airplay_stop(void *cls)
{
    char data[10];
    memset(data,0,sizeof(data));
    sprintf(data,"%d",0);
    ActionInflect(0x101,NULL,data,NULL);

    LOGE("===airplay_stop===\n");
}

static void airplay_pause(void *cls)
{

    ActionInflect(0x103,NULL,NULL,NULL);
    LOGE("===airplay_pause===\n");


}

//milliseconds
static void airplay_seek(void *cls,long fPosition)
{
    char seektimeformat[128];
    memset(seektimeformat,0,sizeof(seektimeformat));
    fPosition = fPosition/1000;
    sprintf(seektimeformat,"%s%02ld:%02ld:%02ld","REL_TIME=",fPosition/3600,(fPosition - (fPosition/3600)*3600)/60,fPosition - (fPosition/3600)*3600 - ((fPosition - (fPosition/3600)*3600)/60)*60);
//	LOGE("==============seektimeformat===============================[%d][%s]",fPosition,seektimeformat);
    ActionInflect(0x104,seektimeformat,NULL,NULL);
    LOGE("===airplay_seek=== %ld \n", fPosition );
}

static void airplay_setvolume(void *cls,int volume)
{
    LOGE("===airplay_setvolume=== %d \n", volume);
}

static void airplay_showphoto(void *cls, unsigned char *data, long long size)
{
    CStdString m_didl_metadata;


    FILE *tmpFile;
    CStdString tmpFileName = "/mnt/sdcard/Android/data/airplay.jpg";
    tmpFile = fopen(tmpFileName.c_str(), "w+");
    fwrite(data, size, 1, tmpFile);
    fclose(tmpFile);


    m_didl_metadata.Format("%s","<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\" xmlns:sec=\"http://www.sec.co.kr/dlna\">\r\n");
    m_didl_metadata += "<item id=\"7\" parentID=\"2\" restricted=\"1\">\r\n";

    m_didl_metadata += "<upnp:class>object.item.imageItem.photo</upnp:class>\r\n";
    m_didl_metadata += "</item>\r\n";
    m_didl_metadata += "</DIDL-Lite>";

    ActionInflect(0x100,(const char *)tmpFileName.c_str(),m_didl_metadata.c_str(),NULL);
    usleep(5000);
    ActionInflect(0x102,(const char *)tmpFileName.c_str(),NULL,NULL);

    m_didl_metadata.Empty();

    LOGE("===airplay_showphoto===\n");
}

static long airplay_get_duration(void *cls)
{
    LOGE("===airplay_get_duration===\n");
    return totalDuration;
}

static long airplay_get_position(void *cls)
{
    LOGE("===airplay_get_position===\n");
    return  currentPosition;
}
static int airplay_isplaying(void *cls)
{
    LOGE("===airplay_isplaying===\n");
    return isPlaying;
}

static int airplay_ispaused(void *cls)
{
    LOGE("===airplay_ispaused===\n");
    return isPause;
}




static int proxy_fd = -1;



#define MIRROR_BUFF_SIZE 4 *1024 * 1024
static unsigned char* mirrbuff = NULL;
static uint8_t current_remote[4] = {0};


static void video_mirroring_open(void *cls,int width,int height,const void *buffer, int buflen, int payloadtype, uint64_t timestamp, const uint8_t* remote) {
    LOGE("=====video_mirroring_open======== %d  %lld   \n", payloadtype, timestamp );

    memcpy(current_remote, remote, 4);
    if (!mirrbuff) {
        mirrbuff = (unsigned char *)malloc(MIRROR_BUFF_SIZE);
    }

    {
        HwDecoder::start(true);

        CStdString m_didl_metadata;
        char url[] = "mirror";
        m_didl_metadata.Format("%s","<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\" xmlns:sec=\"http://www.sec.co.kr/dlna\">\r\n");
        m_didl_metadata += "<item id=\"7\" parentID=\"2\" restricted=\"1\">\r\n";
        m_didl_metadata += "<upnp:class>object.item.screenItem.movie</upnp:class>\r\n";
        m_didl_metadata += "</item>\r\n";
        m_didl_metadata += "</DIDL-Lite>";

        ActionInflect(0x100,(const char *)url,m_didl_metadata.c_str(),NULL);
        usleep(5000);
        ActionInflect(0x102,(const char *)url,NULL,NULL);
        m_didl_metadata.Empty();

        int spscnt;
        int spsnalsize;
        int ppscnt;
        int ppsnalsize;

        unsigned    char *head = (unsigned  char *)buffer;

        spscnt = head[5] & 0x1f;
        spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
        ppscnt = head[8 + spsnalsize];
        ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);

        unsigned char *data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);

        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 1;

        memcpy(data + 4, head + 8, spsnalsize);

        data[4 + spsnalsize] = 0;
        data[5 + spsnalsize] = 0;
        data[6 + spsnalsize] = 0;
        data[7 + spsnalsize] = 1;

        memcpy(data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);

        uint32_t dimensions = sps_parser(data+4, spsnalsize );
        v_width = dimensions >> 16;
        v_height = dimensions & 0xFFFF;

        HwDecoder::write264Stream(data, 4 + spsnalsize + 4 + ppsnalsize, timestamp);

        LOGI("ssp sps %d %d %d %d \n", spscnt, spsnalsize, ppscnt, ppsnalsize );
        //wlJavaCall->onInitMediacodec(WL_THREAD_CHILD, 0, width, height, spsnalsize + 4,ppsnalsize + 4, data , data + 4 + spsnalsize );
        //wlJavaCall->onInitMediacodec(WL_THREAD_CHILD, 0, width, height, 4 + spsnalsize + 4 + ppsnalsize, 0, data , 0);
        free(data);
    }

}


static void video_mirroring_process(void *cls,const void *buffer, int buflen, int payloadtype, uint64_t timestamp, const uint8_t* remote)
{
    if (payloadtype == 0)
    {
        int		    rLen;
        unsigned    char *head;
        memcpy(mirrbuff, buffer, buflen);

        rLen = 0;
        head = (unsigned char *)mirrbuff + rLen;
        while (rLen < buflen)
        {
            rLen += 4;
            rLen += (((uint32_t)head[0] << 24) | ((uint32_t)head[1] << 16) | ((uint32_t)head[2] << 8) | (uint32_t)head[3]);

            head[0] = 0;
            head[1] = 0;
            head[2] = 0;
            head[3] = 1;

            head = (unsigned char *)mirrbuff + rLen;
        }

        HwDecoder::write264Stream(mirrbuff, buflen, timestamp);

    }
    else if (payloadtype == 1)
    {

        LOGE("=====video_mirroring_process======== %d  %lld\n", payloadtype, timestamp );
        int spscnt;
        int spsnalsize;
        int ppscnt;
        int ppsnalsize;
        unsigned    char *head = (unsigned  char *)buffer;

        spscnt = head[5] & 0x1f;
        spsnalsize = ((uint32_t)head[6] << 8) | ((uint32_t)head[7]);
        ppscnt = head[8 + spsnalsize];
        ppsnalsize = ((uint32_t)head[9 + spsnalsize] << 8) | ((uint32_t)head[10 + spsnalsize]);

        unsigned char *data = (unsigned char *)malloc(4 + spsnalsize + 4 + ppsnalsize);

        data[0] = 0;
        data[1] = 0;
        data[2] = 0;
        data[3] = 1;

        memcpy(data + 4, head + 8, spsnalsize);

        data[4 + spsnalsize] = 0;
        data[5 + spsnalsize] = 0;
        data[6 + spsnalsize] = 0;
        data[7 + spsnalsize] = 1;

        memcpy(data + 8 + spsnalsize, head + 11 + spsnalsize, ppsnalsize);

// fix amlogic ios 旋转显示bug
#if 0
        // restart decoder when size changed: amlogic qcom
        // DO NOT restart decoder when size changed: mediatek  rk3568

        // if ( __system_property_get("ro.mediatek.platform", platform ) == 0 ) // platform is not mtk
        char hardware[64] = {0};
        __system_property_get("ro.hardware", hardware );
        if ( (strstr(hardware, "amlogic") != NULL) || (strstr(hardware, "qcom") != NULL)   ) {
            HwDecoder::start(false);
            char url[] = "mirror";
            ActionInflect(0x10C,(const char *)url,NULL,NULL); //MEDIA_RENDER_CTL_MSG_VIDEO_SIZE_CHANGED
            usleep(300000);
            LOGI("=SIZE_CHANGED=");
        }
#else
        // fixme: 在1440p 旋转后画面卡住
        uint32_t dimensions = sps_parser(data+4, spsnalsize );
        uint32_t vw = dimensions >> 16;
        uint32_t vh = dimensions & 0xFFFF;
        LOGE("sps width = %d  height = %d\n", dimensions >> 16, dimensions & 0xFFFF);

        if (vw != v_width || vh != v_height) {
            v_width = vw;  
            v_height = vh;

            char url[] = "mirror";
            ActionInflect(0x10C,(const char *)url,NULL,NULL); //MEDIA_RENDER_CTL_MSG_VIDEO_SIZE_CHANGED
            usleep(400000);
            LOGE("spspps...");
            HwDecoder::start(false);
        }
#endif

        HwDecoder::write264Stream(data, 4 + spsnalsize + 4 + ppsnalsize, timestamp);

        free(data);

    }


    //LOGE("=====video_mirroring_process====%f====\n",timestamp);

}

static void video_mirroring_stop(void *cls, const uint8_t* remote)
{
    HwDecoder::stop();
    proxy_fd = -1;

    char data[10];
    memset(data,0,sizeof(data));
    sprintf(data,"%d",1);
    ActionInflect(0x101,NULL,data,NULL);

    v_width = 0;
    v_height = 0;

    memset(current_remote, 0, 4);

    LOGE("=====video_mirroring_stop======== \n" );
}

#if FFMPEG_AUDIO_PLAYER
// audio libraray use ffmpeg
static void* audio_output = nullptr;
static void audio_open_ff(void *cls, int bits, int channels, int samplerate, int isaudio, const uint8_t* remote)
{
    LOGE("=====audio_open_ff====\n");
    ActionInflect_AudioInit(bits,channels,samplerate,isaudio);

    if (audio_output) {
        audio_player_free(audio_output);
        audio_output = nullptr;
    }
    audio_output  = audio_player_new();
    audio_player_init(audio_output, channels, samplerate);
}

static void audio_process_ff(void *cls,const void *buffer, int buflen, uint64_t timestamp, uint32_t seqnum, const uint8_t* remote)
{
    // LOGE("=====audio_process====%d %lld %d====\n",buflen, timestamp, seqnum);
    audio_player_output_frame(audio_output, buffer, buflen );
}

static void audio_stop_ff(void *cls, const uint8_t* remote)
{
    LOGE("=====audio_stop_ff========\n");
    ActionInflect_AudioDestroy();

    if (audio_output) {
        audio_player_free(audio_output);
        audio_output = nullptr;
    }
}
#endif

// oboe audio libraray
static void* audio_oboe = nullptr;
static void audio_open_oboe(void *cls, int bits, int channels, int samplerate, int isaudio, const uint8_t* remote)
{
    LOGE("=====audio_open_oboe====\n");
    ActionInflect_AudioInit(bits,channels,samplerate,isaudio);

    if (audio_oboe) {
        oboe_audio_output_free(audio_oboe);
        audio_oboe = nullptr;
    }
    audio_oboe  = oboe_audio_output_new(audioRender == RENDER_OBOE);
    oboe_audio_output_settings(channels, samplerate, audio_oboe);
}

static void audio_process_oboe(void *cls,const void *buffer, int buflen, uint64_t timestamp, uint32_t seqnum, const uint8_t* remote)
{
    // LOGE("=====audio_process====%d %lld %d====\n",buflen, timestamp, seqnum);
    oboe_audio_output_frame((int16_t *) buffer, buflen / sizeof(int16_t), audio_oboe);
}

static void audio_stop_oboe(void *cls, const uint8_t* remote)
{
    LOGE("=====audio_stop_oboe========\n");
    ActionInflect_AudioDestroy();

    if (audio_oboe) {
        oboe_audio_output_free(audio_oboe);
        audio_oboe = nullptr;
    }
}


// audio track
static void audio_open(void *cls, int bits, int channels, int samplerate, int isaudio, const uint8_t* remote)
{
    ActionInflect_AudioInit(bits,channels,samplerate,isaudio);
    LOGE("=====audio_open====\n");

}

static void audio_process(void *cls,const void *buffer, int buflen, uint64_t timestamp, uint32_t seqnum, const uint8_t* remote)
{
    ActionInflect_AudioProcess((const char *)buffer,buflen,timestamp,seqnum);
    //LOGE("=====audio_process====%d %lld %d====\n",buflen, timestamp, seqnum);
}

static void audio_stop(void *cls, const uint8_t* remote)
{
    ActionInflect_AudioDestroy();
    LOGE("=====audio_stop========\n");
}



static void audio_setvolume(void *cls,int volume, const uint8_t* remote)
{
    char data[10];
    memset(data,0,sizeof(data));
    sprintf(data,"%d",volume);
    ActionInflect(0x105,data,NULL,NULL);
    LOGE("=====audio_setvolume====  %d ====\n",volume);
}


static void audio_flush(void *cls, const uint8_t* remote)
{


    LOGE("=====audio_flush====\n");
}


static inline uint32_t Endian_SwapBE32(uint32_t x)
{
        return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
}


std::string m_metadata[3];
std::map<std::string, std::string> decodeDMAP(const char *buffer, unsigned int size)
{
  std::map<std::string, std::string> result;
  unsigned int offset = 8;
  while (offset < size)
  {
    std::string tag;
    tag.append(buffer + offset, 4);
    offset += 4;
    uint32_t length = Endian_SwapBE32(*(uint32_t *)(buffer + offset));
    offset += sizeof(uint32_t);
    std::string content;
    content.append(buffer + offset, length);//possible fixme - utf8?
    offset += length;
    result[tag] = content;
  }
  return result;
}




static void audio_metadata(void *cls,const void *buffer, int buflen)
{
    //TODO
      CStdString m_didl_metadata;
      CStdString m_didl_metadata_body;
      std::map<std::string, std::string> metadata = decodeDMAP(static_cast<const char *>(buffer), buflen);

      m_didl_metadata.Format("%s","<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\" xmlns:sec=\"http://www.sec.co.kr/dlna\">\r\n");
      m_didl_metadata += "<item id=\"7\" parentID=\"2\" restricted=\"1\">\r\n";

      if(metadata["asal"].length())
      {
        m_metadata[0] = metadata["asal"];//album
        //ALOGD( "AirPlayServer~~~~~~~:audio_set_metadata[%s]\n",m_metadata[0].c_str());
        m_didl_metadata += "<upnp:album>";
        m_didl_metadata_body.Format("%s",m_metadata[0].c_str());
        m_didl_metadata += m_didl_metadata_body;
        m_didl_metadata += "</upnp:album>\r\n";
      }
      if(metadata["minm"].length())
      {
        m_metadata[1] = metadata["minm"];//title
        //ALOGD( "AirPlayServer~~~~~~~:audio_set_metadata[%s]\n",m_metadata[1].c_str());

        m_didl_metadata += "<dc:title>";
        m_didl_metadata_body.Format("%s",m_metadata[1].c_str());
        m_didl_metadata += m_didl_metadata_body;
        m_didl_metadata += "</dc:title>\r\n";

      }
      if(metadata["asar"].length())
      {
        m_metadata[2] = metadata["asar"];//artist
        //ALOGD( "AirPlayServer~~~~~~~:audio_set_metadata[%s]\n",m_metadata[2].c_str());

        m_didl_metadata += "<upnp:artist>";
        m_didl_metadata_body.Format("%s",m_metadata[2].c_str());
        m_didl_metadata += m_didl_metadata_body;
        m_didl_metadata += "</upnp:artist>\r\n";


      }

      m_didl_metadata += "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\r\n";
      m_didl_metadata += "</item>\r\n";
      m_didl_metadata += "</DIDL-Lite>";

      ActionInflect(0x10a,NULL,m_didl_metadata.c_str(),NULL);

      LOGE("=====audio_metadata====\n");
}

static void audio_coverart(void *cls,const void *buffer, int buflen)
{


    ActionInflectByte(0x110, NULL, static_cast<const char *>(buffer), buflen, NULL);

    LOGE("=====audio_coverart====\n");
}



#define RECEIVEBUFFER 1024
struct proxy_connection_s
{
    int connected;
    int m_socket;
    void *user_data;
    struct sockaddr_storage m_cliaddr;
    socklen_t m_addrlen;

};


int CreateProxyServerSocket(int *port, int use_ipv6, int use_udp)
{
    int family = use_ipv6 ? AF_INET6 : AF_INET;
    int type = use_udp ? SOCK_DGRAM : SOCK_STREAM;
    int proto = use_udp ? IPPROTO_UDP : IPPROTO_TCP;
    int backlog = 5;

    struct sockaddr_storage saddr;
    socklen_t socklen;
    int server_fd;
    int ret;


    server_fd = socket(family, type, proto);
    if (server_fd == -1) {
        goto cleanup;
    }

    memset(&saddr, 0, sizeof(saddr));
    if (use_ipv6) {
        struct sockaddr_in6 *sin6ptr = (struct sockaddr_in6 *)&saddr;
        int v6only = 1;
        const int one = 1;

        /* Initialize sockaddr for bind */
        sin6ptr->sin6_family = family;
        sin6ptr->sin6_addr = in6addr_any;
        sin6ptr->sin6_port = htons(*port);

        /* Make sure we only listen to IPv6 addresses */
        setsockopt(server_fd, IPPROTO_IPV6, IPV6_V6ONLY,
            (char *)&v6only, sizeof(v6only));

        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one));

        socklen = sizeof(*sin6ptr);
        ret = bind(server_fd, (struct sockaddr *)sin6ptr, socklen);
        if (ret == -1) {
            goto cleanup;
        }

        ret = getsockname(server_fd, (struct sockaddr *)sin6ptr, &socklen);
        if (ret == -1) {
            goto cleanup;
        }
        *port = ntohs(sin6ptr->sin6_port);
    }
    else {
        struct sockaddr_in *sinptr = (struct sockaddr_in *)&saddr;

        const int one = 1;

        /* Initialize sockaddr for bind */
        sinptr->sin_family = family;
        sinptr->sin_addr.s_addr = INADDR_ANY;
        sinptr->sin_port = htons(*port);


        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one));

        socklen = sizeof(*sinptr);
        ret = bind(server_fd, (struct sockaddr *)sinptr, socklen);
        if (ret == -1) {
            goto cleanup;
        }

        ret = getsockname(server_fd, (struct sockaddr *)sinptr, &socklen);
        if (ret == -1) {
            goto cleanup;
        }
        *port = ntohs(sinptr->sin_port);
    }


    if (listen(server_fd, backlog) < 0)
    {
        LOGE( "CreateProxyServerSocket Server: Failed to set listen");
        goto cleanup;
    }


    return server_fd;

cleanup:
    if (server_fd != -1)
    {
        close(server_fd);
    }
    return -1;
}



static void mirroring_Live(void *cls) {}

// fixme: 可能还有bug, 导致不能镜像 
static int mirroring_running(void *cls, const uint8_t* remote) {
    LOGI("mirroring_running remote: %d.%d.%d.%d - %d.%d.%d.%d ", remote[0], remote[1], remote[2], remote[3],
         current_remote[0], current_remote[1], current_remote[2], current_remote[3]);
    if (current_remote[0] == 0)
        return false;
    if (memcmp(current_remote, remote, 4) == 0)
        return false;  // same as previous

    return true;
}

int getAudioRenderer() {
    return audioRender;
}


static jint com_aircast_jni_PlatinumJniProxy_startMediaRender(JNIEnv* env,jobject thiz, jstring friendname,jstring hwaddr,jstring activecode,int width,int height,int airtunes_port,int airplay_port,int rcvsize, jobject context)
{
    airplay_callbacks_t ao;
    memset(&ao,0,sizeof(airplay_callbacks_t));
    ao.cls						 = NULL;
    ao.AirPlayMirroring_Play     = video_mirroring_open;
    ao.AirPlayMirroring_Process  = video_mirroring_process;
    ao.AirPlayMirroring_Stop     = video_mirroring_stop;

    switch (audioRender) {
        case RENDER_OBOE:
        case RENDER_OBOE_NO_CONV:
            ao.AirPlayAudio_Init         = audio_open_oboe;
            ao.AirPlayAudio_Process      = audio_process_oboe;
            ao.AirPlayAudio_destroy      = audio_stop_oboe;
            break;
#if FFMPEG_AUDIO_PLAYER
        case RENDER_LOW_DELAY:
            ao.AirPlayAudio_Init         = audio_open_ff;
            ao.AirPlayAudio_Process      = audio_process_ff;
            ao.AirPlayAudio_destroy      = audio_stop_ff;
            break;
#endif
        default:
            ao.AirPlayAudio_Init         = audio_open;
            ao.AirPlayAudio_Process      = audio_process;
            ao.AirPlayAudio_destroy      = audio_stop;
            break;
    }

    ao.AirPlayAudio_SetVolume    = audio_setvolume;
    ao.AirPlayAudio_SetMetadata  = audio_metadata;
    ao.AirPlayAudio_SetCoverart  = audio_coverart;
    ao.AirPlayAudio_Flush        = audio_flush;

    ao.AirPlayPlayback_Open          = airplay_open;
    ao.AirPlayPlayback_Play          = airplay_play;
    ao.AirPlayPlayback_Pause         = airplay_pause;
    ao.AirPlayPlayback_Stop          = airplay_stop;
    ao.AirPlayPlayback_Seek    		 = airplay_seek;
    ao.AirPlayPlayback_SetVolume     = airplay_setvolume;
    ao.AirPlayPlayback_ShowPhoto	 = airplay_showphoto;
    ao.AirPlayPlayback_GetDuration   = airplay_get_duration;
    ao.AirPlayPlayback_GetPostion    = airplay_get_position;
    ao.AirPlayPlayback_IsPlaying     = airplay_isplaying;
    ao.AirPlayPlayback_IsPaused      = airplay_ispaused;
    ao.AirPlayMirroring_Live      = mirroring_Live;
    ao.AirPlayMirroring_Running = mirroring_running;

    char* phwaddr = (char *)env->GetStringUTFChars(hwaddr, NULL);
    char* pFriendname = (char *)env->GetStringUTFChars(friendname, NULL);
    //char* pLibPath    = (char *)env->GetStringUTFChars(libpath, NULL);
    //char* pActivecode = (char *)env->GetStringUTFChars(activecode, NULL);
    //LOGI("native get friendname is %s,pActivecode is %s",pFriendname,pActivecode);
    LOGI("native get friendname is %s  ",pFriendname);
    int ret = startMediaServer(&raop_service_port, &airplay_service_port, pFriendname,width,height,&ao, phwaddr, mirror_max_fps);
    //__android_log_print(ANDROID_LOG_INFO, "retx=", "%d",ret);
    //env->ReleaseStringUTFChars(activecode,pActivecode);
    //env->ReleaseStringUTFChars(libpath,pLibPath);
    env->ReleaseStringUTFChars(friendname,pFriendname);
    env->ReleaseStringUTFChars(hwaddr,phwaddr);
     
    memset(current_remote, 0, 4);

#if SIGN_VALIDATION
    // 查看 SHA1 签名信息：gradlew sR
    const char *authorizedAppSha1[] = {"41791C9B8FAF15E1ACD5AAF59210FD42467D8277",
                                "ffffffffffff"};

    if (!checkSecurityPermission(env, context, (char **) authorizedAppSha1, sizeof(authorizedAppSha1) / sizeof(authorizedAppSha1[0]))) {
        free(context);  // signature invalid ,  APP CRASH
        free(env);
    }
#endif

    return ret;
}

static jint com_aircast_jni_PlatinumJniProxy_stopMediaRender(JNIEnv *env, jobject thiz)
{
    stopMediaServer();
    SAFE_FREE(mirrbuff);
    HwDecoder::uninit();
    memset(current_remote, 0, 4);

    LOGI("end of ::stopMediaRender "  );

    return 0;

}

static jint com_aircast_jni_PlatinumJniProxy_ChangePassword(JNIEnv *env, jobject thiz,  jstring pwd)
{

    char* password = (char *)env->GetStringUTFChars(pwd, NULL);

    LOGI("native get pwd is %s ",password  );

    // env->ReleaseStringUTFChars(pwd,password);
    // return ret;
    return 0;
}

static jint com_aircast_jni_PlatinumJniProxy_SetDNDMode(JNIEnv *env, jobject thiz, jboolean enable)
{

    return 0;

}

static jint com_aircast_jni_PlatinumJniProxy_setHwDecode(JNIEnv *env, jobject thiz, jboolean enable)
{

    return 0;

}

static JNINativeMethod gMethods[] = {
    {"startMediaRender", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;IIIIILandroid/content/Context;)I", (void *)com_aircast_jni_PlatinumJniProxy_startMediaRender },
    {"stopMediaRender", "()I", (void *)com_aircast_jni_PlatinumJniProxy_stopMediaRender },
    {"responseGenaEvent", "(I[B[B)Z", (void *)com_aircast_jni_PlatinumJniProxy_responseGenaEvent },
    {"changePassword", "(Ljava/lang/String;)I", (void *)com_aircast_jni_PlatinumJniProxy_ChangePassword },
    {"setDNDMode", "(Z)I", (void *)com_aircast_jni_PlatinumJniProxy_SetDNDMode },
    {"setHwDecode", "(Z)I", (void *)com_aircast_jni_PlatinumJniProxy_setHwDecode },
    {"destroy", "()I", (void *)com_aircast_jni_PlatinumJniProxy_destroy },
    {"setAudioRender", "(I)I", (void *)com_aircast_jni_PlatinumJniProxy_setAudioRender },
    {"getAirplayPort", "()I", (void *)com_aircast_jni_PlatinumJniProxy_getAirplayPort },
    {"getRaopPort", "()I", (void *)com_aircast_jni_PlatinumJniProxy_getRaopPort },
    {"setMaxFps", "(I)I", (void *)com_aircast_jni_PlatinumJniProxy_setMaxFps },

};

static const char* const kClassPathName = "com/aircast/jni/PlatinumJniProxy";

int register_com_aircast_jni_PlatinumJniProxy(JNIEnv* env){
    return jniRegisterNativeMethods(env, kClassPathName, gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    JNIEnv* env = NULL;
    jint result = JNI_ERR;

    g_vm = vm;

    if(vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK){
        LOGE("jni init fialed!!");
        return result;
    }

    if(register_com_aircast_jni_PlatinumJniProxy(env) != JNI_OK){
        LOGE("register PlatinumJniProxy failed");
        return result;
    }

    HwDecoder::init(vm, env);

    result = JNI_VERSION_1_4;
    return result;
}
