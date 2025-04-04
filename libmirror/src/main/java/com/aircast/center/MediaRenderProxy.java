package com.aircast.center;

import android.content.Context;
import android.content.Intent;

import com.aircast.AirplayApp;
import com.aircast.service.MediaRenderService;
import com.aircast.util.CommonLog;
import com.aircast.util.LogFactory;

public class MediaRenderProxy implements IBaseEngine {

    private static final CommonLog log = LogFactory.createLog();

    private static MediaRenderProxy mInstance;
    private Context mContext;

    private MediaRenderProxy(Context context) {
        mContext = context;
    }

    public static synchronized MediaRenderProxy getInstance() {
        if (mInstance == null) {
            mInstance = new MediaRenderProxy(AirplayApp.getContext());
        }
        return mInstance;
    }

    @Override
    public boolean startEngine() {
        Intent intent = new Intent(MediaRenderService.START_RENDER_ENGINE);
        intent.setPackage(mContext.getPackageName());
        mContext.startService(intent);
        return true;
    }

    @Override
    public boolean stopEngine() {
        mContext.stopService(new Intent(mContext, MediaRenderService.class));
        return true;
    }

    @Override
    public boolean restartEngine() {
        Intent intent = new Intent(MediaRenderService.RESTART_RENDER_ENGINE);
        intent.setPackage(mContext.getPackageName());
        mContext.startService(intent);
        return true;
    }

}
