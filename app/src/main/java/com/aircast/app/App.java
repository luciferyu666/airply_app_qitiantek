package com.aircast.app;

import android.app.Application;
import android.content.Context;

import com.aircast.AirplayApp;


public class App extends Application {
    private static App mInstance;
    private AirplayApp airplayApp;

    public static Context getContext() {
        return mInstance.getApplicationContext();
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mInstance = this;

        airplayApp = new AirplayApp();
        airplayApp.onCreate(getApplicationContext());
    }

}
