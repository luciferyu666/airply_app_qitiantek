package com.aircast.app.settings.util;

import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.content.Context;
import android.content.pm.PackageManager;
import android.view.View;
import android.view.animation.OvershootInterpolator;

import com.aircast.app.App;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Utils {

    public static int dimOffset(int i) {
        try {
            return App.getContext().getResources().getDimensionPixelOffset(i);
        } catch (Exception e) {

            return 0;
        }
    }

    public static int regex(String str) {
        Matcher matcher = Pattern.compile("[\\u4e00-\\u9fa5]").matcher(str);
        int i = 0;
        while (matcher.find()) {
            int i2 = 0;
            while (i2 <= matcher.groupCount()) {
                i2++;
                i++;
            }
        }
        return i;
    }


    public static int a(Context context, float f) {
        return (int) ((context.getResources().getDisplayMetrics().density * f) + 0.5f);
    }

    public static void anim3(View view) {
        anim(view, 1.08f, 250);
    }

    public static String getStr(int i) {
        try {
            return App.getContext().getString(i);
        } catch (Exception e) {

            return "";
        }
    }

    public static void anim1(View view) {
        anim(view, 1.0f, 250);
    }

    public static void anim2(View view, float f) {
        anim(view, f, 250);
    }

    public static void anim(View view, float f, long j) {
        if (view != null) {
            ObjectAnimator ofFloat = ObjectAnimator.ofFloat(view, "scaleX", new float[]{f});
            ObjectAnimator ofFloat2 = ObjectAnimator.ofFloat(view, "scaleY", new float[]{f});
            AnimatorSet animatorSet = new AnimatorSet();
            animatorSet.setDuration(j);
            animatorSet.play(ofFloat).with(ofFloat2);
            animatorSet.setInterpolator(new OvershootInterpolator());
            animatorSet.start();
        }
    }


    public static int getVerCode() {
        int verCode = 0;
        try {
            verCode = App.getContext().getPackageManager().getPackageInfo(App.getContext().getPackageName(), 0).versionCode;
        } catch (PackageManager.NameNotFoundException e) {

        }

        return verCode;
    }

    public static boolean needUpdate() {
        return true;
    }

    public static String byteArrayToHexString(final byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x ", b & 0xff));
        }
        return sb.toString();
    }
}
