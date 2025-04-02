package com.aircast.source;

import android.util.Log;

import tv.danmaku.ijk.media.player.misc.IMediaDataSource;


public class AirplayMirrorSource implements IMediaDataSource {
    private static final String TAG = "AirplayMirrorSource";
    private volatile boolean eof;

    public AirplayMirrorSource() {
        Log.d(TAG, "AirplayMirrorSource()  ");
        eof = false;
        initNative();
    }

    @Override
    public int readAt(long position, byte[] buffer, int offset, int size) {
        if (size == 0) {
            return 0;
        }

        if (eof) {
            return -1;
        }

        int len = read264Stream(position, buffer, offset, size);
        //Log.d(TAG, "readAt() position = [" + position +  "], offset = [" + offset + "], size = [" + size + "]" + len );
        return len;
    }

    @Override
    public long getSize() {
        Log.d(TAG, " getSize()  ");
        return -1; // is_streamed = 1;
    }

    @Override
    public void close() {
        Log.d(TAG, " close()  ");
        closeStream();
    }

    @Override
    public void shutdown() {
        Log.d(TAG, " shutdown()  ");
    }

    //jni call
    public void streamClosed() {
        Log.d(TAG, " streamClosed()  ");
        eof = true;
    }

    //jni call
    public void streamOpened() {
        Log.d(TAG, " streamOpen() called ");
        eof = true;
    }


    public native int read264Stream(long position, byte[] buffer, int offset, int size);

    public native int readableSize(int size);

    public native void closeStream();

    public native void initNative();
}
