package com.aircast.app.service;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import com.aircast.app.R;
import com.aircast.center.MediaRenderProxy;
import com.aircast.jni.PlatinumJniProxy;
import com.aircast.nsd.NsdHelper;
import com.aircast.util.Action;
import com.aircast.util.CommonDeviceLocks;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainService extends Service {
    public static final int CMD_EXIT = -1;
    public static final int CMD_NULL = 0;
    public static final int CMD_RESTART = 1;
    public static final int CMD_STOP = 2;
    public static final int CMD_START = 3;
    private static final String TAG = "MainService";
    private static volatile boolean isRunning;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final ExecutorService executor = Executors.newFixedThreadPool(2);
    private MediaRenderProxy airplayRender;
    private NsdHelper nsdHelper;
    private final BroadcastReceiver mLocalReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive()  intent = [" + intent + "]");
            switch (intent.getAction()) {
                case PlatinumJniProxy.MDNS_REG_AIRPLAY:
                    handler.postDelayed(() -> {
                        nsdHelper.registerRaop();
                        nsdHelper.registerAirplay();
                    }, 500);
                    break;
                case PlatinumJniProxy.MDNS_UNREG_AIRPLAY:
                    handler.post(() -> nsdHelper.destroy());
                    break;

                default:

            }
        }
    };
    private CommonDeviceLocks locks;

    public static Intent newIntent(Context context) {
        return new Intent(context, MainService.class);
    }

    public static void intentToStop(Context context) {
        context.stopService(newIntent(context));
    }

    public static void intentToStart(Context context) {
        Log.d(TAG, "intentToStart()  ");
        if (isRunning)
            Log.d(TAG, "intentToStart() service running ");
        else
            intentToStart(context, CMD_NULL);
    }

    private static void intentToStart(Context context, int cmd) {
        Log.d(TAG, "intentToStart()  cmd = [" + cmd + "]");
        Intent i = newIntent(context);
        i.putExtra("cmd", cmd);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            context.startForegroundService(i);
        } else {
            context.startService(i);
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        super.onStartCommand(intent, flags, startId);
        if (intent != null) {
            int cmd = intent.getIntExtra("cmd", CMD_NULL);
            switch (cmd) {
                case CMD_RESTART:
                    Log.d(TAG, "restart all");
                    restartAirplay();
                    break;
                case CMD_START:
                    Log.d(TAG, "start all");
                    startAirplay();
                    break;
                case CMD_STOP:
                    Log.d(TAG, "stop all");
                    stopAirplay();
                    break; 

                case CMD_EXIT:
                    stopSelf();
                    break;

                default:
                    break;
            }
        }
        return START_NOT_STICKY;
    }

    @Override
    public void onCreate() {
        isRunning = true;
        super.onCreate();
        Log.d(TAG, "onCreate!");

        registerLocalReceiver();
        locks = new CommonDeviceLocks();
        locks.acquire(this);

        nsdHelper = new NsdHelper(getApplicationContext());
        initData();
        startAirplay();
        keepAliveTrick();
    }

    private void keepAliveTrick() {
        String channelId = "mainServiceChannel";
        NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(channelId, channelId, NotificationManager.IMPORTANCE_HIGH);
            notificationManager.createNotificationChannel(channel);
        }

        Notification notification = new NotificationCompat.Builder(getApplicationContext(), channelId)
                .setSmallIcon(R.drawable.ic_airplay_black_24dp)
                .setSilent(true)
                .setOngoing(false)
                .build();
        startForeground(0x9527, notification);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        isRunning = false;
        Log.d(TAG, "onDestroy()  ");

        Action.unregisterLocalReceiver(mLocalReceiver);
        nsdHelper.destroy();
        stopAirplay();
        stopForeground(true);

        executor.shutdownNow();
        locks.release();
    }


    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void initData() {
        airplayRender = MediaRenderProxy.getInstance();
    }

    private void startAirplay() {
        Log.d(TAG, "startAirplay()  ");
        airplayRender.restartEngine();
    }

    private void restartAirplay() {
        airplayRender.restartEngine();
    }

    private void stopAirplay() {
        Log.d(TAG, "stopAirplay()  ");
        airplayRender.stopEngine();
        PlatinumJniProxy.destroy();
    }

    private void registerLocalReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(PlatinumJniProxy.MDNS_REG_AIRPLAY);
        intentFilter.addAction(PlatinumJniProxy.MDNS_UNREG_AIRPLAY);
        Action.registerLocalReceiver(getApplicationContext(), mLocalReceiver, intentFilter);
    }

}