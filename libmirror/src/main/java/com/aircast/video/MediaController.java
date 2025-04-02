package com.aircast.video;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Message;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.aircast.R;
import com.aircast.util.SysUtils;
import com.aircast.util.WindowUtils;
import com.cicada.player.CicadaPlayer;

import java.util.Formatter;
import java.util.Locale;

/**
 * MediaController will hide and
 * show the buttons according to these rules:
 * <ul>
 * <li> The "previous" and "next" buttons are hidden until setPrevNextListeners()
 *   has been called
 * <li> The "previous" and "next" buttons are visible but disabled if
 *   setPrevNextListeners() was called with null listeners
 * <li> The "rewind" and "fastforward" buttons are shown unless requested
 *   otherwise by using the MediaController(Context, boolean) constructor
 *   with the boolean set to false
 * </ul>
 */
public class MediaController extends FrameLayout {
    public static final boolean FULLSCREEN_MAINTIANXY = true;
    //屏幕大小模式
    public static final int SCREEN_MODE_ORIG = 0;
    public static final int SCREEN_MODE_169 = 1;
    public static final int SCREEN_MODE_43 = 2;
    public static final int SCREEN_MODE_FULL = 3;
    private static final String TAG = "MediaController";
    private static final int sDefaultTimeout = 5000;
    private static final int FADE_OUT = 1;
    private static final int SHOW_PROGRESS = 2;
    private static final int SEEK_TO = 3;
    /**
     * 屏幕亮度监听
     */
    private final int MIN_BRIGHTNESS = 118;
    private final int INTEVAL_BRIGHTNESS = 32;
    StringBuilder mFormatBuilder;
    Formatter mFormatter;
    private MediaPlayerControl mPlayer;
    private final OnClickListener mExitListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            if (mPlayer != null) {
                mPlayer.finish();
            }
        }
    };
    private Context mContext;
    private View mControlView;
    private SeekBar mProgress;
    private TextView mEndTime, mCurrentTime;
    private boolean mShowing;
    private boolean mDragging;
    private ImageButton mScreenModeButton;
    private ImageButton mScreenBrightButton;
    private ImageButton mPrevButton;
    private ImageButton mPauseButton;
    private ImageButton mNextButton;
    private ImageButton mExitButton;
    private ImageButton mMoreButton;
    private ImageButton mVolumePlusButton;
    private ImageButton mVolumeMinusButton;
    private int mScreenSizeMode = -1;
    private int mScreenBrightMode = 0;
    private long mLastSeekTime;
    private int mLastSeekPosition;
    private View mLastFocusView;
    private LinearLayout mSpeedLayout;
    private Button speedBtn50;
    private Button speedBtn75;
    private Button speedBtn100;
    private Button speedBtn125;
    private Button speedBtn150;
    private Button speedBtn200;
    private LinearLayout mScaleLayout;
    private final OnClickListener mMoreListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
/*        	if(mPlayer!=null){
        		mPlayer.selectPlayMode();
        	}*/
            if (mSpeedLayout.getVisibility() == INVISIBLE) {
                mSpeedLayout.setVisibility(VISIBLE);
                mScaleLayout.setVisibility(INVISIBLE);

                setFocusBySpeed(mPlayer.getSpeedMode());
            } else {
                mSpeedLayout.setVisibility(INVISIBLE);
            }
        }
    };
    private Button mAspectFitBtn;
    private Button mAspectFillBtn;

    //    public void disableFeature(Activity caller) {
//        if (caller instanceof VideoPlayer) {
//            mMoreButton.setVisibility(INVISIBLE);
//            mScreenModeButton.setVisibility(INVISIBLE);
//        }
//    }
    private Button mToFitBtn;

    public MediaController(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
    }

    private final OnClickListener mPlaySpeedListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            show();

            int viewId = v.getId();

            if (viewId == R.id.btn_speed_50) {
                mPlayer.setSpeedMode(0.5f);
                mMoreButton.setImageResource(R.drawable.ap_s0_5);
            } else if (viewId == R.id.btn_speed_75) {
                mPlayer.setSpeedMode(0.75f);
                mMoreButton.setImageResource(R.drawable.ap_s0_75);
            } else if (viewId == R.id.btn_speed_125) {
                mPlayer.setSpeedMode(1.25f);
                mMoreButton.setImageResource(R.drawable.ap_s1_25);
            } else if (viewId == R.id.btn_speed_150) {
                mPlayer.setSpeedMode(1.5f);
                mMoreButton.setImageResource(R.drawable.ap_s1_5);
            } else if (viewId == R.id.btn_speed_200) {
                mPlayer.setSpeedMode(2.0f);
                mMoreButton.setImageResource(R.drawable.ap_s2_0);
            } else {
                // Default case or R.id.btn_speed_100
                mPlayer.setSpeedMode(1.0f);
                mMoreButton.setImageResource(R.drawable.ap_s1_0);
            }
        }
    };

    public MediaController(Context context) {
        super(context);
        mContext = context;
    }

    public void setControlView(View view) {
        mControlView = view;
        initControllerView(mControlView);
    }

    private final OnClickListener mScaleModeListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            show();

            int viewId = v.getId();

            if (viewId == R.id.btn_aspect_fit) {
                mPlayer.setScaleMode(CicadaPlayer.ScaleMode.SCALE_ASPECT_FIT);
            } else if (viewId == R.id.btn_aspect_fill) {
                mPlayer.setScaleMode(CicadaPlayer.ScaleMode.SCALE_ASPECT_FILL);
            } else if (viewId == R.id.btn_to_fill) {
                mPlayer.setScaleMode(CicadaPlayer.ScaleMode.SCALE_TO_FILL);
            }
        }
    };

    public void setMediaPlayer(MediaPlayerControl player) {
        mPlayer = player;
        updatePausePlay();
    }

    private void setFocusBySpeed(float speed) {
        int ispeed = (int) (speed * 100);
        Log.d(TAG, "setFocusBySpeed() : speed = [" + speed + "]" + ispeed);
        switch (ispeed) {
            case 50:
                speedBtn50.requestFocus();
                break;
            case 75:
                speedBtn75.requestFocus();
                break;
            default:
            case 100:
                speedBtn100.requestFocus();
                break;
            case 125:
                speedBtn125.requestFocus();
                break;
            case 150:
                speedBtn150.requestFocus();
                break;
            case 200:
                speedBtn200.requestFocus();
                break;
        }
    }

    private void initControllerView(View v) {
        mSpeedLayout = v.findViewById(R.id.ll_play_speed);
        speedBtn50 = v.findViewById(R.id.btn_speed_50);
        speedBtn75 = v.findViewById(R.id.btn_speed_75);
        speedBtn100 = v.findViewById(R.id.btn_speed_100);
        speedBtn125 = v.findViewById(R.id.btn_speed_125);
        speedBtn150 = v.findViewById(R.id.btn_speed_150);
        speedBtn200 = v.findViewById(R.id.btn_speed_200);
        speedBtn50.setOnClickListener(mPlaySpeedListener);
        speedBtn75.setOnClickListener(mPlaySpeedListener);
        speedBtn100.setOnClickListener(mPlaySpeedListener);
        speedBtn125.setOnClickListener(mPlaySpeedListener);
        speedBtn150.setOnClickListener(mPlaySpeedListener);
        speedBtn200.setOnClickListener(mPlaySpeedListener);

        mScaleLayout = v.findViewById(R.id.ll_scale_mode);
        mAspectFitBtn = v.findViewById(R.id.btn_aspect_fit);
        mAspectFillBtn = v.findViewById(R.id.btn_aspect_fill);
        mToFitBtn = v.findViewById(R.id.btn_to_fill);
        mAspectFitBtn.setOnClickListener(mScaleModeListener);
        mAspectFillBtn.setOnClickListener(mScaleModeListener);
        mToFitBtn.setOnClickListener(mScaleModeListener);

        mScreenModeButton = v.findViewById(R.id.menubar_btn_screenMode);
        if (mScreenModeButton != null) {
            mScreenModeButton.requestFocus();
            mScreenModeButton.setOnClickListener(mScreenModeListener);
        }

        mScreenBrightButton = (ImageButton) v.findViewById(R.id.menubar_btn_screenBright);
        if (mScreenBrightButton != null) {
            mScreenBrightButton.requestFocus();
            mScreenBrightButton.setOnClickListener(mScreenBrightListener);
        }

        mPrevButton = (ImageButton) v.findViewById(R.id.menubar_btn_prev);
        if (mPrevButton != null) {
            mPrevButton.requestFocus();
            mPrevButton.setOnClickListener(mPrevListener);
        }

        mPauseButton = (ImageButton) v.findViewById(R.id.menubar_btn_pause);
        if (mPauseButton != null) {
            mPauseButton.requestFocus();
            mPauseButton.setOnClickListener(mPauseListener);
        }

        mNextButton = (ImageButton) v.findViewById(R.id.menubar_btn_next);
        if (mNextButton != null) {
            mNextButton.requestFocus();
            mNextButton.setOnClickListener(mNextListener);
        }

        mExitButton = (ImageButton) v.findViewById(R.id.menubar_btn_exit);
        if (mExitButton != null) {
            mExitButton.requestFocus();
            mExitButton.setOnClickListener(mExitListener);
        }

        mMoreButton = v.findViewById(R.id.menubar_btn_more);
        if (mMoreButton != null) {
            mMoreButton.requestFocus();
            mMoreButton.setOnClickListener(mMoreListener);
        }

        mProgress = v.findViewById(R.id.mediacontroller_progress);
        if (mProgress != null) {
            mProgress.setOnSeekBarChangeListener(mSeekListener);
            mProgress.setMax(1000);
            mProgress.setKeyProgressIncrement(10);
        }

        mVolumePlusButton = (ImageButton) v.findViewById(R.id.menubar_btn_volumeplus);
        if (mVolumePlusButton != null) {
            mVolumePlusButton.requestFocus();
            mVolumePlusButton.setOnClickListener(mVolumePlusListener);
        }

        mVolumeMinusButton = (ImageButton) v.findViewById(R.id.menubar_btn_volumeminus);
        if (mVolumeMinusButton != null) {
            mVolumeMinusButton.requestFocus();
            mVolumeMinusButton.setOnClickListener(mVolumeMinusListener);
        }

        mEndTime = (TextView) v.findViewById(R.id.time);
        mCurrentTime = (TextView) v.findViewById(R.id.time_current);
        mFormatBuilder = new StringBuilder();
        mFormatter = new Formatter(mFormatBuilder, Locale.getDefault());
    }

    /**
     * Show the controller on screen. It will go away
     * automatically after 3 seconds of inactivity.
     */
    public void show() {
        show(sDefaultTimeout);
    }

    /**
     * Disable pause or seek buttons if the stream cannot be paused or seeked.
     * This requires the control interface to be a MediaPlayerControlExt
     */
    private void disableUnsupportedButtons() {
        try {
            if (mPlayer == null)
                return;
            if (mPauseButton != null && !mPlayer.canPause()) {
                mPauseButton.setEnabled(false);
            }
            if (mPrevButton != null && !mPlayer.canSeekBackward()) {
                mPrevButton.setEnabled(false);
            }
            if (mNextButton != null && !mPlayer.canSeekForward()) {
                mNextButton.setEnabled(false);
            }
        } catch (IncompatibleClassChangeError ex) {
            // We were given an old version of the interface, that doesn't have
            // the canPause/canSeekXYZ methods. This is OK, it just means we
            // assume the media can be paused and seeked, and so we don't disable
            // the buttons.
        }
    }

    private boolean isLastShow(){
        if(mLastFocusView != null){
            if(mLastFocusView instanceof ImageButton)return true;
            if(mLastFocusView instanceof SeekBar)return true;
        }
        return false;
    }
    /**
     * Show the controller on screen. It will go away
     * automatically after 'timeout' milliseconds of inactivity.
     *
     * @param timeout The timeout in milliseconds. Use 0 to show
     *                the controller until hide() is called.
     */
    public void show(int timeout) {
        Log.d(TAG, "show() : timeout = [" + timeout + "]");
        if (mPlayer == null) {
            Log.e("MediaController", "MediaPlayer is null. ");
            return;
        }
        if (!mShowing) {
            setProgress();
            if (mLastFocusView != null && isLastShow()) {
                mLastFocusView.requestFocus();
            } else if (mPauseButton != null) {
                mHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        mPauseButton.requestFocus();
                    }
                }, 50);

            }
            disableUnsupportedButtons();
            if (mControlView != null) {
                mControlView.setVisibility(View.VISIBLE);
            }
            mShowing = true;
        }
        //updatePausePlay();

        // cause the progress bar to be updated even if mShowing
        // was already true.  This happens, for example, if we're
        // paused with the progress bar showing the user hits play.
        mHandler.sendEmptyMessage(SHOW_PROGRESS);

        if (timeout != 0) {
            hideDelayed(timeout);
        }
    }

    private void hideDelayed(int timeout) {
        mHandler.removeMessages(FADE_OUT);
        mHandler.sendMessageDelayed(mHandler.obtainMessage(FADE_OUT), timeout);
    }

    public boolean isShowing() {
        return mShowing;
    }

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            int pos;
            switch (msg.what) {
                case FADE_OUT:
                    hide();
                    break;
                case SHOW_PROGRESS:
                    pos = setProgress();
                    if (!mDragging && mShowing && mPlayer.isPlaying()) {
                        mHandler.removeMessages(SHOW_PROGRESS);
                        sendMessageDelayed(obtainMessage(SHOW_PROGRESS), 1000);  //1000 - (pos % 1000));
                    }
                    break;
                case SEEK_TO:
                    mPlayer.seekTo(msg.arg1);
                    sendMessageDelayed(obtainMessage(SHOW_PROGRESS), 1000);
                    break;
                default:
            }
        }
    };

    /**
     * Remove the controller from the screen.
     */
    public void hide() {
        if (mControlView == null)
            return;

        if (mShowing) {
            mLastFocusView = mControlView.findFocus();
            mHandler.removeMessages(SHOW_PROGRESS);
            if (mControlView != null) {
                mControlView.setVisibility(View.GONE);
            }
            mShowing = false;

            mSpeedLayout.setVisibility(INVISIBLE);
            mScaleLayout.setVisibility(INVISIBLE);
        }
    }

    private String stringForTime(int timeMs) {
        int totalSeconds = timeMs / 1000;

        int seconds = totalSeconds % 60;
        int minutes = (totalSeconds / 60) % 60;
        int hours = totalSeconds / 3600;

        mFormatBuilder.setLength(0);
//        if (hours > 0) {
        return mFormatter.format("%d:%02d:%02d", hours, minutes, seconds).toString();
//        } else {
//            return mFormatter.format("%02d:%02d", minutes, seconds).toString();
//        }
    }

    public int setProgress() {
        if (mPlayer == null || mDragging) {
            return 0;
        }
        int position = mPlayer.getCurrentPosition();
        if (mLastSeekPosition > 0 && mHandler.hasMessages(SEEK_TO)) {
            position = mLastSeekPosition;
        }
        int duration = mPlayer.getDuration();
        if (mProgress != null) {
            if (duration > 0) {
                // use long to avoid overflow
                long pos = 1000L * position / duration;
                Log.d(TAG, "setProgress()  pos " + pos);
                mProgress.setProgress((int) pos);
            }
            //mPlayer.getBufferPercentage获取的不是整个内容的缓存百分比，暂时没有接口获取
            //int percent = mPlayer.getBufferPercentage();
            //mProgress.setSecondaryProgress(percent * 10);
        }

        if (mEndTime != null)
            mEndTime.setText(stringForTime(duration));
        if (mCurrentTime != null)
            mCurrentTime.setText(stringForTime(position));

        return position;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        show(sDefaultTimeout);
        return true;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent ev) {
        show(sDefaultTimeout);
        return false;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        Log.d(TAG, "dispatchKeyEvent() : event = [" + event + "]");
        int keyCode = event.getKeyCode();
        final boolean uniqueDown = event.getRepeatCount() == 0
                && event.getAction() == KeyEvent.ACTION_DOWN;
        if (event.getRepeatCount() == 0 && event.getAction() == KeyEvent.ACTION_DOWN && (
                keyCode == KeyEvent.KEYCODE_HEADSETHOOK ||
                        keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE ||
                        keyCode == KeyEvent.KEYCODE_SPACE)) {
            if (uniqueDown) {
                doPauseResume();
                show(sDefaultTimeout);
                if (mPauseButton != null) {
                    mPauseButton.requestFocus();
                }
            }
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP) {
            if (uniqueDown && mPlayer.isPlaying()) {
                mPlayer.pause();
                updatePausePlay();
            }
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
            // don't show the controls for volume adjustment
            if (uniqueDown) {
                SysUtils.volumeAdjust(mContext, mPlayer.getVolumeMode(), SysUtils.Def.VOLUMEMINUS);
            }
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
            if (uniqueDown) {
                SysUtils.volumeAdjust(mContext, mPlayer.getVolumeMode(), SysUtils.Def.VOLUMEPLUS);
            }
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_ESCAPE) {
            if (uniqueDown) {
                hide();
            }
            //return true;
            return false;
        } else if (keyCode == KeyEvent.KEYCODE_HOME) {
            //DBUtils.setbackBacklight(mContext,mPlayer.getBrightMode());
            if (uniqueDown) {
                SysUtils.setbackVolume(mContext, mPlayer.getVolumeMode());
            }
            return false;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_MUTE) {
            return super.dispatchKeyEvent(event);
        } else {
            show(sDefaultTimeout);
        }
        return super.dispatchKeyEvent(event);
    }

    private OnClickListener mPauseListener = new OnClickListener() {
        public void onClick(View v) {
            doPauseResume();
            show(sDefaultTimeout);
        }
    };

    public void updatePausePlay(boolean playing) {
        Log.d(TAG, "updatePausePlay() playing " + playing);
        if (mControlView == null || mPauseButton == null)
            return;

        if (playing) {
            mPauseButton.setImageResource(R.drawable.ap_icon_zanting);
        } else {
            mPauseButton.setImageResource(R.drawable.ap_icon_bofang);
        }
    }

    private void updatePausePlay() {
        updatePausePlay(mPlayer.isPlaying());
    }

    public void doPauseResume() {
        Log.d(TAG, "doPauseResume()  ");
        if (mPlayer.isPlaying()) {
            mPlayer.pause();
        } else {
            mPlayer.start();
        }
        updatePausePlay();
    }

    @Override
    public void setEnabled(boolean enabled) {
        if (mPauseButton != null) {
            mPauseButton.setEnabled(enabled);
        }
        if (mNextButton != null) {
            mNextButton.setEnabled(enabled);
        }
        if (mPrevButton != null) {
            mPrevButton.setEnabled(enabled);
        }
        if (mProgress != null) {
            mProgress.setEnabled(enabled);
        }
        if (mVolumePlusButton != null) {
            mVolumePlusButton.setEnabled(enabled);
        }
        if (mVolumeMinusButton != null) {
            mVolumeMinusButton.setEnabled(enabled);
        }
        if (mScreenModeButton != null) {
            mScreenModeButton.setEnabled(enabled);
        }
        disableUnsupportedButtons();
        super.setEnabled(enabled);
    }    // There are two scenarios that can trigger the seekbar listener to trigger:

    //
    // The first is the user using the touchpad to adjust the posititon of the
    // seekbar's thumb. In this case onStartTrackingTouch is called followed by
    // a number of onProgressChanged notifications, concluded by onStopTrackingTouch.
    // We're setting the field "mDragging" to true for the duration of the dragging
    // session to avoid jumps in the position in case of ongoing playback.
    //
    // The second scenario involves the user operating the scroll ball, in this
    // case there WON'T BE onStartTrackingTouch/onStopTrackingTouch notifications,
    // we will simply apply the updated position without suspending regular updates.
    private OnSeekBarChangeListener mSeekListener = new OnSeekBarChangeListener() {
        long duration;

        @Override
        public void onStartTrackingTouch(SeekBar bar) {
            setProgress();
            //show(3600000);
            show();
            duration = mPlayer.getDuration();
            mPlayer.pause();

            // By removing these pending progress messages we make sure
            // that a) we won't update the progress while the user adjusts
            // the seekbar and b) once the user is done dragging the thumb
            // we will post one of these messages to the queue again and
            // this ensures that there will be exactly one message queued up.
            // mHandler.removeMessages(SHOW_PROGRESS);
        }

        @Override
        public void onProgressChanged(SeekBar bar, int progress, boolean fromuser) {
            if (!fromuser) {
                // We're not interested in programmatically generated changes to
                // the progress bar's position.
                return;
            }

            //mDragging = true;
            hideDelayed(sDefaultTimeout);//解决不断seek, Bar隐藏后，焦点移动到下方。
            mHandler.removeMessages(SHOW_PROGRESS);

            duration = mPlayer.getDuration();
            long newposition = (duration * progress) / 1000L;
            if (mPlayer.isSeeking() || mHandler.hasMessages(SEEK_TO) || (System.currentTimeMillis() - mLastSeekTime < 180)) {
                mHandler.removeMessages(SEEK_TO);
                Message seekMsg = mHandler.obtainMessage(SEEK_TO);
                seekMsg.arg1 = (int) newposition;
                mHandler.sendMessageDelayed(seekMsg, 180);
            } else {
                mPlayer.seekTo((int) newposition);
                mHandler.sendMessageDelayed(mHandler.obtainMessage(SHOW_PROGRESS), 1300);
            }
            mLastSeekPosition = (int) newposition;
            mLastSeekTime = System.currentTimeMillis();

            if (mCurrentTime != null)
                mCurrentTime.setText(stringForTime((int) newposition));
            if (mEndTime != null) {
                mEndTime.setText(stringForTime((int) duration));
            }
        }

        @Override
        public void onStopTrackingTouch(SeekBar bar) {
            Log.d(TAG, "onStopTrackingTouch() called  ");
            mHandler.removeMessages(SEEK_TO);
            mDragging = false;
            int progress = mProgress.getProgress();
            duration = mPlayer.getDuration();
            long newposition = (duration * progress) / 1000L;
            if (newposition >= duration) {
                newposition = duration - 2000;
            }
            mPlayer.seekTo((int) newposition);
            if (mCurrentTime != null)
                mCurrentTime.setText(stringForTime((int) newposition));
            if (mEndTime != null) {
                mEndTime.setText(stringForTime((int) duration));
            }
            setProgress();
            mPlayer.start();
            updatePausePlay();
            show(sDefaultTimeout);
        }
    };

    public void doPrevious() {
        if (!mPlayer.prev())//上一首不成功时 执行快退
            doQuickPrevious();
    }

    public void doQuickPrevious() {
        if (mPlayer == null) return;
        int pos = mPlayer.getCurrentPosition();
        pos -= 5000; // milliseconds
        mPlayer.seekTo(pos);
        setProgress();

        show(sDefaultTimeout);
    }

    private final OnClickListener mPrevListener = new OnClickListener() {
        public void onClick(View v) {
            doPrevious();
        }
    };

    public void doNext() {
        if (!mPlayer.next())//下一首不成功时 执行快进
            doQuickNext();
    }

    public void doQuickNext() {
        if (mPlayer == null) return;
        int pos = mPlayer.getCurrentPosition();
        pos += 15000; // milliseconds
        mPlayer.seekTo(pos);
        setProgress();

        show(sDefaultTimeout);
    }

    private void setFocusByScaleMode() {
        switch (mPlayer.getScaleMode()) {
            default:
            case SCALE_ASPECT_FIT:
                mAspectFitBtn.requestFocus();
                break;
            case SCALE_ASPECT_FILL:
                mAspectFillBtn.requestFocus();
                break;
            case SCALE_TO_FILL:
                mToFitBtn.requestFocus();
                break;
        }
    }

    private final OnClickListener mNextListener = new OnClickListener() {
        public void onClick(View v) {
            doNext();
        }
    };

    public void setCurrentScreen() {
        if (mScreenSizeMode == -1) {
            mScreenSizeMode = SysUtils.getScreenValue(mContext);
        }
        setButtonSrcByMode(mScreenSizeMode);
        setScreenSize(mScreenSizeMode);
    }

    public void setScreen(int ScreenSizeMode) {
        setButtonSrcByMode(ScreenSizeMode);
        setScreenSize(ScreenSizeMode);
    }

    private void setScreenSize(int mode) {
        DisplayMetrics dm = WindowUtils.getWindowMetrics((Activity) mContext);
        int maxWidth = dm.widthPixels;
        int maxHeight = dm.heightPixels;
        switch (mode) {
            case SCREEN_MODE_ORIG:
                if (FULLSCREEN_MAINTIANXY) {
                    if (mPlayer.getDefaultWidth() > maxWidth || mPlayer.getDefaultHeight() > maxHeight) {
                        float degree = (float) mPlayer.getDefaultWidth() / (float) mPlayer.getDefaultHeight();
                        int tmpWidth1 = maxWidth;
                        int tmpHeight1 = (int) (tmpWidth1 / degree);

                        int tmpHeight2 = maxHeight;
                        int tmpWidth2 = (int) (tmpHeight2 * degree);

                        if (tmpHeight1 > maxHeight && tmpWidth2 <= maxWidth) {
                            mPlayer.setScreenSize(tmpWidth2, tmpHeight2);
                        } else if (tmpWidth2 > maxWidth && tmpHeight1 <= maxHeight) {
                            mPlayer.setScreenSize(tmpWidth1, tmpHeight1);
                        } else if (tmpHeight1 <= maxHeight && tmpWidth2 <= maxWidth) {
                            if (tmpWidth1 * tmpHeight1 > tmpWidth2 * tmpHeight2) {
                                mPlayer.setScreenSize(tmpWidth1, tmpHeight1);
                            } else {
                                mPlayer.setScreenSize(tmpWidth2, tmpHeight2);
                            }
                        } else {
                            mPlayer.setScreenSize(maxWidth, maxHeight);
                        }
                    } else {
                        mPlayer.setScreenSize(mPlayer.getDefaultWidth(), mPlayer.getDefaultHeight());
                    }
                } else {
                    mPlayer.setScreenSize(mPlayer.getDefaultWidth(), mPlayer.getDefaultHeight());
                }
                SysUtils.setScreenValue(mContext, mode);
                break;
            case SCREEN_MODE_169:
                mPlayer.setScreenSize(maxWidth, maxWidth / 16 * 9);
                SysUtils.setScreenValue(mContext, mode);
                break;
            case SCREEN_MODE_43:
                mPlayer.setScreenSize(maxHeight / 3 * 4, maxHeight);
                SysUtils.setScreenValue(mContext, mode);
                break;
            case SCREEN_MODE_FULL:
                if (mPlayer.getDefaultWidth() == 0 || mPlayer.getDefaultHeight() == 0) {
                    mPlayer.setScreenSize(maxWidth, maxHeight);
                    SysUtils.setScreenValue(mContext, mode);
                    break;
                }

                if (FULLSCREEN_MAINTIANXY) {
                    float degree = (float) mPlayer.getDefaultWidth() / (float) mPlayer.getDefaultHeight();
                    int tmpWidth1 = maxWidth;
                    int tmpHeight1 = (int) (tmpWidth1 / degree);

                    int tmpHeight2 = maxHeight;
                    int tmpWidth2 = (int) (tmpHeight2 * degree);

                    if (tmpHeight1 > maxHeight && tmpWidth2 <= maxWidth) {
                        mPlayer.setScreenSize(tmpWidth2, tmpHeight2);
                    } else if (tmpWidth2 > maxWidth && tmpHeight1 <= maxHeight) {
                        mPlayer.setScreenSize(tmpWidth1, tmpHeight1);
                    } else if (tmpHeight1 <= maxHeight && tmpWidth2 <= maxWidth) {
                        if (tmpWidth1 * tmpHeight1 > tmpWidth2 * tmpHeight2) {
                            mPlayer.setScreenSize(tmpWidth1, tmpHeight1);
                        } else {
                            mPlayer.setScreenSize(tmpWidth2, tmpHeight2);
                        }
                    } else {
                        mPlayer.setScreenSize(maxWidth, maxHeight);
                    }
                } else {
                    mPlayer.setScreenSize(maxWidth, maxHeight);
                }

                SysUtils.setScreenValue(mContext, mode);
                break;
        }
    }

    public void setButtonSrcByMode(int mode) {
/*    	if(mScreenModeButton == null){
    		return;
    	}
    	Drawable drawable = mScreenModeButton.getDrawable();
    	drawable.setLevel(mode);
    	mScreenSizeMode = mode;*/
    }

    private void setScreenMode(int mode) {
        if (mode > 4) mode = 4;
        if (mode < 0)
            mode = 0;
        SysUtils.BrightnessUtil.setUserBrightness(mContext, MIN_BRIGHTNESS + mode * INTEVAL_BRIGHTNESS);
        Drawable drawable = mScreenBrightButton.getDrawable();
        drawable.setLevel(mode);
        mScreenBrightMode = mode;
    }

    private OnClickListener mVolumePlusListener = new OnClickListener() {
        public void onClick(View v) {
            if (mPlayer != null) {
                SysUtils.volumeAdjust(mContext, mPlayer.getVolumeMode(), SysUtils.Def.VOLUMEPLUS);
                show();
            }
        }
    };

    public void setScreenBrightness(int brightness) {
        int screenBrightMode = 0;
        int levelBrightness = brightness - MIN_BRIGHTNESS;
        if (levelBrightness <= 0) {
            screenBrightMode = 0;
        } else {
            screenBrightMode = levelBrightness / INTEVAL_BRIGHTNESS;
        }
        setScreenMode(screenBrightMode);
    }

    private OnClickListener mVolumeMinusListener = new OnClickListener() {
        public void onClick(View v) {
            if (mPlayer != null) {
                SysUtils.volumeAdjust(mContext, mPlayer.getVolumeMode(), SysUtils.Def.VOLUMEMINUS);
                show();
            }
        }
    };

    /**
     * Whether in media control view
     *
     * @param x
     * @param y
     * @return
     */
    public boolean isInMediaControlLayout(float x, float y) {
        if (mControlView != null) {
            int[] loc = new int[2];
            ViewGroup vg = (ViewGroup) mControlView;
            if (vg.getChildCount() > 0) {
                vg.getChildAt(0).getLocationInWindow(loc);
                if ((x >= loc[0] && y >= loc[1]) || (loc[0] == 0 && loc[1] == 0)) {
                    return true;
                } else {
                    return false;
                }
            }
        }
        return true;
    }

    public interface MediaPlayerControl {
        void start();

        boolean prev();

        boolean next();

        void pause();

        int getDuration();

        int getCurrentPosition();

        void seekTo(int pos);

        boolean isPlaying();

        int getBufferPercentage();

        boolean canPause();

        boolean canSeekBackward();

        boolean canSeekForward();

        void finish();

        int getVolumeMode();

        int getDefaultWidth();

        int getDefaultHeight();

        void setScreenSize(int width, int height);

        void selectPlayMode();

        boolean isSeeking();

        float getSpeedMode();

        void setSpeedMode(float speed);

        CicadaPlayer.ScaleMode getScaleMode();

        void setScaleMode(CicadaPlayer.ScaleMode scaleMode);
    }

    /**
     * 屏幕尺寸大小调整监听
     */
    private final OnClickListener mScreenModeListener = new OnClickListener() {
        @Override
        public void onClick(View v) {
            show(sDefaultTimeout);
/*        	if(mScreenSizeMode == SCREEN_MODE_FULL){
        		mScreenSizeMode = SCREEN_MODE_ORIG;
        	}else{
        		mScreenSizeMode++;
        	}
        	setScreen(mScreenSizeMode);*/

            if (mScaleLayout.getVisibility() == INVISIBLE) {
                mScaleLayout.setVisibility(VISIBLE);
                mSpeedLayout.setVisibility(INVISIBLE);
                setFocusByScaleMode();
                show();
            } else {
                mScaleLayout.setVisibility(INVISIBLE);
            }
        }
    };


    private OnClickListener mScreenBrightListener = new OnClickListener() {
        public void onClick(View v) {
            show();
            if (mScreenBrightMode == 4)
                mScreenBrightMode = 0;
            else
                mScreenBrightMode++;
            setScreenMode(mScreenBrightMode);
        }
    };


}