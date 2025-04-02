package com.aircast.util;

import static android.content.Context.POWER_SERVICE;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.TrafficStats;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.Build;
import android.os.Environment;
import android.os.PowerManager;
import android.provider.Settings;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;
import android.widget.Toast;

public class CommonUtil {

    private static final CommonLog log = LogFactory.createLog();
    private static long m_lSysNetworkSpeedLastTs = 0;
    private static long m_lSystNetworkLastBytes = 0;
    private static float m_fSysNetowrkLastSpeed = 0.0f;

    public static boolean hasSDCard() {
        String status = Environment.getExternalStorageState();
        if (!status.equals(Environment.MEDIA_MOUNTED)) {
            return false;
        }
        return true;
    }

    public static String getRootFilePath() {
        if (hasSDCard()) {
            return Environment.getExternalStorageDirectory().getAbsolutePath() + "/";// filePath:/sdcard/
        } else {
            return Environment.getDataDirectory().getAbsolutePath() + "/data/"; // filePath: /data/data/
        }
    }

    public static MulticastLock openWifiBrocast(Context context) {
        WifiManager wifiManager = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
        MulticastLock multicastLock = wifiManager.createMulticastLock("MediaRender");
        if (multicastLock != null) {
            multicastLock.acquire();
        }
        return multicastLock;
    }

    public static void setCurrentVolume(int percent, Context mc) {
        AudioManager am = (AudioManager) mc.getSystemService(Context.AUDIO_SERVICE);
        int maxvolume = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
        am.setStreamVolume(AudioManager.STREAM_MUSIC, (maxvolume * percent) / 100,
                AudioManager.FLAG_PLAY_SOUND | AudioManager.FLAG_SHOW_UI);
        am.setMode(AudioManager.MODE_NORMAL);
    }

    public static void setVolumeMute(Context mc) {
        AudioManager am = (AudioManager) mc.getSystemService(Context.AUDIO_SERVICE);
        am.setStreamMute(AudioManager.STREAM_MUSIC, true);
    }

    public static void setVolumeUnmute(Context mc) {
        AudioManager am = (AudioManager) mc.getSystemService(Context.AUDIO_SERVICE);
        am.setStreamMute(AudioManager.STREAM_MUSIC, false);
    }

    public static void showToask(Context context, String tip) {
        Toast.makeText(context, tip, Toast.LENGTH_SHORT).show();
    }

    public static int getScreenWidth(Context context) {
        WindowManager manager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = manager.getDefaultDisplay();
        return display.getWidth();
    }

    public static int getScreenHeight(Context context) {
        WindowManager manager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = manager.getDefaultDisplay();
        return display.getHeight();
    }

    public static ViewSize getFitSize(Context context, MediaPlayer mediaPlayer) {
        int videoWidth = mediaPlayer.getVideoWidth();
        int videoHeight = mediaPlayer.getVideoHeight();
        double fit1 = videoWidth * 1.0 / videoHeight;

        int width2 = getScreenWidth(context);
        int height2 = getScreenHeight(context);
        double fit2 = width2 * 1.0 / height2;

        double fit = 1;
        if (fit1 > fit2) {
            fit = width2 * 1.0 / videoWidth;
        } else {
            fit = height2 * 1.0 / videoHeight;
        }

        ViewSize viewSize = new ViewSize();
        viewSize.width = (int) (fit * videoWidth);
        viewSize.height = (int) (fit * videoHeight);

        return viewSize;
    }

    public static float getSysNetworkDownloadSpeed() {
        long nowMS = System.currentTimeMillis();
        long nowBytes = TrafficStats.getTotalRxBytes();

        long timeinterval = nowMS - m_lSysNetworkSpeedLastTs;
        long bytes = nowBytes - m_lSystNetworkLastBytes;

        if (timeinterval > 0) m_fSysNetowrkLastSpeed = (float) bytes * 1.0f / (float) timeinterval;

        m_lSysNetworkSpeedLastTs = nowMS;
        m_lSystNetworkLastBytes = nowBytes;

        return m_fSysNetowrkLastSpeed;
    }

    public static class ViewSize {
        public int width = 0;
        public int height = 0;
    }
}


