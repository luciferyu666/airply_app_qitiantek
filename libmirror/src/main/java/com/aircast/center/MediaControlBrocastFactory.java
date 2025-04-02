package com.aircast.center;


import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import com.aircast.source.Source;
import com.aircast.util.Action;

import java.io.File;
import java.io.FileOutputStream;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class MediaControlBrocastFactory {

    public static final String MEDIA_RENDERER_CMD_PLAY = "com.aircast.control.play_command";
    public static final String MEDIA_RENDERER_CMD_PAUSE = "com.aircast.control.pause_command";
    public static final String MEDIA_RENDERER_CMD_STOP = "com.aircast.control.stop_command";
    public static final String MEDIA_RENDERER_CMD_SEEKPS = "com.aircast.control.seekps_command";
    public static final String MEDIA_RENDERER_CMD_COVER = "com.aircast.control.cover";
    public static final String MEDIA_RENDERER_CMD_METADATA = "com.aircast.control.metadata";
    public static final String MEDIA_RENDERER_CMD_IPADDR = "com.aircast.control.ipaddr";
    public static final String MEDIA_RENDERER_CMD_VIDEO_SIZE_CHANGED = "com.aircast.video.size.change";
    public static final String PARAM_CMD_SEEKPS = "get_param_seekps";
    public static final String PARAM_CMD_STOPTYPE = "get_param_stoptype";
    public static final String PARAM_CMD_COVER = "get_param_cover";
    public static final String PARAM_CMD_METADATA = "get_param_metadata";
    public static final String PARAM_CMD_IPADDR = "get_param_ipaddr";

    public static final String PARAM_METADATA_TITLE = "audio_param_metadata_title";
    public static final String PARAM_METADATA_ARTIST = "audio_param_metadata_artist";
    private final Context mContext;
    private MediaControlBrocastReceiver mMediaControlReceiver;

    public MediaControlBrocastFactory(Context context) {
        mContext = context;
    }

    public static void sendPlayBrocast(Context context) {
        Intent intent = new Intent(MEDIA_RENDERER_CMD_PLAY);
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public static void sendPauseBrocast(Context context) {
        Intent intent = new Intent(MEDIA_RENDERER_CMD_PAUSE);
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public static void sendStopBorocast(Context context, int type) {
        Intent intent = new Intent(MEDIA_RENDERER_CMD_STOP);
        intent.putExtra(PARAM_CMD_STOPTYPE, type);
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public static void sendSeekBrocast(Context context, int seekPos) {
        Intent intent = new Intent(MEDIA_RENDERER_CMD_SEEKPS);
        intent.putExtra(PARAM_CMD_SEEKPS, seekPos);
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public static void sendCoverBrocast(Context context, byte[] data) {
        Log.d("music", "cover  " + context.getCacheDir().getAbsolutePath()+"/cover.jpeg" );
        File file = new File(context.getCacheDir(), "cover.jpeg");  // audio cover
        try(FileOutputStream fs = new FileOutputStream(file) ) {
            fs.write(data);
            fs.flush();
        } catch ( Exception e) {
            e.printStackTrace();
        }
        Intent intent = new Intent(MEDIA_RENDERER_CMD_COVER);
        intent.putExtra(PARAM_CMD_COVER, file.getAbsolutePath());
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public static void sendMetaDataBrocast(Context context, String data) {
        Intent intent = new Intent(MEDIA_RENDERER_CMD_METADATA);
        String title = "", artist = "";
        try {
            Pattern titlePattern = Pattern.compile("<dc:title>(.*?)</dc:title>");
            Matcher titleMatcher = titlePattern.matcher(data);

            if (titleMatcher.find()) {
                title = titleMatcher.group(1);
                intent.putExtra(PARAM_METADATA_TITLE, title);  // music lyric
            }

            Pattern artistPattern = Pattern.compile("<upnp:artist>(.*?)</upnp:artist>");
            Matcher artistMatcher = artistPattern.matcher(data);

            if (artistMatcher.find()) {
                artist = artistMatcher.group(1);
                intent.putExtra(PARAM_METADATA_ARTIST, artist);
            }
            Log.d("music", "title = [" + title + "], artist = [" + artist + "]");
        } catch ( Exception e) {
            e.printStackTrace();
        }
        intent.putExtra(PARAM_CMD_METADATA, data);
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public static void sendIPAddrBrocast(Context context, String data) {
        Intent intent = new Intent(MEDIA_RENDERER_CMD_IPADDR);
        intent.putExtra(PARAM_CMD_IPADDR, data);
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public static void sendSizeChangeBrocast(Context context, String data) {
        Action.broadcast(MEDIA_RENDERER_CMD_VIDEO_SIZE_CHANGED, Source.MIRROR_AIRPLAY);
    }

    public void register(IMediaControlListener listener) {
        if (mMediaControlReceiver == null) {
            mMediaControlReceiver = new MediaControlBrocastReceiver();
            mMediaControlReceiver.setMediaControlListener(listener);

            LocalBroadcastManager.getInstance(mContext).registerReceiver(mMediaControlReceiver, new IntentFilter(MediaControlBrocastFactory.MEDIA_RENDERER_CMD_PLAY));
            LocalBroadcastManager.getInstance(mContext).registerReceiver(mMediaControlReceiver, new IntentFilter(MediaControlBrocastFactory.MEDIA_RENDERER_CMD_PAUSE));
            LocalBroadcastManager.getInstance(mContext).registerReceiver(mMediaControlReceiver, new IntentFilter(MediaControlBrocastFactory.MEDIA_RENDERER_CMD_STOP));
            LocalBroadcastManager.getInstance(mContext).registerReceiver(mMediaControlReceiver, new IntentFilter(MediaControlBrocastFactory.MEDIA_RENDERER_CMD_SEEKPS));
            LocalBroadcastManager.getInstance(mContext).registerReceiver(mMediaControlReceiver, new IntentFilter(MediaControlBrocastFactory.MEDIA_RENDERER_CMD_COVER));
            LocalBroadcastManager.getInstance(mContext).registerReceiver(mMediaControlReceiver, new IntentFilter(MediaControlBrocastFactory.MEDIA_RENDERER_CMD_METADATA));
            LocalBroadcastManager.getInstance(mContext).registerReceiver(mMediaControlReceiver, new IntentFilter(MediaControlBrocastFactory.MEDIA_RENDERER_CMD_IPADDR));
        }
    }

    public void unregister() {
        if (mMediaControlReceiver != null) {
            LocalBroadcastManager.getInstance(mContext).unregisterReceiver(mMediaControlReceiver);
            mMediaControlReceiver = null;
        }
    }

    public static interface IMediaControlListener {
        public void onPlayCommand();

        public void onPauseCommand();

        public void onStopCommand(int type);

        public void onSeekCommand(int time);

        public void onCoverCommand(byte data[]);

        public void onMetaDataCommand(String data);

        public void onIPAddrCommand(String data);
    }
}
