package tv.danmaku.ijk.media.player;

import android.util.Log;

import java.lang.reflect.Method;

public class Platform {
    private static final String TAG = "Platform";

    private static String hw = "";

    public static String hardware() {
        if ("".equals(hw)) {
            hw = getSystemProperty("ro.hardware");
            hw = hw.toLowerCase();
        }

        Log.d(TAG, "ro.hardware: " + hw );
        return hw;
    }

    public static boolean isAmlogic () {
        return hardware().contains("amlogic");
    }

    public static String getSystemProperty(String key) {
        String value = null;

        try {
            value = (String) Class.forName("android.os.SystemProperties")
                    .getMethod("get", String.class).invoke(null, key);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return value;
    }


    /**
     * Set the value for the given key.
     *
     * @throws IllegalArgumentException if the key exceeds 32 characters
     * @throws IllegalArgumentException if the value exceeds 92 characters
     */
    public static void set(String key, String val) throws IllegalArgumentException {

        try {
            Class<?> SystemProperties = Class.forName("android.os.SystemProperties");

            //Parameters Types
            @SuppressWarnings("rawtypes")
            Class[] paramTypes = { String.class, String.class };
            Method set = SystemProperties.getMethod("set", paramTypes);

            //Parameters
            Object[] params = { key, val };
            set.invoke(SystemProperties, params);
        } catch (IllegalArgumentException iAE) {
            throw iAE;
        } catch (Exception e) {
            //TODO
        }

    }

}
