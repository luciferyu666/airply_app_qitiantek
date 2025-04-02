package com.aircast.video;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Color;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

import com.aircast.R;
import com.aircast.center.DLNAGenaEventBrocastFactory;
import com.aircast.center.DlnaMediaModel;
import com.aircast.center.DlnaMediaModelFactory;
import com.aircast.center.MediaControlBrocastFactory;
import com.aircast.player.AbstractTimer;
import com.aircast.player.SingleSecondTimer;
import com.aircast.util.SysUtils;
import com.aircast.util.ToastUtil;
import com.cicada.player.CicadaPlayer;

import com.cicada.player.bean.ErrorInfo;
import com.cicada.player.bean.InfoBean;
import com.cicada.player.bean.InfoCode;


public class CicadaVideoPlayer extends Activity implements MediaControlBrocastFactory.IMediaControlListener,
        CicadaPlayer.OnErrorListener, CicadaPlayer.OnCompletionListener, CicadaPlayer.OnSeekCompleteListener,
        CicadaPlayer.OnInfoListener, CicadaPlayer.OnLoadingStatusListener {
    private static final String TAG = "CicadaVideoPlayer";
    private final static int REFRESH_CURPOS = 0x0001;
    private final static int HIDE_TOOL = 0x0002;
    private final static int EXIT_ACTIVITY = 0x0003;
    private final static int REFRESH_SPEED = 0x0004;
    private final static int CHECK_DELAY = 0x0005;
    private final static int EXIT_DELAY_TIME = 1000;
    private final static int HIDE_DELAY_TIME = 3000;
    private static final int DIALOG_PLAY_MODE = 1;

    private CicadaVodPlayerView mVideoView;

    private MediaController mMediaController;
    private SysUtils.PowerManagerUtil mPowerManagerUtil;
    private View mProgressZone;
    private TextView mProgressText;

    private boolean isPlayingWhenPopup = false;

    private boolean hasBufferStarted;
    private boolean isActivityVisible = false;
    private MediaControlBrocastFactory mMediaControlBorcastFactory;
    private int mVideoPosition;
    private long exitTime;
    private AbstractTimer mPlayPosTimer;
    private Handler mHandler;
    private Context mContext;
    private String videoPath;

    //在 safari 播放视频全屏开始时, 会发送 stop , 需要忽略掉
    private long startPlayVideoTime;

    //停止视频播放器
    private Runnable mStopRunnable = new Runnable() {
        @Override
        public void run() {
            CicadaVideoPlayer.this.finish();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate() : ");
        super.onCreate(savedInstanceState);
        mContext = getApplicationContext();

        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        isActivityVisible = true;

        setContentView(R.layout.ap_airplay_cicada_video_view);

        mVideoView = findViewById(R.id.videoview);
        mVideoView.setKeepScreenOn(true);
        mProgressZone = findViewById(R.id.progress_indicator);
        mProgressText = findViewById(R.id.progress_text);
        View controlView = findViewById(R.id.layout_controller);
        mMediaController = new MediaController(this);
        mMediaController.setControlView(controlView);
        mVideoView.setMediaController(mMediaController);
        mVideoView.setActivity(this);
        mPowerManagerUtil = new SysUtils.PowerManagerUtil(CicadaVideoPlayer.this);
        mPowerManagerUtil.acquireWakeLock();
        initData();

        refreshIntent(getIntent());
    }

    public void initData() {
        mPlayPosTimer = new SingleSecondTimer(this);
        mHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case REFRESH_CURPOS:
                        refreshCurPos();
                        break;
                    case HIDE_TOOL:
                        break;
                    case EXIT_ACTIVITY:
                        finish();
                        break;
                }
            }

        };

        mPlayPosTimer.setHandler(mHandler, REFRESH_CURPOS);

        mMediaControlBorcastFactory = new MediaControlBrocastFactory(getApplicationContext());
        mMediaControlBorcastFactory.register(this);
    }

    public void refreshCurPos() {
        int pos = mVideoView.getCurrentPosition();
        DLNAGenaEventBrocastFactory.sendSeekEvent(mContext, pos);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        Log.d(TAG, "onNewIntent() ");
        super.onNewIntent(intent);

        mVideoView.stop();
        refreshIntent(intent);
    }

    public void refreshIntent(Intent intent) {
        videoPath = intent.getStringExtra(DlnaMediaModelFactory.PARAM_GET_URL);
        initVideo();
    }

    @Override
    protected void onStart() {
        super.onStart();
        isActivityVisible = true;
    }

    @Override
    public void onResume() {
        super.onResume();
        isActivityVisible = true;

        updatePlayerViewMode();
        if (mVideoView != null) {
            //mVideoView.onResume();
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        updatePlayerViewMode();
    }

    private void updatePlayerViewMode() {
        if (mVideoView != null) {
            int orientation = getResources().getConfiguration().orientation;
            //转为竖屏了。
            if (orientation == Configuration.ORIENTATION_PORTRAIT) {
                //显示标题栏
                //this.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
                //mVideoView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);

            } else if (orientation == Configuration.ORIENTATION_LANDSCAPE) {
                //转到横屏了。
                //隐藏状态栏
                this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
                mVideoView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
            }
        }
    }

    private void initVideo() {
        Log.d(TAG, "initVideo()  ");

        mProgressZone.setVisibility(View.VISIBLE);
        mVideoView.stop();

        mVideoView.setOnErrorListener(this);
        mVideoView.setOnCompletionListener(this);
        mVideoView.setOnSeekCompleteListener(this);
        mVideoView.setOnInfoListener(this);
        mVideoView.setOnLoadingListener(this);
        mVideoView.requestFocus();
        mVideoView.setOnPreparedListener(() -> {
            Log.w(TAG, "onPrepared()  ");
            mVideoView.setBackgroundColor(Color.argb(0, 0, 255, 0));
            if (!hasBufferStarted) {
                mProgressZone.setVisibility(View.INVISIBLE);
            }
            mVideoView.start();

            int duration = mVideoView.getDuration();
            DLNAGenaEventBrocastFactory.sendDurationEvent(mContext, duration);
        });

        hasBufferStarted = false;
        mVideoView.setDataSource(videoPath);
        mVideoView.prepare();
        mPlayPosTimer.stopTimer();
        DLNAGenaEventBrocastFactory.sendTranstionEvent(mContext);
        Log.w(TAG, "prepare()  ");
        startPlayVideoTime = System.currentTimeMillis();
    }

    @Override
    public void onPause() {
        Log.d(TAG, "onPause()  ");
        isActivityVisible = false;

        super.onPause();
    }

    @Override
    protected void onStop() {
        Log.d(TAG, "onStop()  ");
        mVideoView.stop();
        mPowerManagerUtil.releaseWakeLock();
        super.onStop();

        this.finish();
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy()  ");
        mVideoView.stop();
        mPowerManagerUtil.releaseWakeLock();
        mMediaControlBorcastFactory.unregister();
        mPlayPosTimer.stopTimer();
        releasePlayer();
        super.onDestroy();
    }

    private void releasePlayer() {
        if (mVideoView != null) {
            mVideoView.onDestroy();
            mVideoView = null;
        }
    }

    @Override
    public void onError(ErrorInfo errorInfo) {
        Log.d(TAG, "onError()  ");
        //super.doStop();
        mPlayPosTimer.stopTimer();
        this.finish();
        switchToPlayer(this);
    }

    public void switchToPlayer(Context context) {
        Intent intent = new Intent();
        intent.setClass(context, VideoActivity.class);
        DlnaMediaModel mediaInfo = new DlnaMediaModel();
        mediaInfo.setUrl(videoPath);
        DlnaMediaModelFactory.pushMediaModelToIntent(intent, mediaInfo);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);

        try {
            context.startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onInfo(InfoBean infoBean) {
        if (infoBean.getCode() == InfoCode.CurrentPosition) {
            mVideoPosition = (int) infoBean.getExtraValue();
        }
    }

    @Override
    public void onLoadingBegin() {
        Log.d(TAG, "onLoadingBegin()  ");
        hasBufferStarted = true;
        mProgressZone.setVisibility(View.VISIBLE);
        mProgressText.setText("0%");
    }

    @Override
    public void onLoadingProgress(int percent, float speed) {
        if (mProgressZone.getVisibility() == View.VISIBLE) {
            mProgressText.setText(percent + "%");
        }
    }

    @Override
    public void onLoadingEnd() {
        Log.d(TAG, "onLoadingEnd()  ");
        hasBufferStarted = false;
        mProgressText.setText("100%");
        mProgressZone.setVisibility(View.INVISIBLE);
    }

    @Override
    public void onCompletion() {
        Log.w(TAG, "onCompletion()  ");
        delayToExit();
        mPlayPosTimer.stopTimer();
        DLNAGenaEventBrocastFactory.sendStopStateEvent(mContext);
    }

    @Override
    public void onSeekComplete() {
        Log.w(TAG, "onSeekComplete()  ");
    }

    /**
     * 选择播放模式
     */
    public void selectPlayMode() {
        //showDialog(DIALOG_PLAY_MODE);
        onCreateDialog(DIALOG_PLAY_MODE).show();
        isPlayingWhenPopup = mVideoView.isPlaying();
        if (isPlayingWhenPopup) mVideoView.pause();
    }

    private void delayToExit() {
        removeExitMessage();
        mHandler.sendEmptyMessageDelayed(EXIT_ACTIVITY, EXIT_DELAY_TIME);
    }

    private void removeExitMessage() {
        mHandler.removeMessages(EXIT_ACTIVITY);
    }

    //--------------------------------------处理远端控制命令------------------------------------------

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyDown() : keyCode = [" + keyCode + "], event = [" + event + "]");
        switch (keyCode) {
            case KeyEvent.KEYCODE_MEDIA_FAST_FORWARD:
                mMediaController.doQuickNext();
                return true;
            case KeyEvent.KEYCODE_MEDIA_REWIND:
                mMediaController.doQuickPrevious();
                return true;
            case KeyEvent.KEYCODE_MEDIA_PREVIOUS:
                mMediaController.doPrevious();
                return true;
            case KeyEvent.KEYCODE_MEDIA_NEXT:
                mMediaController.doNext();
                return true;
            case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
                mMediaController.doPauseResume();
                return true;
            case KeyEvent.KEYCODE_MEDIA_STOP:
                mVideoView.stop();
                return true;
            case KeyEvent.KEYCODE_BACK:
            case KeyEvent.KEYCODE_ESCAPE:
                if (mMediaController.isShowing()) {
                    mMediaController.hide();
                    return true;
                }
            case KeyEvent.KEYCODE_HOME:
            case KeyEvent.KEYCODE_VOLUME_MUTE:
            case KeyEvent.KEYCODE_VOLUME_UP:
            case KeyEvent.KEYCODE_VOLUME_DOWN:
                break;
            default:
                boolean consume = false;
                if (!mMediaController.isShowing() && (keyCode == KeyEvent.KEYCODE_DPAD_UP || keyCode == KeyEvent.KEYCODE_DPAD_DOWN
                        || keyCode == KeyEvent.KEYCODE_DPAD_LEFT || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT)) {
                    consume = true;//For save last focus in controller view
                }
                mMediaController.show();
                if (consume)
                    return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    public boolean isActivityVisible() {
        return isActivityVisible;
    }

    protected void onCommandNewMedia() {
        initVideo();
        mMediaController.show();
    }

    protected void onCommandPrevious() {
        mMediaController.doQuickPrevious();
    }

    protected void onCommandNext() {
        mMediaController.doQuickNext();
    }

    protected int getMediaDuration() {
        return mVideoView.getDuration();
    }

    protected int getMediaCurrentPosition() {
        //Log.d(TAG, "getMediaCurrentPosition() called " + mVideoPosition);
        return mVideoPosition;
    }

    protected boolean isMediaPlaying() {
        return mVideoView.isPlaying();
    }

    /**
     * 设置缩放模式
     */
    public void setScaleMode(CicadaPlayer.ScaleMode scaleMode) {
        if (mVideoView != null) {
            mVideoView.setScaleMode(scaleMode);
        }
    }

    /**
     * 设置镜像模式
     */
    public void setMirrorMode(CicadaPlayer.MirrorMode mirrorMode) {
        if (mVideoView != null) {
            mVideoView.setMirrorMode(mirrorMode);
        }
    }

    /**
     * 设置旋转模式
     */
    public void setRotationMode(CicadaPlayer.RotateMode rotate) {
        if (mVideoView != null) {
            mVideoView.setRotationMode(rotate);
        }
    }

    /**
     * 设置倍速播放
     */
    public void setSpeedMode(float speed) {
        if (mVideoView != null) {
            mVideoView.setSpeedMode(speed);
        }
    }

    /**
     * 设置音量
     */
    public void setVolume(float volume) {
        if (mVideoView != null) {
            mVideoView.setVolume(volume);
        }
    }

    /**
     * 获取当前音量
     */
    public float getcurrentVolume() {
        if (mVideoView != null) {
            return mVideoView.getVolume();
        }
        return 0;
    }

    /**
     * 设置seek模式
     *
     * @param isChecked true为精准seek,false为非精准seek
     */
    public void setSeekMode(boolean isChecked) {
        if (mVideoView != null) {
            mVideoView.setSeekMode(isChecked);
        }
    }

    /**
     * 设置是否循环
     */
    public void setLoop(boolean isLoop) {
        if (mVideoView != null) {
            mVideoView.setLoop(isLoop);
        }
    }

    /**
     * 设置是否静音
     */
    public void setMute(boolean isMute) {
        if (mVideoView != null) {
            mVideoView.setMute(isMute);
        }
    }

    @Override
    public void onBackPressed() {
        if ((System.currentTimeMillis() - exitTime) > 2000) {
            ToastUtil.show(getApplicationContext(), R.string.click_exit);
            exitTime = System.currentTimeMillis();
        } else {
            super.onBackPressed();
            finish();
        }
    }

    @Override
    public void onPlayCommand() {
        mVideoView.start();
        mMediaController.show();
    }

    @Override
    public void onPauseCommand() {
        mVideoView.pause();
        mMediaController.show();
    }

    @Override
    public void onStopCommand(int type) {
        Log.d(TAG, "onStopCommand() [" + type + "]");
        if (System.currentTimeMillis() - startPlayVideoTime < 3000) {
            return;
        }
        mPlayPosTimer.stopTimer();
        this.finish();
    }

    @Override
    public void onSeekCommand(int time) {
        mVideoView.seekTo(time);
        mMediaController.show();
    }

    @Override
    public void onCoverCommand(byte[] data) {

    }

    @Override
    public void onMetaDataCommand(String data) {

    }

    @Override
    public void onIPAddrCommand(String data) {

    }

    public boolean doPrevious() {
        return false;
    }

    public boolean doNext() {
        return false;
    }

    public int getVolumeMode() {
        return 1;
    }

    public void doStart() {
        Log.w(TAG, "doStart()  ");
        mPlayPosTimer.startTimer();
        DLNAGenaEventBrocastFactory.sendPlayStateEvent(mContext);
    }

    public void doPause() {
        mPlayPosTimer.stopTimer();
        DLNAGenaEventBrocastFactory.sendPauseStateEvent(mContext);
    }

    public void doStop() {
        mPlayPosTimer.stopTimer();
        DLNAGenaEventBrocastFactory.sendStopStateEvent(mContext);
    }
}