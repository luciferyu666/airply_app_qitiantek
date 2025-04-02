package com.aircast.center;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;


public class DeviceUpdateBrocastFactory {

    public static final String PARAM_DEV_UPDATE = "com.aircast.PARAM_DEV_UPDATE";
    private DeviceUpdateBrocastReceiver mReceiver;
    private Context mContext;

    public DeviceUpdateBrocastFactory(Context context) {
        mContext = context;
    }

    public static void sendDevUpdateBrocast(Context context) {
        Intent intent = new Intent();
        intent.setAction(PARAM_DEV_UPDATE);

        LocalBroadcastManager.getInstance(context).sendBroadcast(intent);
    }

    public void register(IDevUpdateListener listener) {
        if (mReceiver == null) {
            mReceiver = new DeviceUpdateBrocastReceiver();
            mReceiver.setListener(listener);

            LocalBroadcastManager.getInstance(mContext).registerReceiver(mReceiver, new IntentFilter(PARAM_DEV_UPDATE));
        }
    }

    public void unregister() {
        if (mReceiver != null) {

            LocalBroadcastManager.getInstance(mContext).unregisterReceiver(mReceiver);
            mReceiver = null;
        }
    }

    public static interface IDevUpdateListener {
        public void onUpdate();
    }

}
