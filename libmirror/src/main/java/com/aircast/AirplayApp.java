package com.aircast;

import android.content.Context;

import com.aircast.center.DeviceUpdateBrocastFactory;
import com.aircast.settings.Setting;
import com.aircast.util.Action;
import com.aircast.util.DeviceInfo;

public class AirplayApp {
    private static AirplayApp mInstance;
    private static Context context;
    private DeviceInfo mDeviceInfo;

    public static Context getContext() {
        return AirplayApp.context;
    }

    public static AirplayApp getInstance() {
        return mInstance;
    }

    public void onCreate(Context appContext) {
        Action.init(appContext);
        context = appContext;
        mInstance = this;

        mDeviceInfo = new DeviceInfo();
        Setting.getInstance().init(appContext);
    }

    public void updateDevInfo(String name, String uuid) {
        mDeviceInfo.dev_name = name;
        mDeviceInfo.uuid = uuid;
    }

    public void setDevStatus(boolean flag) {
        mDeviceInfo.status = flag;
        DeviceUpdateBrocastFactory.sendDevUpdateBrocast(context);
    }

    public DeviceInfo getDevInfo() {
        return mDeviceInfo;
    }
}
