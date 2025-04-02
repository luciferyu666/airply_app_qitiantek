package com.aircast.center;

import android.content.Context;
import android.content.Intent;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.aircast.image.ImageActivity;
import com.aircast.jni.PlatinumJniProxy;
import com.aircast.jni.PlatinumReflection;
import com.aircast.jni.PlatinumReflection.ActionReflectionListener;
import com.aircast.mirror.MirrorActivity;
import com.aircast.music.MusicActivity;
import com.aircast.settings.Setting;
import com.aircast.source.Source;
import com.aircast.util.Action;
import com.aircast.util.CommonLog;
import com.aircast.util.CommonUtil;
import com.aircast.util.DlnaUtils;
import com.aircast.util.LogFactory;
import com.aircast.video.CicadaVideoPlayer;
import com.aircast.video.VideoActivity;

public class DMRCenter implements ActionReflectionListener, IDMRAction {
    public static final int CUR_MEDIA_TYPE_MUSCI = 0x0001;
    public static final int CUR_MEDIA_TYPE_VIDEO = 0x0000;
    public static final int CUR_MEDIA_TYPE_PICTURE = 0x0002;
    public static final int CUR_MEDIA_TYPE_SCREEN = 0x0003;//0x0003;
    private static final String TAG = "DMRCenter";
    private static final CommonLog log = LogFactory.createLog();
    private static final int DELAYTIME = 200;
    private static final int MSG_START_MUSICPLAY = 0x0001;
    private static final int MSG_START_PICPLAY = 0x0002;
    private static final int MSG_START_VIDOPLAY = 0x0003;
    private static final int MSG_SEND_STOPCMD = 0x0004;
    private static final int MSG_START_SCREENPLAY = 0x0005;
    public static AudioManager N;
    public static AudioTrack track = null;
    private Context mContext;
    private final Handler mHandler = new Handler() {

        @Override
        public void dispatchMessage(Message msg) {

            try {
                switch (msg.what) {
                    case MSG_START_MUSICPLAY:
                        DlnaMediaModel mediaInfo1 = (DlnaMediaModel) msg.obj;
                        startPlayMusic(mediaInfo1);
                        break;
                    case MSG_START_PICPLAY:
                        DlnaMediaModel mediaInfo2 = (DlnaMediaModel) msg.obj;
                        startPlayPicture(mediaInfo2);
                        break;
                    case MSG_START_VIDOPLAY:
                        DlnaMediaModel mediaInfo3 = (DlnaMediaModel) msg.obj;
                        startPlayVideo(mediaInfo3);
                        break;
                    case MSG_SEND_STOPCMD:
                        int type = (int) msg.arg1;
                        MediaControlBrocastFactory.sendStopBorocast(mContext, type);
                        break;
                    case MSG_START_SCREENPLAY:
                        DlnaMediaModel mediaInfo4 = (DlnaMediaModel) msg.obj;
                        startPlayScreen(mediaInfo4);
                        break;
                }
            } catch (Exception e) {
                e.printStackTrace();
                log.e("DMRCenter transdel msg catch Exception!!! msgID = " + msg.what);
            }

        }

    };
    private DlnaMediaModel mMusicMediaInfo;
    private DlnaMediaModel mVideoMediaInfo;
    private DlnaMediaModel mImageMediaInfo;
    private DlnaMediaModel mScreenMediaInfo;
    private int mCurMediaInfoType = -1;

    public DMRCenter(Context context) {
        mContext = context;
    }

    @Override
    public synchronized void onActionInvoke(int cmd, String value, String data, String title) {

        switch (cmd) {
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_SET_AV_URL:
                onRenderAvTransport(value, data);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_PLAY:
                onRenderPlay(value, data);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_PAUSE:
                onRenderPause(value, data);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_STOP:
                onRenderStop(value, data);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_SEEK:
                onRenderSeek(value, data);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_SETMUTE:
                onRenderSetMute(value, data);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_SETVOLUME:
                onRenderSetVolume(value, data);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_SETMETADATA:
                onRenderSetMetaData(value, data);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_SETIPADDR:
                onRenderSetIPAddr(value, data);
                log.e("ipaddr = " + value);
                break;
            case PlatinumReflection.MEDIA_RENDER_CTL_MSG_VIDEO_SIZE_CHANGED:
                onVideoSizeChanged(value, data);
                break;

            default:
                log.e("unrognized cmd!!!");
                break;
        }
    }

    public synchronized void onActionInvoke(int cmd, String value, byte data[], String title) {
        onRenderSetCover(value, data);
    }

    private AudioTrack createAudioTrack(int channelConfig, int sampleRate, int bufferSize, boolean lowLatency) {
        Log.d(TAG, "createAudioTrack() : channelConfig = [" + channelConfig + "], sampleRate = [" + sampleRate + "], bufferSize = [" + bufferSize + "], lowLatency = [" + lowLatency + "]");
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            return new AudioTrack(AudioManager.STREAM_MUSIC,
                    sampleRate,
                    channelConfig,
                    AudioFormat.ENCODING_PCM_16BIT,
                    bufferSize,
                    AudioTrack.MODE_STREAM);
        } else {
            AudioAttributes.Builder attributesBuilder = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_MEDIA)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC);

            AudioFormat format = new AudioFormat.Builder()
                    .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                    .setSampleRate(sampleRate)
                    .setChannelMask(channelConfig)
                    .build();

            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
                // Use FLAG_LOW_LATENCY on L through N
                if (lowLatency) {
                    attributesBuilder.setFlags(AudioAttributes.FLAG_LOW_LATENCY);
                }
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                AudioTrack.Builder trackBuilder = new AudioTrack.Builder()
                        .setAudioFormat(format)
                        .setAudioAttributes(attributesBuilder.build())
                        .setTransferMode(AudioTrack.MODE_STREAM)
                        .setBufferSizeInBytes(bufferSize);

                // Use PERFORMANCE_MODE_LOW_LATENCY on O and later
                if (lowLatency) {
                    trackBuilder.setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY);
                }

                return trackBuilder.build();
            } else {
                return new AudioTrack(attributesBuilder.build(),
                        format,
                        bufferSize,
                        AudioTrack.MODE_STREAM,
                        AudioManager.AUDIO_SESSION_ID_GENERATE);
            }
        }
    }

    public synchronized void audio_init(int bits, int channels, int sampleRate, int isaudio) {
        Log.d(TAG, "audio_init()  bits = [" + bits + "], channels = [" + channels + "], samplerate = [" + sampleRate + "], isaudio = [" + isaudio + "]");

        Action.broadcast(Action.AIRPLAY_MUSIC_START);
        if (PlatinumJniProxy.AUDIO_RENDER != PlatinumJniProxy.RENDER_AUDIO_TRACK) {
            return;
        }

        if (track == null) {
            int minBufferSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_STEREO, AudioFormat.ENCODING_PCM_16BIT);

            boolean lowLatency = true;
            if (AudioTrack.getNativeOutputSampleRate(AudioManager.STREAM_MUSIC) != sampleRate && lowLatency) {
                Log.d(TAG, "audio_init() disable lowLatency ");
                lowLatency = false;
            }
            track = createAudioTrack(AudioFormat.CHANNEL_OUT_STEREO, sampleRate, minBufferSize * 2, lowLatency);
        }
        track.play();
    }

    public synchronized void audio_process(byte[] data, double timestamp, int seqnum) {
        try {
            if (track != null)
                track.write(data, 0, data.length);
        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    public synchronized void audio_destroy() {
        Log.d(TAG, "audio_destroy() ");

        Action.broadcast(Action.AIRPLAY_MUSIC_STOP);
        if (PlatinumJniProxy.AUDIO_RENDER != PlatinumJniProxy.RENDER_AUDIO_TRACK) {
            return;
        }

        if (track != null) {
            try {
                track.flush();
                track.stop();
                //track = null;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public synchronized void audio_uninit() {
        Log.d(TAG, "audio_uninit() ");
        if (track != null) {
            try {
                track.release();
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                track = null;
            }
        }
    }

    @Override
    public void onRenderAvTransport(String value, String data) {
        if (data == null) {
            log.e("meteData = null!!!");
            return;
        }

        if (value == null || value.length() < 2) {
            log.e("url = " + value + ", it's invalid...");
            return;
        }

        DlnaMediaModel mediaInfo = DlnaMediaModelFactory.createFromMetaData(data);
        mediaInfo.setUrl(value);
        if (DlnaUtils.isAudioItem(mediaInfo)) {
            mMusicMediaInfo = mediaInfo;
            mCurMediaInfoType = CUR_MEDIA_TYPE_MUSCI;
        } else if (DlnaUtils.isVideoItem(mediaInfo)) {
            mVideoMediaInfo = mediaInfo;
            mCurMediaInfoType = CUR_MEDIA_TYPE_VIDEO;
        } else if (DlnaUtils.isImageItem(mediaInfo)) {
            mImageMediaInfo = mediaInfo;
            mCurMediaInfoType = CUR_MEDIA_TYPE_PICTURE;
        } else if (DlnaUtils.isScreenItem(mediaInfo)) {
            mScreenMediaInfo = mediaInfo;
            mCurMediaInfoType = CUR_MEDIA_TYPE_SCREEN;
        } else {
            log.e("unknow media type!!! mediainfo.objectclass = \n" + mediaInfo.getObjectClass());
        }
    }

    @Override
    public void onRenderPlay(String value, String data) {
        Log.d(TAG, "onRenderPlay() : value = [" + value + "], data = [" + mCurMediaInfoType + "]");
        switch (mCurMediaInfoType) {
            case CUR_MEDIA_TYPE_MUSCI:
                if (mMusicMediaInfo != null) {
                    delayToPlayMusic(mMusicMediaInfo);
                } else {
                    MediaControlBrocastFactory.sendPlayBrocast(mContext);
                }
                clearState();
                break;
            case CUR_MEDIA_TYPE_VIDEO:
                if (mVideoMediaInfo != null) {
                    delayToPlayVideo(mVideoMediaInfo);
                } else {
                    MediaControlBrocastFactory.sendPlayBrocast(mContext);
                }
                clearState();
                break;
            case CUR_MEDIA_TYPE_PICTURE:
                if (mImageMediaInfo != null) {
                    delayToPlayImage(mImageMediaInfo);
                } else {
                    MediaControlBrocastFactory.sendPlayBrocast(mContext);
                }
                clearState();
                break;
            case CUR_MEDIA_TYPE_SCREEN:
                if (mScreenMediaInfo != null) {
                    delayToPlayScreen(mScreenMediaInfo);
                } else {
                    MediaControlBrocastFactory.sendPlayBrocast(mContext);
                }
                clearState();
                break;
            default:
        }
    }

    @Override
    public void onRenderPause(String value, String data) {
        MediaControlBrocastFactory.sendPauseBrocast(mContext);
    }

    @Override
    public void onRenderStop(String value, String data) {
        delayToStop(Integer.valueOf(data));
        log.d("MediaControlBrocastFactory stop");
        MediaControlBrocastFactory.sendStopBorocast(mContext, Integer.valueOf(data));

        audio_uninit();  // stop audio when mirroring stop
    }

    @Override
    public void onRenderSeek(String value, String data) {
        int seekPos = 0;
        try {
            seekPos = DlnaUtils.parseSeekTime(value);
            MediaControlBrocastFactory.sendSeekBrocast(mContext, seekPos);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onRenderSetMute(String value, String data) {

        if ("1".equals(value)) {
            CommonUtil.setVolumeMute(mContext);
        } else if ("0".equals(value)) {
            CommonUtil.setVolumeUnmute(mContext);
        }
    }

    public void onRenderSetVolume(String value, String data) {
        try {
            int volume = (int) Math.round((Float.valueOf(value) + 30) * 3.34);  // max 0  min  -30
            Log.d(TAG, "onRenderSetVolume()  value = [" + value + "], data = [" + volume + "]");
            if (volume < 101) {
                CommonUtil.setCurrentVolume(volume, mContext);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onRenderSetCover(String value, byte data[]) {

        MediaControlBrocastFactory.sendCoverBrocast(mContext, data);
    }

    @Override
    public void onRenderSetMetaData(String value, String data) {

        MediaControlBrocastFactory.sendMetaDataBrocast(mContext, data);
    }

    @Override
    public void onRenderSetIPAddr(String value, String data) {

        MediaControlBrocastFactory.sendIPAddrBrocast(mContext, value);
    }

    @Override
    public void onVideoSizeChanged(String value, String data) {
        Log.d(TAG, "onVideoSizeChanged() : value = [" + value + "], data = [" + data + "]");
        MediaControlBrocastFactory.sendSizeChangeBrocast(mContext, value);
    }

    private void clearState() {
        mMusicMediaInfo = null;
        mVideoMediaInfo = null;
        mImageMediaInfo = null;
        mScreenMediaInfo = null;
    }

    private void delayToPlayMusic(DlnaMediaModel mediaInfo) {
        if (mediaInfo != null) {
            clearDelayMsg();
            Message msg = mHandler.obtainMessage(MSG_START_MUSICPLAY, mediaInfo);
            //mHandler.sendMessageDelayed(msg, DELAYTIME);
            mHandler.sendMessage(msg);
        }
    }

    private void delayToPlayVideo(DlnaMediaModel mediaInfo) {
        if (mediaInfo != null) {
            clearDelayMsg();
            Message msg = mHandler.obtainMessage(MSG_START_VIDOPLAY, mediaInfo);
            //mHandler.sendMessageDelayed(msg, DELAYTIME);
            mHandler.sendMessage(msg);
        }
    }

    private void delayToPlayImage(DlnaMediaModel mediaInfo) {
        if (mediaInfo != null) {
            clearDelayMsg();
            Message msg = mHandler.obtainMessage(MSG_START_PICPLAY, mediaInfo);
            //mHandler.sendMessageDelayed(msg, DELAYTIME);
            mHandler.sendMessage(msg);
        }
    }

    private void delayToPlayScreen(DlnaMediaModel mediaInfo) {
        if (mediaInfo != null) {
            clearDelayMsg();
            Message msg = mHandler.obtainMessage(MSG_START_SCREENPLAY, mediaInfo);
            //mHandler.sendMessageDelayed(msg, DELAYTIME);
            mHandler.sendMessage(msg);
        }
    }

    private void delayToStop(int type) {
        clearDelayMsg();
        mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_SEND_STOPCMD, type), DELAYTIME);
        //mHandler.sendMessage(mHandler.obtainMessage(MSG_SEND_STOPCMD,type));
    }

    private void clearDelayMsg() {
        clearDelayMsg(MSG_START_MUSICPLAY);
        clearDelayMsg(MSG_START_PICPLAY);
        clearDelayMsg(MSG_START_VIDOPLAY);
        clearDelayMsg(MSG_SEND_STOPCMD);
        clearDelayMsg(MSG_START_SCREENPLAY);
    }

    private void clearDelayMsg(int num) {
        mHandler.removeMessages(num);
    }

    private void startPlayMusic(DlnaMediaModel mediaInfo) {
        log.d("startPlayMusic" + mediaInfo);
        Intent intent = new Intent();
        intent.setClass(mContext, MusicActivity.class);
        DlnaMediaModelFactory.pushMediaModelToIntent(intent, mediaInfo);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        mContext.startActivity(intent);
    }

    private void startPlayVideo(DlnaMediaModel mediaInfo) {
        Intent intent = new Intent();
        if (Setting.getInstance().isUseMediaPlayer() )
            intent.setClass(mContext, VideoActivity.class);
         else
            intent.setClass(mContext, CicadaVideoPlayer.class);
        DlnaMediaModelFactory.pushMediaModelToIntent(intent, mediaInfo);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        mContext.startActivity(intent);
    }

    private void startPlayPicture(DlnaMediaModel mediaInfo) {
        log.d("startPlayPicture" + mediaInfo);
        Intent intent = new Intent();
        intent.setClass(mContext, ImageActivity.class);
        DlnaMediaModelFactory.pushMediaModelToIntent(intent, mediaInfo);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        mContext.startActivity(intent);
    }

    private void startPlayScreen(DlnaMediaModel mediaInfo) {
        MirrorActivity.intentTo(mContext, Source.MIRROR_AIRPLAY);
    }

}
