package com.aircast.app;

import static com.aircast.util.NetworkObserverKt.NetworkObserver;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentTransaction;

import com.aircast.app.service.MainService;
import com.aircast.app.ui.MainFragment;

import com.aircast.app.ui.SetupFragment;
import com.aircast.app.ui.SpectrumView;
import com.aircast.settings.Setting;
import com.aircast.util.Action;
import com.aircast.util.NetUtils;
import com.aircast.util.NetworkObserver;


import java.util.Calendar;

public class TvMainActivity extends AppCompatActivity {
    private static final String TAG = "TvMainActivity";
    private static final int PERMISSION_REQUEST_CODE = 1002;
    private final String[] permissions = {Manifest.permission.INTERNET, Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.WAKE_LOCK
    };
    private TextView tvAppName;
    private LinearLayout llReturn;
    private TextView tvTitle;
    private TextView devName;
    private TextView tvssid;
    private TextView tvTime;
    private LinearLayout tvMainTip;
    private SpectrumView sv;
    private TextView tvIP;
    private LinearLayout tvSetTip;
    private TextView tvVer;
    private Handler mHandler = new Handler();
    Runnable spectrumRunnable = new Runnable() {
        @Override
        public void run() {
            sv.makeData();
            sv.invalidate();
            mHandler.postDelayed(this, 300);
        }
    };
    private BroadcastReceiver mLocalReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "onReceive()  intent = [" + intent + "]");
            switch (intent.getAction()) {
                case Action.AIRPLAY_MUSIC_START:
                    mHandler.removeCallbacks(spectrumRunnable);
                    sv.setVisibility(View.VISIBLE);
                    mHandler.post(spectrumRunnable);
                    break;
                case Action.AIRPLAY_MUSIC_STOP:
                    sv.setVisibility(View.GONE);
                    mHandler.removeCallbacks(spectrumRunnable);
                    break;
                default:
                    break;
            }
        }
    };
    private Fragment currentFragment = new Fragment();
    private MainFragment mainFragment = MainFragment.newInstance();
    private SetupFragment setupFragment = SetupFragment.newInstance();

    private NetworkObserver networkObserver;
    private final Runnable showTimeRunnable = new Runnable() {
        @Override
        public void run() {
            Calendar c = Calendar.getInstance();
            int h = c.get(Calendar.HOUR_OF_DAY);
            int m = c.get(Calendar.MINUTE);

            tvTime.setText(String.format("%d:%s", h, (m < 10) ? "0" + m : m));

            mHandler.postDelayed(this, 1000 * 30);
        }
    };


    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG, "onCreate()  ");
        super.onCreate(savedInstanceState);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);


        setContentView(R.layout.tv_activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        tvAppName = findViewById(R.id.tvAppName);
        llReturn = findViewById(R.id.llReturn);
        tvTitle = findViewById(R.id.tvTitle);
        devName = findViewById(R.id.dev_name);
        tvssid = findViewById(R.id.tvssid);
        tvTime = findViewById(R.id.tvTime);
        tvMainTip = findViewById(R.id.tv_main_tip);
        sv = findViewById(R.id.sv);
        tvIP = findViewById(R.id.tvIP);
        tvSetTip = findViewById(R.id.tv_set_tip);
        tvVer = findViewById(R.id.tv_ver);

        networkObserver = NetworkObserver(getApplicationContext(), isOnline -> {
            if (isOnline) {
                Log.w(TAG, "isOnline");
                MainService.intentToStart(getApplicationContext());
            }
        });

        registerLocalReceiver();

        if (!hasPermissions()) {
            ActivityCompat.requestPermissions(this, permissions, PERMISSION_REQUEST_CODE);
        } else {
            initUi();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            boolean isAllGranted = true;
            for (int grant : grantResults) {
                if (grant != PackageManager.PERMISSION_GRANTED) {
                    isAllGranted = false;
                    break;
                }
            }

            if (isAllGranted) {
                initUi();
            } else {
                Toast.makeText(this, "请重启app并允许权限", Toast.LENGTH_SHORT).show();
            }
        }
    }

    private boolean hasPermissions() {
        for (String permission : permissions) {
            int i = ContextCompat.checkSelfPermission(getApplicationContext(), permission);
            if (i != PackageManager.PERMISSION_GRANTED) {
                Log.d(TAG, "hasPermissions()  !! " + permission);
                return false;
            }
        }
        return true;
    }

    protected void initUi() {
        gotoMain();

        tvMainTip.setOnTouchListener((v, event) -> {
            gotoSetup();
            switch (event.getAction()) {
                case MotionEvent.ACTION_UP:
                    tvMainTip.performClick();
                    break;
                default:
            }
            return false;
        });

        llReturn.setOnTouchListener((v, event) -> {
            onReturn();
            switch (event.getAction()) {
                case MotionEvent.ACTION_UP:
                    llReturn.performClick();
                    break;
                default:
            }
            return false;
        });
    }

    private boolean onReturn() {
        if (currentFragment == setupFragment) {
            gotoMain();
            return true;
        }

        return false;
    }

    private void gotoSetup() {
        if (currentFragment == setupFragment) {
            return;
        }

        switchFragment(setupFragment).commit();
        tvMainTip.setVisibility(View.GONE);
        tvSetTip.setVisibility(View.VISIBLE);

        tvAppName.setVisibility(View.GONE);
        tvTitle.setText(getString(R.string.setting));
        llReturn.setVisibility(View.VISIBLE);
    }

    private void gotoMain() {
        if (currentFragment == mainFragment) {
            return;
        }

        switchFragment(mainFragment).commit();
        tvMainTip.setVisibility(View.VISIBLE);
        tvSetTip.setVisibility(View.GONE);

        tvAppName.setVisibility(View.VISIBLE);
        llReturn.setVisibility(View.GONE);
    }

    private FragmentTransaction switchFragment(Fragment targetFragment) {
        FragmentTransaction transaction = getSupportFragmentManager().beginTransaction();
        if (!targetFragment.isAdded()) {
            if (currentFragment != null) {
                transaction.remove(currentFragment);    // fix bug 4532
            }
            transaction.add(R.id.fragment, targetFragment, targetFragment.getClass().getName());
        } else {
            transaction.remove(currentFragment).show(targetFragment);
        }
        currentFragment = targetFragment;
        return transaction;
    }


    private void registerLocalReceiver() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Action.AIRPLAY_MUSIC_START);
        intentFilter.addAction(Action.AIRPLAY_MUSIC_STOP);
        Action.registerLocalReceiver(getApplicationContext(), mLocalReceiver, intentFilter);
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause()  ");

        mHandler.removeCallbacks(showTimeRunnable);
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume()  ");

        mHandler.postDelayed(showTimeRunnable, 2000);

        String ssid = NetUtils.getWIFISSID(getApplicationContext());
        tvssid.setText(ssid);

        tvIP.setText(NetUtils.getIpAddress());
        devName.setText(Setting.getInstance().getName());
        tvVer.setText(getString(R.string.app_name) + " " + BuildConfig.VERSION_NAME);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        Log.d(TAG, "onKeyDown()  : keyCode = [" + keyCode + "], event = [" + event + "]");

        switch (keyCode) {
            case KeyEvent.KEYCODE_MENU:
            case KeyEvent.KEYCODE_DPAD_UP:
                gotoSetup();
                break;
            case KeyEvent.KEYCODE_BACK:
                if (onReturn()) {
                    return true;
                }
            default:
                break;
        }

        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onBackPressed() {
        finish();
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "onDestroy()  ");
        super.onDestroy();

        Action.unregisterLocalReceiver(mLocalReceiver);
        networkObserver.shutdown();
    }


}
