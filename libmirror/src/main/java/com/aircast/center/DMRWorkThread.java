package com.aircast.center;

import android.content.Context;
import android.util.Log;

import com.aircast.AirplayApp;
import com.aircast.jni.PlatinumJniProxy;
import com.aircast.util.Action;
import com.aircast.util.CommonLog;
import com.aircast.util.LogFactory;

public class DMRWorkThread extends Thread implements IBaseEngine {
    private static final String TAG = "DMRWorkThread";
    private static final CommonLog log = LogFactory.createLog();

    private static final int CHECK_INTERVAL = 7 * 1000;

    private Context mContext = null;
    private boolean mStartSuccess = false;
    private boolean mExitFlag = false;

    private String mFriendName = "";
    private String mUUID = "";
    private AirplayApp mApplication;


    public DMRWorkThread(Context context) {
        mContext = context;
        mApplication = AirplayApp.getInstance();
    }

    public void setFlag(boolean flag) {
        mStartSuccess = flag;
    }

    public void setParam(String friendName, String uuid) {
        mFriendName = friendName;
        mUUID = uuid;
        mApplication.updateDevInfo(mFriendName, mUUID);
    }

    public void awakeThread() {
        synchronized (this) {
            notifyAll();
        }
    }

    public void exit() {
        mExitFlag = true;
        awakeThread();
    }

    @Override
    public void run() {

        log.e("DMRWorkThread run...");

        while (true) {
            if (mExitFlag) {
                stopEngine();
                break;
            }
            refreshNotify();
            synchronized (this) {
                try {
                    wait(CHECK_INTERVAL);
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
            if (mExitFlag) {
                stopEngine();
                break;
            }
        }

        log.e("DMRWorkThread over...");

    }

    public void refreshNotify() {
        //if (!CommonUtil.checkNetworkState(mContext)){
        //	return ;
        //}

        if (!mStartSuccess) {
            stopEngine();
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            boolean ret = startEngine();
            if (ret) {
                mStartSuccess = true;
            }
        }

    }


    @Override
    public boolean startEngine() {
        if (mFriendName.length() == 0) {
            return false;
        }
        boolean result = false;

        int ret = PlatinumJniProxy.startMediaRender_Java(mFriendName);
        result = (ret == 0 ? true : false);
        mApplication.setDevStatus(result);

        Log.d(TAG, "airplay() ret  : " + ret + " port:" +  PlatinumJniProxy.getAirplayPort());
        Action.broadcast(PlatinumJniProxy.MDNS_REG_AIRPLAY);

        return result;
    }

    @Override
    public boolean stopEngine() {
        log.d("stopEngine()  ");

        PlatinumJniProxy.stopMediaRender();
        mApplication.setDevStatus(false);

        return true;
    }

    @Override
    public boolean restartEngine() {
        setFlag(false);
        awakeThread();
        return true;
    }

}
