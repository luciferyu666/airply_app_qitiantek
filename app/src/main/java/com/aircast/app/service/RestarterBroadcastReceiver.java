package com.aircast.app.service;


import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.util.Log;

import com.aircast.util.NetUtils;

public class RestarterBroadcastReceiver extends BroadcastReceiver {
    private static final String TAG = "AIR";
    private final Handler handler = new Handler();

    @Override
    public void onReceive(Context ctx, Intent intent) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                if (NetUtils.isOnline(ctx)) {
                    MainService.intentToStart(ctx);
                    Log.d(TAG, "NetUtils Online");
                } else {
                    Log.d(TAG, "NetUtils offline");
                    handler.postDelayed(this, 1500);
                }
            }
        });
    }
}