package com.aircast.music.lrc;

import com.aircast.util.CommonLog;
import com.aircast.util.LogFactory;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;


public class LrcDownLoadHelper {

    private static final CommonLog log = LogFactory.createLog();

    private final static int THREAD_COUNT = 3;
    private ExecutorService mExecutorService;

    public LrcDownLoadHelper() {

    }

    public void init() {
        if (mExecutorService == null) {
            mExecutorService = Executors.newFixedThreadPool(THREAD_COUNT);
        }
    }


    public void unInit() {
        if (mExecutorService != null) {
            mExecutorService.shutdown();
            mExecutorService.shutdownNow();
            mExecutorService = null;
        }
    }

    public boolean syncDownLoadLRC(String song, String artist, ILRCDownLoadCallback callback) {
        if (mExecutorService == null) {
            return false;
        }

        InnerRunnable runnable = new InnerRunnable(song, artist, callback);
        mExecutorService.execute(runnable);

        return true;

    }


    public static interface ILRCDownLoadCallback {
        public void lrcDownLoadComplete(final boolean isSuccess, String song, String artist);
    }

    class InnerRunnable implements Runnable {

        private String mSong = "";
        private String mArtist = "";
        private ILRCDownLoadCallback mCallback;

        public InnerRunnable(String song, String artist, ILRCDownLoadCallback callback) {
            mSong = song;
            mArtist = artist;
            mCallback = callback;
        }

        @Override
        public void run() {
            boolean ret = LyricHelper.searchLryics(mSong, mArtist);
            log.e(" LyricHelper.searchLryics mSong = " + mSong + ", mArtist = " + mArtist + ", ret = " + ret);

            if (mCallback != null) {
                mCallback.lrcDownLoadComplete(ret, mSong, mArtist);
            }

        }

    }


}
