package com.aircast.settings;

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.util.Log;

import com.aircast.AirplayApp;
import com.aircast.R;
import com.aircast.util.NetUtils;
import com.google.gson.ExclusionStrategy;
import com.google.gson.FieldAttributes;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import java.util.Random;


public class Setting {
    private static final String TAG = "Setting";
    private static final Object MUTEX = new Object();
    // singleton
    private static volatile Setting instance;
    private final ExclusionStrategy strategy = new ExclusionStrategy() {
        @Override
        public boolean shouldSkipField(FieldAttributes field) {
            return false;
        }

        @Override
        public boolean shouldSkipClass(Class<?> clazz) {
            return clazz == Context.class;
        }
    };

    //app
    private boolean useMediaPlayer = false;
    //device
    private String name = null;     // device name
    private String hwaddr = null;    //for airplay
    //airplay
    private int resolution = 1080;  //720  1080  1440
    private int maxfps = 30;  // 30 60

    private Setting() {
    }

    public static Setting getInstance() {
        if (instance == null) {
            synchronized (MUTEX) {
                if (instance == null) {
                    instance = new Setting();
                }
            }
        }
        return instance;
    }

    public void init(final Context ctx) {
        final String json = ctx.getSharedPreferences("mirror_setting", Context.MODE_PRIVATE).getString("settings", "");
        if (json.contains("{")) {
            instance = new Gson().fromJson(json, Setting.class);
        }
    }

    public void commit() {
        SharedPreferences.Editor prefsEditor = AirplayApp.getContext().getSharedPreferences("mirror_setting", Context.MODE_PRIVATE).edit();

        Gson gson = new GsonBuilder()
                .addSerializationExclusionStrategy(strategy)
                .create();
        String json = gson.toJson(instance);

        Log.d(TAG, "commit()  " + json);
        prefsEditor.putString("settings", json);
        prefsEditor.apply();
    }

    public String getName() {
        if (name == null) {
            int s = new Random().nextInt(900) + 100;
            setName(  AirplayApp.getContext().getString(R.string.app_name) + "-" + s);
        }

        return name;
    }

    public void setName(String name) {
        this.name = name;
        commit();
    }

    public int getResolution() {
        return resolution;
    }

    public void setResolution(int resolution) {
        this.resolution = resolution;
    }

    public int getResWidth() {
        switch (resolution) {
            case 720:
                return 1280;
            case 1080:
                return 1920;
            default:
                return 2560; //2560*1440     3840*2160
        }
    }

    public int getResHeight() {
        return resolution;
    }

    public String getResName() {
        return resolution + "P";
    }

    public String getHwaddr() {
        if (hwaddr == null) {
            setHwaddr(randomMACAddress());
        }
        return hwaddr;
    }

    public void setHwaddr(String hw) {
        this.hwaddr = hw;
        commit();
    }

    private String randomMACAddress() {
        Random rand = new Random();
        byte[] macAddr = new byte[6];
        rand.nextBytes(macAddr);

        macAddr[0] = (byte) (macAddr[0] & (byte) 254);  //zeroing last 2 bytes to make it unicast and locally adminstrated

        StringBuilder sb = new StringBuilder(18);
        for (byte b : macAddr) {

            if (sb.length() > 0)
                sb.append(":");

            sb.append(String.format("%02x", b));
        }
        Log.d(TAG, "hwaddr is: " + sb.toString());
        return sb.toString();
    }

    public boolean isUseMediaPlayer() {
        return useMediaPlayer;
    }

    public void setUseMediaPlayer(boolean useMediaPlayer) {
        this.useMediaPlayer = useMediaPlayer;
        commit();
    }

    public int getMaxfps() {
        return maxfps;
    }

    public void setMaxfps(int maxfps) {
        this.maxfps = maxfps;
        commit();
    }
}
