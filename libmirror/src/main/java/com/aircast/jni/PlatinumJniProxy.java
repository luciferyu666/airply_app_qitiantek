package com.aircast.jni;

import android.content.Context;
import android.util.Log;

import com.aircast.AirplayApp;
import com.aircast.settings.Setting;

import java.io.UnsupportedEncodingException;

public class PlatinumJniProxy {
    public static final int RENDER_AUDIO_TRACK = 0;
    public static final int RENDER_OBOE = 1; // With Sample Rate Conversion
    public static final int RENDER_LOW_DELAY = 2;
    public static final int RENDER_OBOE_NO_CONV = 3; // No Sample Rate Conversion
    public static final int AUDIO_RENDER = RENDER_OBOE_NO_CONV; // airplay mirror audio render
    private static final String TAG = "JniProxy";
    public static final String MDNS_REG_AIRPLAY = "reg.airplay";
    public static final String MDNS_UNREG_AIRPLAY = "unreg.airplay";
    static {
        System.loadLibrary("airplay");
    }

    public static native int setAudioRender(int render);
    public static native int setMaxFps(int fps);
    public static native int startMediaRender(String friendname, String mac, String activecode, int width, int height, int airtunes_port, int airplay_port, int rcv_size, Context obj);

    public static native int stopMediaRender();

    public static native boolean responseGenaEvent(int cmd, byte[] value, byte[] data);

    public static native boolean enableLogPrint(boolean flag);

    public static native int changePassword(String pwd);

    public static native int setDNDMode(boolean enable);

    public static native int setHwDecode(boolean enable);

    public static native int destroy();
    public static native int getAirplayPort();
    public static native int getRaopPort();
    private static void setup() {
        PlatinumJniProxy.setMaxFps(Setting.getInstance().getMaxfps());
        PlatinumJniProxy.setAudioRender(AUDIO_RENDER);
    }

    public static int startMediaRender_Java(String friendname) {
        Log.d(TAG, "startMediaRender_Java() : friendname = [" + friendname + "]");
        if (friendname == null) friendname = "";

        int ret = -1;

        int w = Setting.getInstance().getResWidth();
        int h = Setting.getInstance().getResHeight();
        setup();
        ret = startMediaRender(friendname, Setting.getInstance().getHwaddr(), "", w, h, 0, 0, 0, AirplayApp.getContext());

        return ret;
    }

    public static boolean responseGenaEvent(int cmd, String value, String data) {
        if (value == null) value = "";
        if (data == null) data = "";
        boolean ret = false;
        try {
            ret = responseGenaEvent(cmd, value.getBytes("utf-8"), data.getBytes("utf-8"));
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
        return ret;
    }


}
