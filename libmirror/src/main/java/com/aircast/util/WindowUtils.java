package com.aircast.util;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.util.DisplayMetrics;

/**
 * @author fxw
 * @since 1.0
 */
public class WindowUtils {

    public static DisplayMetrics getWindowMetrics(Activity activity) {
        DisplayMetrics dm = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(dm);
        if (dm.heightPixels != 1080 && dm.heightPixels != 720 && dm.heightPixels != 2160)
            dm.heightPixels += getNavigationBarHeight(activity);
        return dm;
    }

    //获取NavigationBar的高度
    public static int getNavigationBarHeight(Context context) {
        Resources resources = context.getResources();
        try {
            int resourceId = resources.getIdentifier("navigation_bar_height", "dimen", "android");
            if (resourceId <= 0) {
                return 0;
            }
            int height = resources.getDimensionPixelSize(resourceId);
            return height;
        } catch (Exception e) {
            return 0;
        }
    }

}
