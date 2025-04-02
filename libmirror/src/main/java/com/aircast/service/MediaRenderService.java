package com.aircast.service;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;

import androidx.core.app.NotificationCompat;

import com.aircast.R;
import com.aircast.center.DLNAGenaEventBrocastFactory;
import com.aircast.center.DMRCenter;
import com.aircast.center.DMRWorkThread;
import com.aircast.center.IBaseEngine;
import com.aircast.jni.PlatinumReflection;
import com.aircast.jni.PlatinumReflection.ActionReflectionListener;
import com.aircast.util.CommonLog;
import com.aircast.util.CommonUtil;
import com.aircast.util.DlnaUtils;
import com.aircast.util.LogFactory;

public class MediaRenderService extends Service implements IBaseEngine {
    public static final String START_RENDER_ENGINE = "com.aircast.start.engine";
    public static final String RESTART_RENDER_ENGINE = "com.aircast.restart.engine";
    private static final String TAG = "MRS";
    private static final CommonLog log = LogFactory.createLog();
    private static final int START_ENGINE_MSG_ID = 0x0001;
    private static final int RESTART_ENGINE_MSG_ID = 0x0002;
    private static final int DELAY_TIME = 1000;
    private DMRWorkThread mWorkThread;
    private ActionReflectionListener mListener;
    private DLNAGenaEventBrocastFactory mMediaGenaBrocastFactory;
    private Handler mHandler;
    private MulticastLock mMulticastLock;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        initRenderService();

        log.e("MediaRenderService onCreate");
        keepAliveTrick();
    }

    private void keepAliveTrick() {
        String channelId = "mediaRenderServiceChannel";
        NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(channelId, channelId, NotificationManager.IMPORTANCE_HIGH);
            notificationManager.createNotificationChannel(channel);
        }

        Notification notification = new NotificationCompat.Builder(getApplicationContext(), channelId)
                .setSmallIcon(R.drawable.ap_ic_airplay_black_24dp)
                //.setSilent(true)
                .setOngoing(false)
                .build();
        startForeground(0x19527, notification);
    }

    @Override
    public void onDestroy() {
        stopForeground(true);
        unInitRenderService();
        log.e("MediaRenderService onDestroy");
        super.onDestroy();

    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        if (intent != null) {
            String actionString = intent.getAction();
            if (actionString != null) {
                if (actionString.equalsIgnoreCase(START_RENDER_ENGINE)) {
                    delayToSendStartMsg();
                } else if (actionString.equalsIgnoreCase(RESTART_RENDER_ENGINE)) {
                    delayToSendRestartMsg();
                }
            }
        }

        return super.onStartCommand(intent, flags, startId);

    }


    private void initRenderService() {
        mListener = new DMRCenter(this);
        PlatinumReflection.setActionInvokeListener(mListener);
        mMediaGenaBrocastFactory = new DLNAGenaEventBrocastFactory(this);
        mMediaGenaBrocastFactory.registerBrocast();
        mWorkThread = new DMRWorkThread(this);
        mWorkThread.setName(TAG);

        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                Log.d(TAG, "handleMessage() : msg = [" + msg + "]");
                switch (msg.what) {
                    case START_ENGINE_MSG_ID:
                        startEngine();
                        break;
                    case RESTART_ENGINE_MSG_ID:
                        restartEngine();
                        break;
                }
            }

        };

        mMulticastLock = CommonUtil.openWifiBrocast(this);
        log.e("openWifiBrocast = " + mMulticastLock != null ? true : false);
    }


    private void unInitRenderService() {
        stopEngine();
        removeStartMsg();
        removeRestartMsg();
        mMediaGenaBrocastFactory.unRegisterBrocast();
        if (mMulticastLock != null) {
            mMulticastLock.release();
            mMulticastLock = null;
            log.e("closeWifiBrocast");
        }
    }

    private void delayToSendStartMsg() {
        removeStartMsg();
        mHandler.sendEmptyMessageDelayed(START_ENGINE_MSG_ID, DELAY_TIME);
    }

    private void delayToSendRestartMsg() {
        removeStartMsg();
        removeRestartMsg();
        mHandler.sendEmptyMessageDelayed(RESTART_ENGINE_MSG_ID, DELAY_TIME);
    }

    private void removeStartMsg() {
        mHandler.removeMessages(START_ENGINE_MSG_ID);
    }

    private void removeRestartMsg() {
        mHandler.removeMessages(RESTART_ENGINE_MSG_ID);
    }


    @Override
    public boolean startEngine() {
        awakeWorkThread();
        return true;
    }

    @Override
    public boolean stopEngine() {
        mWorkThread.setParam("", "");
        exitWorkThread();
        return true;
    }

    @Override
    public boolean restartEngine() {
        String friendName = DlnaUtils.getDevName(this);
        mWorkThread.setParam(friendName, "");
        if (mWorkThread.isAlive()) {
            mWorkThread.restartEngine();
        } else {
            mWorkThread.start();
        }
        return true;
    }

    private void awakeWorkThread() {
        String friendName = DlnaUtils.getDevName(this);
        mWorkThread.setParam(friendName, "");


        if (mWorkThread.isAlive()) {
            mWorkThread.awakeThread();
        } else {
            mWorkThread.start();
        }
    }

    private void exitWorkThread() {
        if (mWorkThread != null && mWorkThread.isAlive()) {
            mWorkThread.exit();
            long time1 = System.currentTimeMillis();
            while (mWorkThread.isAlive()) {
                try {
                    Thread.sleep(100);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
            long time2 = System.currentTimeMillis();
            log.e("exitWorkThread cost time:" + (time2 - time1));
            mWorkThread = null;
        }
    }


}
