package com.aircast.util;

import android.content.Context;

import com.aircast.center.DlnaMediaModel;
import com.aircast.jni.PlatinumReflection;
import com.aircast.settings.Setting;

import java.util.regex.Matcher;
import java.util.regex.Pattern;


public class DlnaUtils {

    public final static String DLNA_OBJECTCLASS_MUSICID = "object.item.audioItem";
    public final static String DLNA_OBJECTCLASS_VIDEOID = "object.item.videoItem";
    public final static String DLNA_OBJECTCLASS_PHOTOID = "object.item.imageItem";
    public final static String DLNA_OBJECTCLASS_SCREEN = "object.item.screenItem";
    private static final CommonLog log = LogFactory.createLog();

    public static String getDevName(Context context) {
        return Setting.getInstance().getName();
    }

    public static int parseSeekTime(String data) throws Exception {

        int seekPos = 0;

        String[] seektime = data.split("=");
        if (2 != seektime.length) {
            return seekPos;
        }
        String timetype = seektime[0];
        String position = seektime[1];
        if (PlatinumReflection.MEDIA_SEEK_TIME_TYPE_REL_TIME.equals(timetype)) {
            seekPos = convertSeekRelTimeToMs(position);
        } else {
            log.e("timetype = " + timetype + ", position = " + position);
        }


        return seekPos;
    }

    public static int convertSeekRelTimeToMs(String reltime) {
        int sec = 0;
        int ms = 0;
        String[] times = reltime.split(":");
        if (3 != times.length)
            return 0;
        if (!isNumeric(times[0]))
            return 0;
        int hour = Integer.parseInt(times[0]);
        if (!isNumeric(times[1]))
            return 0;
        int min = Integer.parseInt(times[1]);
        String[] times2 = times[2].split("\\.");
        if (2 == times2.length) {//00:00:00.000
            if (!isNumeric(times2[0]))
                return 0;
            sec = Integer.parseInt(times2[0]);
            if (!isNumeric(times2[1]))
                return 0;
            ms = Integer.parseInt(times2[1]);
        } else if (1 == times2.length) {//00:00:00
            if (!isNumeric(times2[0]))
                return 0;
            sec = Integer.parseInt(times2[0]);
        }
        return (hour * 3600000 + min * 60000 + sec * 1000 + ms);
    }

    public static boolean isNumeric(String str) {
        if ("".equals(str))
            return false;
        Pattern pattern = Pattern.compile("[0-9]*");
        Matcher isNum = pattern.matcher(str);
        if (!isNum.matches()) {
            return false;
        }
        return true;
    }

    public static String formatTimeFromMSInt(int time) {
        String hour = "00";
        String min = "00";
        String sec = "00";
        String split = ":";
        int tmptime = time;
        int tmp = 0;
        if (tmptime >= 3600000) {
            tmp = tmptime / 3600000;
            hour = formatHunToStr(tmp);
            tmptime -= tmp * 3600000;
        }
        if (tmptime >= 60000) {
            tmp = tmptime / 60000;
            min = formatHunToStr(tmp);
            tmptime -= tmp * 60000;
        }
        if (tmptime >= 1000) {
            tmp = tmptime / 1000;
            sec = formatHunToStr(tmp);
            tmptime -= tmp * 1000;
        }

        String ret = hour + split + min + split + sec;
        return ret;
    }

    private static String formatHunToStr(int hun) {
        hun = hun % 100;
        if (hun > 9)
            return ("" + hun);
        else
            return ("0" + hun);
    }

    public static String formateTime(long millis) {
        String str = "";
        int hour = 0;
        int time = (int) (millis / 1000);
        int second = time % 60;
        int minute = time / 60;
        if (minute >= 60) {
            hour = minute / 60;
            minute %= 60;
            str = String.format("%02d:%02d:%02d", hour, minute, second);
        } else {
            str = String.format("%02d:%02d", minute, second);
        }


        return str;

    }

    public static boolean isAudioItem(DlnaMediaModel item) {
        String objectClass = item.getObjectClass();
        if (objectClass.contains(DLNA_OBJECTCLASS_MUSICID)) {
            return true;
        }

        return false;
    }

    public static boolean isVideoItem(DlnaMediaModel item) {
        String objectClass = item.getObjectClass();
        if (objectClass.contains(DLNA_OBJECTCLASS_VIDEOID)) {
            return true;
        }
        return false;
    }

    public static boolean isImageItem(DlnaMediaModel item) {
        String objectClass = item.getObjectClass();
        if (objectClass.contains(DLNA_OBJECTCLASS_PHOTOID)) {
            return true;
        }
        return false;
    }

    public static boolean isScreenItem(DlnaMediaModel item) {
        String objectClass = item.getObjectClass();
        if (objectClass.contains(DLNA_OBJECTCLASS_SCREEN)) {
            return true;
        }
        return false;
    }

}
