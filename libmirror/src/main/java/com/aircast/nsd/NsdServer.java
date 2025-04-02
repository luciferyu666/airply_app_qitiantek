package com.aircast.nsd;

import android.content.Context;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdServiceInfo;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

public class NsdServer {

    private static final String TAG = "NsdServer";
    public ServerCallBack callBack;
    private NsdManager.RegistrationListener mRegistrationListener;
    private NsdManager mNsdManager;

    public NsdServer(ServerCallBack callBack) {
        this.callBack = callBack;
    }


    public void register(Context app, int port, String serviceName, String serviceType, HashMap<String, String> map) {
        mNsdManager = (NsdManager) app.getSystemService(Context.NSD_SERVICE);
        NsdServiceInfo serviceInfo = new NsdServiceInfo();
        serviceInfo.setServiceName(serviceName);
        serviceInfo.setServiceType(serviceType);
        serviceInfo.setPort(port);//port must be >0

        if (map != null) {
            for (Map.Entry<String, String> m : map.entrySet()) {
                serviceInfo.setAttribute(m.getKey(), m.getValue());
            }
        } else {
            Log.e(TAG, "params require sdk 21");
        }


        mRegistrationListener = new NsdManager.RegistrationListener() {
            @Override
            public void onServiceRegistered(NsdServiceInfo serviceInfo) {
                String mServiceName = serviceInfo.getServiceName();
                NsdServer.this.callBack.onSuccess(mServiceName);
            }

            @Override
            public void onRegistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
                NsdServer.this.callBack.onError(serviceInfo.getServiceName(), String.valueOf(errorCode));
            }

            @Override
            public void onServiceUnregistered(NsdServiceInfo arg0) {
                Log.d(TAG, "onServiceUnregistered() : arg0 = [" + arg0 + "]");
            }

            @Override
            public void onUnregistrationFailed(NsdServiceInfo serviceInfo, int errorCode) {
                Log.d(TAG, "onUnregistrationFailed() : serviceInfo = [" + serviceInfo + "], errorCode = [" + errorCode + "]");
            }
        };
        mNsdManager.registerService(serviceInfo, NsdManager.PROTOCOL_DNS_SD, mRegistrationListener);
    }


    public void destroy() {
        if (mNsdManager != null)
            mNsdManager.unregisterService(mRegistrationListener);
    }
}