package com.aircast.center;


import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.aircast.jni.PlatinumReflection;
import com.aircast.util.CommonLog;
import com.aircast.util.DlnaUtils;
import com.aircast.util.LogFactory;

public class DLNAGenaEventBrocastFactory {

    private static final CommonLog log = LogFactory.createLog();

    private DLNAGenaEventBrocastReceiver mReceiver;
    private Context mContext;

    public DLNAGenaEventBrocastFactory(Context context) {
        mContext = context;
    }

    public static void sendTranstionEvent(Context context) {
        sendGenaPlayState(context, PlatinumReflection.MEDIA_PLAYINGSTATE_TRANSTION);
    }

    public static void sendDurationEvent(Context context, int duration) {
        if (duration != 0) {
            Intent setintent = new Intent(PlatinumReflection.RENDERER_TOCONTRPOINT_CMD_INTENT_NAME);
            setintent.putExtra(PlatinumReflection.GET_RENDERER_TOCONTRPOINT_CMD, PlatinumReflection.MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_DURATION);
            setintent.putExtra(PlatinumReflection.GET_PARAM_MEDIA_DURATION, DlnaUtils.formatTimeFromMSInt(duration));
            LocalBroadcastManager.getInstance(context).sendBroadcast(setintent);
        }
    }

    public static void sendSeekEvent(Context context, int time) {
        if (time != 0) {

            Intent setintent = new Intent(PlatinumReflection.RENDERER_TOCONTRPOINT_CMD_INTENT_NAME);
            setintent.putExtra(PlatinumReflection.GET_RENDERER_TOCONTRPOINT_CMD, PlatinumReflection.MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_POSITION);
            setintent.putExtra(PlatinumReflection.GET_PARAM_MEDIA_POSITION, DlnaUtils.formatTimeFromMSInt(time));
            LocalBroadcastManager.getInstance(context).sendBroadcast(setintent);
        }
    }

    public static void sendPlayStateEvent(Context context) {
        sendGenaPlayState(context, PlatinumReflection.MEDIA_PLAYINGSTATE_PLAYING);
    }

    public static void sendPauseStateEvent(Context context) {
        sendGenaPlayState(context, PlatinumReflection.MEDIA_PLAYINGSTATE_PAUSE);
    }

    public static void sendStopStateEvent(Context context) {
        sendGenaPlayState(context, PlatinumReflection.MEDIA_PLAYINGSTATE_STOP);
    }

    private static void sendGenaPlayState(Context context, String state) {
        Intent intent = new Intent(PlatinumReflection.RENDERER_TOCONTRPOINT_CMD_INTENT_NAME);
        intent.putExtra(PlatinumReflection.GET_RENDERER_TOCONTRPOINT_CMD, PlatinumReflection.MEDIA_RENDER_TOCONTRPOINT_SET_MEDIA_PLAYINGSTATE);
        intent.putExtra(PlatinumReflection.GET_PARAM_MEDIA_PLAYINGSTATE, state);
        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public void registerBrocast() {
        if (mReceiver == null) {
            mReceiver = new DLNAGenaEventBrocastReceiver();
            LocalBroadcastManager.getInstance(mContext).registerReceiver(mReceiver, new IntentFilter(PlatinumReflection.RENDERER_TOCONTRPOINT_CMD_INTENT_NAME));
        }
    }

    public void unRegisterBrocast() {
        if (mReceiver != null) {
            LocalBroadcastManager.getInstance(mContext).unregisterReceiver(mReceiver);
            mReceiver = null;
        }
    }

}
