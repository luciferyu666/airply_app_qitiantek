package com.aircast.app.ui;


import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.drawable.ColorDrawable;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.View.OnHoverListener;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;

import com.aircast.app.R;
import com.aircast.app.settings.fragment.BaseFragment;
import com.aircast.app.settings.model.ItemBean;
import com.aircast.app.settings.util.Timeout;
import com.aircast.app.settings.util.Utils;
import com.aircast.app.settings.view.ItemView;
import com.aircast.app.settings.view.MyScrollView;
import com.aircast.app.settings.view.MyScrollView.ScrollChangedListener;
import com.aircast.center.MediaRenderProxy;
import com.aircast.jni.PlatinumJniProxy;
import com.aircast.settings.Setting;

import java.util.ArrayList;

public class SetupFragment extends BaseFragment implements OnClickListener, OnFocusChangeListener, OnHoverListener, ScrollChangedListener, ItemView.SelectListener {
    public static final boolean USE_SYS_PLAYER = true;
    public static final boolean AIRPLAY_PWD = false;
    public static final boolean LOGIN = false;
    public static final boolean RTP_MODE = true;
    public static final boolean YOUTUBE = false;
    public static final String UDP = "UDP";
    public static final String TCP = "TCP";
    public static final String P1440 = "1440P";
    public static final String P1080 = "1080P";
    private static final String TAG = "SetupFragment";

    private ArrayList<ItemBean> itemList;
    private LinearLayout linearLayout;
    private MyScrollView scroll;
    private int viewID;
    private int height;
    //public static final String P720 = "720P";
    private androidx.appcompat.app.AlertDialog alert = null;

    public static SetupFragment newInstance() {
        return new SetupFragment();
    }

    public static void simulateKey(final int KeyCode) {
        new Thread(() -> {
            try {
                Instrumentation inst = new Instrumentation();
                inst.sendKeyDownUpSync(KeyCode);
            } catch (Exception e) {
                Log.e("hello", e.toString());
            }
        }
        ).start();
    }

    @Override
    public void onAttach(@NonNull Context context) {
        Log.d(TAG, "onAttach() ");
        super.onAttach(context);

    }

    @Override
    public int layoutId() {
        return R.layout.tv_fragment_setup;
    }

    @Override
    public void setupView() {
        this.scroll = getView().findViewById(R.id.content_sv);
        this.scroll.setOnScrollListener(this);
        this.linearLayout = getView().findViewById(R.id.content_ll);

    }

    @Override
    public void initView() {
        Log.d(TAG, "initView()  ");
        itemList = new ArrayList<>();

        ItemBean itemBean;
        itemBean = new ItemBean();
        itemBean.title = getString(R.string.server_name);
        itemBean.content = Setting.getInstance().getName();
        itemBean.viewType = ItemView.Type.TYPE_TEXT;
        itemBean.dividerHeight = Utils.dimOffset((int) R.dimen.px_positive_30);;
        itemBean.backgroundDrawable = R.drawable.selector_setting_item;
        this.itemList.add(itemBean);

        itemBean = new ItemBean();
        itemBean.title = getString(R.string.setting_mirror_resolution);
        itemBean.content = Setting.getInstance().getResName();
        itemBean.viewType = ItemView.Type.TYPE_OPTION;
        itemBean.option1 = P1080;
        itemBean.option2 = P1440;
        itemBean.listener = this;
        itemBean.dividerHeight = Utils.dimOffset((int) R.dimen.px_positive_30);
        itemBean.backgroundDrawable = R.drawable.selector_setting_item;
        this.itemList.add(itemBean);

        itemBean = new ItemBean();
        itemBean.title = getString(R.string.setting_max_frame_rate);
        itemBean.content = String.valueOf(Setting.getInstance().getMaxfps());
        itemBean.viewType = ItemView.Type.TYPE_OPTION;
        itemBean.option1 = "30";
        itemBean.option2 = "60";
        itemBean.listener = this;
        itemBean.dividerHeight = Utils.dimOffset((int) R.dimen.px_positive_30);
        itemBean.backgroundDrawable = R.drawable.selector_setting_item;
        this.itemList.add(itemBean);

        if (USE_SYS_PLAYER) {
            itemBean = new ItemBean();
            itemBean.title = getString(R.string.use_hwdecode);
            itemBean.content = Setting.getInstance().isUseMediaPlayer() ? getString(R.string.open) : getString(R.string.close);
            itemBean.viewType = ItemView.Type.TYPE_OPTION;
            itemBean.option1 = getString(R.string.open);
            itemBean.option2 = getString(R.string.close);
            itemBean.listener = this;
            itemBean.dividerHeight = Utils.dimOffset((int) R.dimen.px_positive_30);
            itemBean.backgroundDrawable = R.drawable.selector_setting_item;
            this.itemList.add(itemBean);
        }


        addView();
    }

    private void addView() {
        this.height = Utils.dimOffset((int) R.dimen.px_positive_100);
        LayoutParams layoutParams = new LayoutParams(Utils.dimOffset((int) R.dimen.px_positive_1000) + Utils.dimOffset((int) R.dimen.px_positive_440), this.height);

        if (this.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
            layoutParams = new LayoutParams(Utils.dimOffset((int) R.dimen.px_positive_700) + Utils.dimOffset((int) R.dimen.px_positive_240), this.height);
        }
        layoutParams.gravity = 16;

        for (int j = 0; j < itemList.size(); j++) {
            ItemBean itemBean = (ItemBean) this.itemList.get(j);
            ItemView v = new ItemView(getContext(), itemBean);
            v.setId(j);
            v.setOnClickListener(this);
            v.setOnFocusChangeListener(this);
            itemBean.itemView = v;
            this.linearLayout.addView(v, layoutParams);
            if (itemBean.dividerHeight > 0) {
                LayoutParams p = new LayoutParams(-1, itemBean.dividerHeight);
                this.linearLayout.addView(new View(getContext()), p);
            }


        }
    }

    @Override
    public void onSelectClick(ItemBean itemBean) {
        Log.d(TAG, "onSelectClick()  : itemBean = [" + itemBean.title + "]");
        if (getString(R.string.server_searchable).equals(itemBean.title)) {
            setDiscover(itemBean);

        } else if (getString(R.string.rtp_mode).equals(itemBean.title)) {

        } else if (getString(R.string.setting_mirror_resolution).equals(itemBean.title)) {
            setMirrorRes(itemBean);
        } else if (getString(R.string.use_hwdecode).equals(itemBean.title)) {
            setUseMediaPlayer(itemBean);
        } else if (getString(R.string.setting_mirror_youtube).equals(itemBean.title)) {
            setYoutubeMirror(itemBean);
        } else if (getString(R.string.setting_max_frame_rate).equals(itemBean.title)) {
            setMaxFps(itemBean);
        }

    }

    private void setYoutubeMirror(ItemBean itemBean) {

    }

    public void click(String str) {

        if (str.equals(getString(R.string.server_name))) {
            Intent intent = new Intent();
            intent.setClass(getActivity(), RenameActivity.class);
            intent.putExtra("type", 1);
            getActivity().startActivity(intent);
        }
        else if (str.equals(getString(R.string.signout))) {
            signoutDialog(getActivity());
        } else if (!str.equals(getString(R.string.server_searchable)) && !str.equals(getString(R.string.grab_whether))) {
            if (str.equals(getString(R.string.setting_mirror))) {

            } else if (str.equals(getString(R.string.detection))) {

            } else if (str.equals(getString(R.string.version_update))) {

            }
        }
    }

    @Override
    public void onFocusChange(View view, boolean hasFocus) {
        if (hasFocus) {
            handleFocus(view, this.viewID, view.getId());
            this.viewID = view.getId();
            Timeout.getInstance().add("onFocusChange", 1, new Timeout.TimeoutListener() {
                @Override
                public void OnTimeout(String str) {
                }

                @Override
                public void OnDelete(String str) {
                }

                @Override
                public void OnRemove(String str) {
                }
            });
        }
    }

    @Override
    public boolean onHover(View view, MotionEvent motionEvent) {
        if (motionEvent.getAction() == 9) {
            view.requestFocus();
        }
        return false;
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.back_layout) {
            if (getActivity() != null) {
                getActivity().finish();
            }
        }
        click(String.valueOf(view.getTag()));
    }

    private void startPkg(String pkg) {
        Intent intent;
        try {
            intent = getActivity().getPackageManager().getLaunchIntentForPackage(pkg);
            startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void setUseMediaPlayer(ItemBean itemBean) {
        boolean en = false;
        if (getString(R.string.open).equals(itemBean.content)) {
            itemBean.content = getString(R.string.close);
        } else {
            itemBean.content = getString(R.string.open);
            en = true;
        }

        Setting.getInstance().setUseMediaPlayer(en);
        itemBean.itemView.updateDes();
    }

    private void setDiscover(ItemBean itemBean) {
        if (getString(R.string.open).equals(itemBean.content)) {
            itemBean.content = getString(R.string.close);
        } else {
            itemBean.content = getString(R.string.open);
            ///ToastUtil.show((int) R.string.realse_server_hint);
        }
        //
        itemBean.itemView.updateDes();
    }

    private void setAirplayDND(ItemBean itemBean) {
        boolean en = false;
        if (Utils.getStr((int) R.string.open).equals(itemBean.content)) {
            itemBean.content = Utils.getStr((int) R.string.close);
            en = true;
            //ToastUtil.show((int) R.string.grab_hint);
        } else {
            itemBean.content = Utils.getStr((int) R.string.open);
        }


        PlatinumJniProxy.setDNDMode(en);
        itemBean.itemView.updateDes();
    }

    private void setMaxFps(ItemBean itemBean) {
        if ("30".equals(itemBean.content)) {
            itemBean.content = "60";
            Setting.getInstance().setMaxfps(60);
        } else {
            itemBean.content = "30";
            Setting.getInstance().setMaxfps(30);
        }

        MediaRenderProxy.getInstance().restartEngine();
        itemBean.itemView.updateDes();
    }

    private void setMirrorRes(ItemBean itemBean) {
        if (P1080.equals(itemBean.content)) {
            itemBean.content = P1440;
            Setting.getInstance().setResolution(1440);
        } else {
            itemBean.content = P1080;
            Setting.getInstance().setResolution(1080);
        }

        MediaRenderProxy.getInstance().restartEngine();
        itemBean.itemView.updateDes();
    }

    private void handleFocus(View view, int id, int vid) {
        if (vid > id) {
            if (view.getY() - ((float) this.scroll.getScrollY()) > ((float) (R.dimen.px_positive_500))) { // a(R.dimen.px_positive_500
                this.scroll.startScroll(0, ((ItemBean) this.itemList.get(id)).dividerHeight + this.height, 250);
            }
        } else if (view.getY() - ((float) this.scroll.getScrollY()) < ((float) this.height)) {
            int h = vid - 1;
            if (h >= 0) {
                h = ((ItemBean) this.itemList.get(h)).dividerHeight;
            } else {
                h = 0;
            }
            this.scroll.startScroll(0, -(h + this.height), 250);
        }
        if (vid == this.itemList.size() - 1) {
            //this.iv.setVisibility(View.GONE);
        } else {
            //this.iv.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onDestroy() {
        if (this.itemList != null) {
            this.itemList.clear();
            this.itemList = null;
        }
        super.onDestroy();

    }

    @Override
    public void onResume() {
        Log.d(TAG, "onResume()  ");

        for (int j = 0; j < itemList.size(); j++) {
            if (itemList.get(j).title.equals(getString(R.string.server_name))) {
                itemList.get(j).content = Setting.getInstance().getName();
                itemList.get(j).itemView.updateDes();
            }
        }

        simulateKey(KeyEvent.KEYCODE_DPAD_DOWN);
        super.onResume();
    }

    @Override
    public void onPause() {
        Setting.getInstance().commit();
        super.onPause();
    }

    @Override
    public void OnChange(int i, int i2, int i3, int i4) {
    }

    private void signoutDialog(Context context) {
        if (alert == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(context);

            final LayoutInflater inflater = getActivity().getLayoutInflater();
            View dialog = inflater.inflate(R.layout.tv_confirm_dialog, null, false);

            builder.setView(dialog);
            builder.setCancelable(false);
            alert = builder.create();
            alert.getWindow().setBackgroundDrawable(new ColorDrawable(0));

            dialog.findViewById(R.id.view1).setOnClickListener(v -> alert.dismiss());
            dialog.findViewById(R.id.view3).setOnClickListener(v -> {

                alert.dismiss();

            });
        }

        alert.show();
        alert.getWindow().setLayout(Utils.dimOffset(R.dimen.px_positive_500), Utils.dimOffset(R.dimen.px_positive_260)); //Controlling width and height.

    }
}
