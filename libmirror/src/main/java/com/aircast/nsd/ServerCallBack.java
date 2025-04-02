package com.aircast.nsd;

public interface ServerCallBack {
    void onSuccess(String serviceName);

    void onError(String serviceName, String error);
}