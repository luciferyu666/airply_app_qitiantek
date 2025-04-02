package com.aircast.source;

import android.net.Uri;
import android.util.Log;



import tv.danmaku.ijk.media.player.misc.IMediaDataSource;

public class Source {
    public static final String MIRROR_AIRPLAY = "air";
    private static final String TAG = "SourceUtil";

    public static IMediaDataSource makeSource(Uri uri) {
        String url = uri.toString();
        IMediaDataSource dataSource = null;

        switch (url) {
            case MIRROR_AIRPLAY:
                dataSource = new AirplayMirrorSource();
                break;

        }

        return dataSource;
    }

    public static int framedrop(Uri uriStream) {
        int drop = 3;
        Log.d(TAG, uriStream + "  framedrop " + drop);
        return drop;
    }

    public static boolean isMirror(Uri uri) {
        String url = uri.toString();
        return url.startsWith(MIRROR_AIRPLAY);
    }

}
