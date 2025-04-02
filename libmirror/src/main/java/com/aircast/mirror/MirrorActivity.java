package com.aircast.mirror;


import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.RelativeLayout;

import androidx.localbroadcastmanager.content.LocalBroadcastManager;

import com.aircast.R;
import com.aircast.center.MediaControlBrocastFactory;
import com.aircast.center.MediaRenderProxy;
import com.aircast.mirror.media.IjkVideoView;
import com.aircast.source.Source;
import com.aircast.util.Action;
import com.aircast.util.AudioFocusHelper;
import com.aircast.util.ToastUtil;

import java.util.ArrayList;

import tv.danmaku.ijk.media.player.IMediaPlayer;
import tv.danmaku.ijk.media.player.IjkMediaPlayer;

public class MirrorActivity extends Activity implements IMediaPlayer.OnCompletionListener, IMediaPlayer.OnErrorListener, IMediaPlayer.OnInfoListener {
    public static final String DELETE_SOURCE = "del.source";
    private static final String TAG = "MirrorActivity";
    private static final String ADD_SOURCE = "add.source";
    private static boolean isActive = false;
    private final ArrayList<IjkVideoView> vList = new ArrayList<>();
    private final ArrayList<String> paths = new ArrayList<>();

    protected AudioFocusHelper audioFocusHelper = new AudioFocusHelper();
    private RelativeLayout mRoot;
    private View hPlaceholder;
    private View vPlaceholder;
    private boolean exitByUser;  //exit by user press remote or touch
    private long exitTime = 0;
    private String videoPath;
    private IjkVideoView videoView;

    private BroadcastReceiver mLocalReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive() :  intent = [" + intent.getAction() + "]");
            switch (intent.getAction()) {
                case ADD_SOURCE:
                    if ( vList.size() >= 1) {
                        // 先 ios mirror, 然后Android mirror , ios声音还在播放
                        if (vList.get(0).getVideoPath().startsWith(Source.MIRROR_AIRPLAY)) {
                            MediaRenderProxy.getInstance().restartEngine();
                        }
                        stopPlayer(vList.get(0));
                        mRoot.removeView(vList.get(0));
                        vList.clear();
                        paths.clear();
                        addVideoView(intent.getStringExtra(Action.PARAM));
                        return;
                    }

                    addSource(intent.getStringExtra(Action.PARAM));
                    break;
                case DELETE_SOURCE:
                    deleteSource(intent.getStringExtra(Action.PARAM));
                    break;
                case Action.EXIT_MIRRORING:
                    exitByUser = true;
                    finish();
                    break;

                case MediaControlBrocastFactory.MEDIA_RENDERER_CMD_VIDEO_SIZE_CHANGED:
                    setVideoSize(intent.getStringExtra(Action.PARAM));
                    break;

                default:
            }

        }
    };

    private void setVideoSize(String videoPath) {
        for (IjkVideoView vv : vList) {
            if (vv.getVideoPath().contains(videoPath)) {
                vv.togglePlayer();
                break;
            }
        }
    }

    public static Intent newIntent(Context context, String videoPath) {
        Intent intent = new Intent(context, MirrorActivity.class);
        intent.putExtra("videoPath", videoPath);

        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);  //fix mirror error
        return intent;
    }

    public static void intentTo(Context context, String videoPath) {
        Log.d(TAG, "intentTo() :   videoPath = [" + videoPath + "] isActive: " + isActive);

        if (isActive) {
            Action.broadcast(ADD_SOURCE, videoPath);
        } else {
            context.startActivity(newIntent(context, videoPath));
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        setContentView(R.layout.ap_activity_mirror);
        // handle arguments
        videoPath = getIntent().getStringExtra("videoPath");
        Log.d(TAG, "onCreate() videoPath = [" + videoPath + "]");

        audioFocusHelper.requestFocus();
        // init player
        IjkMediaPlayer.loadLibrariesOnce(null);
        IjkMediaPlayer.native_profileBegin("libijkplayer.so");

        View llExit = findViewById(R.id.llExit);
        llExit.setOnTouchListener((view, motionEvent) -> {
            exitByUser = true;
            finish();
            return true;
        });
        Button btnExit = findViewById(R.id.btnExit);
        btnExit.setOnClickListener(v -> {
            exitByUser = true;
            finish();
        });

        videoView = findViewById(R.id.video_view);
        addToList(videoView, videoPath);
        mRoot = findViewById(R.id.ex_root);
        hPlaceholder = findViewById(R.id.h_placeholder);
        vPlaceholder = findViewById(R.id.v_placeholder);

        videoView.setOnCompletionListener(this);
        videoView.setOnErrorListener(this);
        videoView.setOnInfoListener(this);
        // prefer mVideoPath
        if (videoPath != null) {
            videoView.setVideoPath(videoPath);
        } else {
            Log.e(TAG, "Null Data Source\n");
            finish();
            return;
        }
        videoView.start();
        registerLocalReceiver();
    }

    private void registerLocalReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ADD_SOURCE);
        intentFilter.addAction(DELETE_SOURCE);
        intentFilter.addAction(Action.EXIT_MIRRORING);
        intentFilter.addAction(MediaControlBrocastFactory.MEDIA_RENDERER_CMD_VIDEO_SIZE_CHANGED);

        LocalBroadcastManager.getInstance(this).registerReceiver(mLocalReceiver, intentFilter);
    }

    private void deleteSource(String path) {
        Log.d(TAG, "deleteSource() : path = [" + path + "]");
        int index = paths.indexOf(path);
        if (index != -1) {
            //disableFocus();

            paths.remove(path);
            IjkVideoView vv = vList.get(index);
            stopPlayer(vv);
            mRoot.removeView(vv);
            vList.remove(vv);

            Log.d(TAG, "deleteSource() sz [" + vList.size() + "]" + paths.size() + "]");

            if (vList.size() == 0) {
                Log.d(TAG, "deleteSource() finish ");
                finish();
                return;
            }

            switch (vList.size()) {
                case 1:
                    reLayoutOnly1();
                    break;
                case 2:
                    reLayoutOnly2();
                    break;
                case 3:
                    reLayoutOnly3();
                    break;
                default:
                    return;
            }
            enableFocus();
        }

    }

    private void disableFocus() {
        Log.d(TAG, "disableFocus()  ");
        hPlaceholder.requestFocus();
        for (IjkVideoView vv : vList) {
            vv.setFocusEffect(false);
            vv.setFocusable(false);
            vv.setClickable(false);
        }
    }

    private void enableFocus() {
        Log.d(TAG, "enableFocus()  ");
        hPlaceholder.requestFocus();
        for (IjkVideoView vv : vList) {
            vv.setFocusEffect(true);
            vv.setFocusable(true);
            vv.setClickable(true);
        }
    }

    private void stopPlayer(IjkVideoView vv) {
        Log.d(TAG, "stopPlayer() vList  = [" + vList.size() + "]");
        vv.setVisibility(View.VISIBLE);
        vv.stopPlayback();
        vv.release(true);
        vv.stopBackgroundPlay();
    }

    private void addSource(String path) {
        Log.d(TAG, "addSource() : path = [" + path + "] sz = [" + vList.size() + "]");

        disableFocus();

        //change exist view layout
        switch (vList.size()) {
            case 1:
                reLayoutForAdd2();
                addVideoView(path);
                break;
            case 2:
                reLayoutForAdd3();
                addVideoView(path);
                break;
            case 3:
                reLayoutForAdd4();
                addVideoView(path);
                break;
            default:
                return;
        }
        enableFocus();
    }

    private void addVideoView(String path) {
        Log.d(TAG, "addVideoView() : path = [" + path + "], sz = [" + vList.size() + "]");

        IjkVideoView ijk = new IjkVideoView(this);
        ijk.setFocusable(true);
        ijk.setClickable(true);
        ijk.setOnCompletionListener(this);
        ijk.setOnErrorListener(this);
        ijk.setVideoPath(path);

        switch (vList.size()) {
            case 1:
                mRoot.addView(ijk, layoutForView2());
                addToList(ijk, path);
                ijk.start();
                break;
            case 2:
                mRoot.addView(ijk, layoutForView3());
                addToList(ijk, path);
                ijk.start();
                break;
            case 3:
                mRoot.addView(ijk, layoutForView4());
                addToList(ijk, path);
                ijk.start();
                break;

            case 0: // single window
                mRoot.addView(ijk, new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT));
                addToList(ijk, path);
                ijk.start();
                break;
            default:
        }
    }

    private void addToList(IjkVideoView ijk, String path) {
        //ijk.setOnTouchListener(ijkOnTouch);
        vList.add(ijk);
        paths.add(path);
    }

    private RelativeLayout.LayoutParams layoutForView2() {
        RelativeLayout.LayoutParams rl = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);
        rl.addRule(RelativeLayout.ALIGN_LEFT, hPlaceholder.getId());
        rl.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        return rl;
    }

    private RelativeLayout.LayoutParams layoutForView3() {
        RelativeLayout.LayoutParams rl = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);
        rl.addRule(RelativeLayout.ALIGN_LEFT, hPlaceholder.getId());
        rl.addRule(RelativeLayout.ALIGN_BOTTOM, vPlaceholder.getId());
        rl.addRule(RelativeLayout.CENTER_VERTICAL);
        return rl;
    }


    private RelativeLayout.LayoutParams layoutForView4() {
        RelativeLayout.LayoutParams rl = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);
        rl.addRule(RelativeLayout.ALIGN_LEFT, hPlaceholder.getId());
        rl.addRule(RelativeLayout.ALIGN_TOP, vPlaceholder.getId());
        return rl;
    }

    private void reLayoutForAdd2() {
        RelativeLayout.LayoutParams rl = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);
        rl.addRule(RelativeLayout.ALIGN_RIGHT, hPlaceholder.getId());
        rl.addRule(RelativeLayout.ALIGN_PARENT_LEFT); //??
        vList.get(0).setLayoutParams(rl);
    }

    private void reLayoutForAdd3() {
        RelativeLayout.LayoutParams rl = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);
        rl.addRule(RelativeLayout.ALIGN_BOTTOM, vPlaceholder.getId());
        rl.addRule(RelativeLayout.ALIGN_RIGHT, hPlaceholder.getId());
        vList.get(0).setLayoutParams(rl);

        RelativeLayout.LayoutParams rl2 = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);
        rl2.addRule(RelativeLayout.ALIGN_TOP, vPlaceholder.getId());
        rl2.addRule(RelativeLayout.ALIGN_RIGHT, hPlaceholder.getId());
        vList.get(1).setLayoutParams(rl2);
    }

    private void reLayoutForAdd4() {

    }

    private void reLayoutOnly1() {
        RelativeLayout.LayoutParams rl = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);
        vList.get(0).setLayoutParams(rl);
    }

    private void reLayoutOnly2() {
        reLayoutForAdd2();
        vList.get(1).setLayoutParams(layoutForView2());
    }

    private void reLayoutOnly3() {
        reLayoutForAdd3();
        vList.get(2).setLayoutParams(layoutForView3());
    }

    private void reLayoutOnly4() {
        reLayoutOnly3();
        vList.get(3).setLayoutParams(layoutForView4());
    }

    @Override
    public void onBackPressed() {
        if ((System.currentTimeMillis() - exitTime) > 2000) {
            //Toast.makeText(getApplicationContext(), R.string.return_exit, Toast.LENGTH_SHORT).show();
            ToastUtil.show(getApplicationContext(), R.string.click_exit);
            exitTime = System.currentTimeMillis();
        } else {
            exitByUser = true;
            finish();
            //super.onBackPressed();
        }
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume()  ");
        super.onResume();
    }

    @Override
    protected void onRestart() {
        Log.d(TAG, "onRestart()  ");
        super.onRestart();
    }

    @Override
    public void onPause() {
        Log.d(TAG, "onPause()  ");
        super.onPause();
    }

    @Override
    public void onStart() {
        Log.d(TAG, "onStart()  ");
        super.onStart();
        isActive = true;
    }


    protected void stop() {
        Log.d(TAG, "stop() called " + vList.size());

        LocalBroadcastManager.getInstance(this).unregisterReceiver(mLocalReceiver);

        for (IjkVideoView vv : vList) {
            vv.setVisibility(View.GONE);
            vv.stopPlayback();
            vv.release(true);
            vv.stopBackgroundPlay();
        }
        //PlatinumReflection.audio_destroy();  //bug

        vList.clear();
        paths.clear();
        IjkMediaPlayer.native_profileEnd();

        if (exitByUser && videoPath.startsWith(Source.MIRROR_AIRPLAY)) {
            MediaRenderProxy.getInstance().restartEngine();  // fix bug, ios show error mirror state
        }

        audioFocusHelper.abandonFocus();
    }


    @Override
    protected void onStop() {
        Log.d(TAG, "onStop() called :sz " + vList.size());
        super.onStop();

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        isActive = false;
        stop();
        Log.d(TAG, "onDestroy() isActive = false");
    }


    @Override
    public void onCompletion(IMediaPlayer mp) {
        Log.d(TAG, "onCompletion() called  ");
        //exit(mp);
    }

    @Override
    public boolean onError(IMediaPlayer mp, int what, int extra) {
        Log.d(TAG, "onError() : mp = [" + mp + "], what = [" + what + "], extra = [" + extra + "]");
        //exit(mp);
        return true;
    }


    @Override
    public boolean onInfo(IMediaPlayer iMediaPlayer, int arg1, int arg2) {
        if (arg1 == IMediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START) {
            Log.d(TAG, "MEDIA_INFO_VIDEO_RENDERING_START:");
        }
        return false;
    }
}
