package com.aircast.util;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.provider.Settings;
import android.util.Log;

import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.util.Collections;
import java.util.Enumeration;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.provider.Settings;
import java.net.NetworkInterface;
import java.util.Collections;
import java.util.Random;

public class NetUtils {
    public static final String UNKNOWN = "unknown";
    private static final String TAG = "NetUtils";

    public static String getIpAddress() {
        InetAddress wlan = NetUtils.getIpAddress("wlan");
        if (wlan != null)
            return wlan.getHostAddress();

        InetAddress eth = NetUtils.getIpAddress("eth");
        if (eth != null)
            return eth.getHostAddress();

        return UNKNOWN;
    }


    public static String getWifiMacAddress(Context context) {
        String macAddress = "";

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            // For Android 8.0 and above
            try {
                NetworkInterface networkInterface = NetworkInterface.getByName("wlan0");
                if (networkInterface != null) {
                    byte[] hardwareAddress = networkInterface.getHardwareAddress();
                    if (hardwareAddress != null) {
                        StringBuilder builder = new StringBuilder();
                        for (byte b : hardwareAddress) {
                            builder.append(String.format("%02X:", b));
                        }
                        if (builder.length() > 0) {
                            builder.deleteCharAt(builder.length() - 1);
                        }
                        macAddress = builder.toString();
                    }
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            // For Android 6.0 to 7.1
            try {
                // Try getting MAC from NetworkInterface first
                for (NetworkInterface networkInterface : Collections.list(NetworkInterface.getNetworkInterfaces())) {
                    if (networkInterface.getName().equalsIgnoreCase("wlan0")) {
                        byte[] hardwareAddress = networkInterface.getHardwareAddress();
                        if (hardwareAddress != null) {
                            StringBuilder builder = new StringBuilder();
                            for (byte b : hardwareAddress) {
                                builder.append(String.format("%02X:", b));
                            }
                            if (builder.length() > 0) {
                                builder.deleteCharAt(builder.length() - 1);
                            }
                            macAddress = builder.toString();
                            break;
                        }
                    }
                }

                // If MAC is still empty, try getting Android ID as fallback
                if (macAddress.isEmpty()) {
                    macAddress = Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        } else {
            // For Android 5.1 and below
            WifiManager wifiManager = (WifiManager) context.getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (wifiManager != null) {
                macAddress = wifiManager.getConnectionInfo().getMacAddress();
            }
        }


        String cleanMac = macAddress.replace(":", "");
        if (cleanMac.length() >= 4) {
            return cleanMac.substring(cleanMac.length() - 4);
        }

        int s = new Random().nextInt(900) + 100;
        return String.valueOf(s);
    }

    public static boolean isOnline(Context context) {
        ConnectivityManager cm = (ConnectivityManager) context
                .getSystemService(Context.CONNECTIVITY_SERVICE);

        return cm.getActiveNetworkInfo() != null
                && cm.getActiveNetworkInfo().isConnected();
    }

    private static InetAddress getIpAddress(String iface) {  //wlan  eth
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                //Log.d(TAG, "NetworkInterface  " + intf.getName());
                if (intf.getName().contains(iface)) {
                    for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                        InetAddress inetAddress = enumIpAddr.nextElement();
                        if (!inetAddress.isLoopbackAddress() && inetAddress instanceof Inet4Address) {
                            Log.d(TAG, "intf.getName()  " + inetAddress.getHostAddress());
                            return inetAddress;
                        }
                    }
                }
            }
        } catch (IOException ex) {
            ex.printStackTrace();
        }

        Log.d(TAG, "getIpAddress return null  ");
        return null;
    }


    private static boolean isSSIDEmpty(String ssid) {
        return ssid == null || ssid.contains("unknown ssid") || ssid.equals("0x");
    }

    public static String getWIFISSID(Context ctx) {
        String ssid = null;
        WifiManager wm = (WifiManager) ctx.getSystemService(Context.WIFI_SERVICE);
        WifiInfo info = null;
        if (wm != null) {
            info = wm.getConnectionInfo();
        }

        if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.O || Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            if (info != null) {
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
                    ssid = info.getSSID();
                } else {
                    ssid = info.getSSID().replace("\"", "");
                }
            }
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1 && isSSIDEmpty(ssid)) {
            ConnectivityManager cm = (ConnectivityManager) ctx.getSystemService(Context.CONNECTIVITY_SERVICE);
            if (cm != null) {
                NetworkInfo networkInfo = cm.getActiveNetworkInfo();
                if (networkInfo != null && networkInfo.isConnected()) {
                    if (networkInfo.getExtraInfo() != null) {
                        return networkInfo.getExtraInfo().replace("\"", "");
                    }
                }
            }
        }

        return isSSIDEmpty(ssid) ? "有线网络" : ssid.replace("\"", "");
    }

}
