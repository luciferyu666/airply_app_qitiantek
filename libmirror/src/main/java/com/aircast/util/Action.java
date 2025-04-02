package com.aircast.util;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import java.io.Serializable;

public class Action {

    public static final String PARAM = "param";
    public static final String AIRPLAY_MUSIC_START = "air.music.start";
    public static final String AIRPLAY_MUSIC_STOP = "air.music.stop";
    public static final String EXIT_MIRRORING = "exit.mirroring";
    private static LocalBroadcastManager manager = null;

    public static void init(Context context) {
        manager = LocalBroadcastManager.getInstance(context);
    }

    public static void broadcast(String action, String param) {
        Intent intent = new Intent();
        intent.setAction(action);
        intent.putExtra(Action.PARAM, param);
        manager.sendBroadcast(intent);
    }

    public static void broadcast(String action) {
        broadcast(action, null);
    }

    public static void registerLocalReceiver(Context context, BroadcastReceiver receiver, IntentFilter filter) {
        if (manager == null) {
            manager = LocalBroadcastManager.getInstance(context);
        }
        manager.registerReceiver(receiver, filter);
    }

    public static void unregisterLocalReceiver(BroadcastReceiver receiver) {
        if (manager != null)
            manager.unregisterReceiver(receiver);
    }
}

