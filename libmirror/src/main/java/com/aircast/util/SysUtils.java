package com.aircast.util;

import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.os.Build;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.SystemClock;
import android.provider.Settings;

import java.lang.reflect.Method;

public class SysUtils {
    public static void setOriVolume(Context context, int mode) {
        setOriVolume(context, mode, getOrigVolume(context));
    }

    public static void setOriVolume(Context context, int mode, int value) {
        if (mode == 1) {
            setVolume(context, mode, value);
        } else {
            storeVolumeValue(context, 1, getOrigVolume(context));
            setVolume(context, mode, value);
        }
    }

    public static void setVolume(Context context, int mode) {
        setVolume(context, mode, getVolumeValue(context, mode));
    }

    public static void setVolume(Context context, int mode, int value) {
        setVolumes(context, value);
        storeVolumeValue(context, mode, value);
    }

    public static int getVolume(Context context, int mode) {
        return getVolumeValue(context, mode);
    }

    public static void setVolumes(Context context, int value) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        int originVolume = audioManager.getStreamVolume(3);
        if (value == originVolume)
            return;
        if (value > audioManager.getStreamMaxVolume(3))
            value = audioManager.getStreamMaxVolume(3);
        audioManager.setStreamVolume(3, value, 1);
    }

    public static void volumeAdjust(Context context, int mode, int option) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        if (audioManager != null) {
            if (option == 1) {
                audioManager.adjustSuggestedStreamVolume(1, 3, 9);
                int volume = audioManager.getStreamVolume(3);
                storeVolumeValue(context, mode, volume);
            }
            if (option == -1) {
                audioManager.adjustSuggestedStreamVolume(-1, 3, 9);
                int volume = audioManager.getStreamVolume(3);
                storeVolumeValue(context, mode, volume);
            }
        }
    }

    public static void storeVolumeValue(Context context, int mode, int value) {
        SharedPreferences settings = context.getSharedPreferences("com.rockchip.mediacenter", 0);
        SharedPreferences.Editor editor = settings.edit();
        switch (mode) {
            case 1:
                editor.putInt("sysVolumeValue", value);
                editor.commit();
                break;
            case 2:
                editor.putInt("userVolumeValue", value);
                editor.commit();
                break;
        }
    }

    public static int getVolumeValue(Context context, int mode) {
        int volume = 0;
        SharedPreferences settings = context.getSharedPreferences("com.rockchip.mediacenter", 0);
        switch (mode) {
            case 1:
                volume = settings.getInt("sysVolumeValue", 0);
                if (volume == 0)
                    volume = getOrigVolume(context);
                break;
            case 2:
                volume = settings.getInt("userVolumeValue", 0);
                if (volume == 0)
                    volume = getOrigVolume(context);
                break;
        }
        return volume;
    }

    public static int getOrigVolume(Context context) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        int volume = audioManager.getStreamVolume(3);
        return volume;
    }

    public static void setbackVolume(Context context, int mode) {
        switch (mode) {
            case 2:
                setVolumes(context, getVolumeValue(context, 1));
                break;
        }
    }

    public static void setMute(Context context, boolean mute) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        int volume = audioManager.getStreamVolume(3);
        if (mute) {
            if (isMute(context))
                return;
            storeVolumeValue(context, 1, volume);
            audioManager.setStreamVolume(3, 0, 1);
        } else {
            if (!isMute(context))
                return;
            volume = getVolumeValue(context, 1);
            if (volume == 0)
                volume = 7;
            audioManager.setStreamVolume(3, volume, 1);
        }
    }

    public static boolean isMute(Context context) {
        AudioManager audio = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        return audio.getRingerMode() != AudioManager.RINGER_MODE_NORMAL;
    }

    public static void setScreenValue(Context context, int mode) {
        SharedPreferences settings = context.getSharedPreferences("com.rockchip.mediacenter", 0);
        SharedPreferences.Editor editor = settings.edit();
        editor.putInt("VideoscreenSize", mode);
        editor.commit();
    }

    public static int getScreenValue(Context context) {
        SharedPreferences settings = context.getSharedPreferences("com.rockchip.mediacenter", 0);
        return settings.getInt("VideoscreenSize", 3);
    }

    public static interface Def {
        public static final int MODE_SYSTEM = 1;

        public static final int MODE_USER = 2;

        public static final int VOLUMEPLUS = 1;

        public static final int VOLUMEMINUS = -1;
    }

    public static final class PowerManagerUtil {
        private static final String TAG = "PowerManagerUtil";

        private PowerManager mPowerManager;

        private PowerManager.WakeLock mWakeLock;

        public PowerManagerUtil(Context context) {
            this(context, 10);
        }

        public PowerManagerUtil(Context context, int flags) {
            this.mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            this.mWakeLock = this.mPowerManager.newWakeLock(flags, "Power:ManagerUtil");
        }

        public void acquireWakeLock() {
            if (this.mWakeLock != null)
                try {
                    if (!this.mWakeLock.isHeld())
                        this.mWakeLock.acquire();
                } catch (Exception e) {
                    e.printStackTrace();
                }
        }

        public void releaseWakeLock() {
            if (this.mWakeLock != null)
                try {
                    if (this.mWakeLock.isHeld()) {
                        this.mWakeLock.release();
                        this.mWakeLock.setReferenceCounted(false);
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
        }

        public boolean isScreenOn() {
            return this.mPowerManager.isScreenOn();
        }

        public void wakeUp() {
            long time = SystemClock.uptimeMillis();
            ReflectionUtils.invokeMethod(this.mPowerManager, "wakeUp", new Class[]{long.class}, new Object[]{Long.valueOf(time)});
        }
    }

    public static final class BrightnessUtil {
        public static void saveCurrentBrightness(Context context) {
            saveSystemBrightness(context);
            setUserBrightness(context);
        }

        public static void restoreCurrentBrightness(Context context) {
            setSystemBrightness(context);
        }

        public static void saveSystemBrightness(Context context) {
            saveSystemBrightness(context, getBrightness(context));
        }

        public static void saveSystemBrightness(Context context, int brightness) {
            SharedPreferences settings = context.getSharedPreferences("com.rockchip.mediacenter", 0);
            SharedPreferences.Editor editor = settings.edit();
            editor.putInt("sysBrightnessValue", brightness);
            editor.commit();
        }

        public static void saveUserBrightness(Context context) {
            saveUserBrightness(context, getUserBrightness(context));
        }

        public static void saveUserBrightness(Context context, int brightness) {
            SharedPreferences settings = context.getSharedPreferences("com.rockchip.mediacenter", 0);
            SharedPreferences.Editor editor = settings.edit();
            editor.putInt("userBrightnessValue", brightness);
            editor.commit();
        }

        public static void setUserBrightness(Context context) {
            setUserBrightness(context, getUserBrightness(context));
        }

        public static void setUserBrightness(Context context, int brightness) {
            setBrightness(context, brightness);
            saveUserBrightness(context, brightness);
        }

        public static int getUserBrightness(Context context) {
            SharedPreferences settings = context.getSharedPreferences("com.rockchip.mediacenter", 0);
            int brightness = settings.getInt("userBrightnessValue", 0);
            if (brightness == 0)
                brightness = getBrightness(context);
            return brightness;
        }

        public static void setSystemBrightness(Context context) {
            setSystemBrightness(context, getSystemBrightness(context));
        }

        public static void setSystemBrightness(Context context, int brightness) {
            setBrightness(context, brightness);
            saveSystemBrightness(context, brightness);
        }

        public static int getSystemBrightness(Context context) {
            SharedPreferences settings = context.getSharedPreferences("com.rockchip.mediacenter", 0);
            int brightness = settings.getInt("sysBrightnessValue", 0);
            if (brightness == 0)
                brightness = getBrightness(context);
            return brightness;
        }

        public static void setBrightness(Context context, int brightness) {
            try {
                Object powerService = ReflectionUtils.invokeStaticMethod("android.os.ServiceManager", "getService", new Object[]{"power"});
                Object powerManager = ReflectionUtils.invokeStaticMethod("android.os.IPowerManager$Stub", "asInterface", new Class[]{IBinder.class}, new Object[]{powerService});
                Method injectPointerEvent = null;
                if (Build.VERSION.SDK_INT >= 17) {
                    injectPointerEvent = ReflectionUtils.getMethod("android.os.IPowerManager", "setTemporaryScreenBrightnessSettingOverride", new Class[]{int.class});
                    Settings.System.putInt(context.getContentResolver(), "screen_brightness", brightness);
                } else {
                    injectPointerEvent = ReflectionUtils.getMethod("android.os.IPowerManager", "setBacklightBrightness", new Class[]{int.class});
                }
                injectPointerEvent.invoke(powerManager, new Object[]{Integer.valueOf(brightness)});
            } catch (Exception e) {
            }
        }

        public static int getBrightness(Context context) {
            int nowBrightnessValue = 0;
            ContentResolver resolver = context.getContentResolver();
            try {
                nowBrightnessValue = Settings.System.getInt(resolver, "screen_brightness");
            } catch (Exception e) {
                e.printStackTrace();
            }
            return nowBrightnessValue;
        }
    }
}
