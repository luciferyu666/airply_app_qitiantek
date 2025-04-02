package com.aircast.nsd;

import android.content.Context;
import android.util.Log;

import com.aircast.jni.PlatinumJniProxy;
import com.aircast.settings.Setting;

import java.util.HashMap;

public class NsdHelper implements ServerCallBack {
    private static final String TAG = "NsdHelper";
    private static final String FEATURES = "0x527FFFF7"; //"0x5A7FFFF7,0x1E";
    private static final String FEATURES_YOUTUBE = "0x527FFFE4"; //mirror mode

    private final NsdServer nsdAirplay;
    private final NsdServer nsdRaop;
    private final Context context;

    public NsdHelper(Context ctx) {
        context = ctx;
        nsdAirplay = new NsdServer(this);
        nsdRaop = new NsdServer(this);
    }

    public void registerAirplay() {
        HashMap<String, String> txtRecord = new HashMap<>(20);
        Log.d(TAG, "registerAirplay port = " + PlatinumJniProxy.getAirplayPort() + ", mMacAddress = " + Setting.getInstance().getHwaddr());
        txtRecord.put("deviceid", Setting.getInstance().getHwaddr());
        txtRecord.put("features", FEATURES_YOUTUBE);   //0x5A7FFFF7,0x1E
        txtRecord.put("srcvers", "220.68");
        txtRecord.put("flags", "0x4");
        txtRecord.put("vv", "2");
        txtRecord.put("model", "AppleTV3,1");
        txtRecord.put("pw", "0");
        //txtRecord.put("rhd", "5.6.0.0");
        txtRecord.put("pk", "11c18e46fcd95587a70c9bd6e4a64a593c789cdd14c0ec8318d2651b43290eaa");
        txtRecord.put("pi", "b08f5a79-db29-4384-b456-a4784d9e6055");
        nsdAirplay.register(context, PlatinumJniProxy.getAirplayPort(), Setting.getInstance().getName(), "_airplay._tcp", txtRecord);
    }

    public void registerRaop() {
        Log.d(TAG, "registerRaop port = " + PlatinumJniProxy.getRaopPort());
        HashMap<String, String> txtRecord = new HashMap<>(30);
        txtRecord.put("ch", "2");
        txtRecord.put("cn", "0,1,3");
        txtRecord.put("da", "true");
        txtRecord.put("et", "0,3,5");
        txtRecord.put("ek", "1");
        txtRecord.put("vv", "2");
        txtRecord.put("ft", FEATURES_YOUTUBE);
        txtRecord.put("am", "AppleTV3,1");
        txtRecord.put("md", "0,1,2");
        //txtRecord.put("rhd", "5.6.0.0");
        txtRecord.put("pw", "false");
        txtRecord.put("sm", "false");
        txtRecord.put("sr", "44100");
        txtRecord.put("ss", "16");
        txtRecord.put("sv", "false");
        txtRecord.put("tp", "UDP");
        txtRecord.put("txtvers", "1");
        txtRecord.put("sf", "0x4");
        txtRecord.put("vs", "220.68");
        txtRecord.put("vn", "3");
        txtRecord.put("pk", "11c18e46fcd95587a70c9bd6e4a64a593c789cdd14c0ec8318d2651b43290eaa");
        String serviceName = Setting.getInstance().getHwaddr().replace(":", "") + "@" + Setting.getInstance().getName();
        nsdRaop.register(context, PlatinumJniProxy.getRaopPort(),serviceName,
                "_raop._tcp", txtRecord);
    }

    public void destroy() {
        Log.d(TAG, "destroy() ");
        if (nsdAirplay != null) {
            nsdAirplay.destroy();
        }
        if (nsdRaop != null) {
            nsdRaop.destroy();
        }
    }


    @Override
    public void onSuccess(String serviceName) {
        Log.d(TAG, "onSuccess() : serviceName = [" + serviceName + "]");
    }

    @Override
    public void onError(String serviceName, String error) {
        Log.d(TAG, "onError() : serviceName = [" + serviceName + "], error = [" + error + "]");
    }
}