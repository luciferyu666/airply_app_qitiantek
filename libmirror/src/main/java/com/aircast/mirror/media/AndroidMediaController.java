package com.aircast.mirror.media;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.MediaController;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;

import java.util.ArrayList;

public class AndroidMediaController extends MediaController implements IMediaController {
    private ActionBar mActionBar;
    //----------
    // Extends
    //----------
    private ArrayList<View> mShowOnceArray = new ArrayList<View>();

    public AndroidMediaController(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView(context);
    }

    public AndroidMediaController(Context context, boolean useFastForward) {
        super(context, useFastForward);
        initView(context);
    }

    public AndroidMediaController(Context context) {
        super(context);
        initView(context);
    }

    private void initView(Context context) {
    }

    public void setSupportActionBar(@Nullable ActionBar actionBar) {
        mActionBar = actionBar;
        if (isShowing()) {
            actionBar.show();
        } else {
            actionBar.hide();
        }
    }

    @Override
    public void show() {
        super.show();
        if (mActionBar != null)
            mActionBar.show();
    }

    @Override
    public void hide() {
        super.hide();
        if (mActionBar != null)
            mActionBar.hide();
        for (View view : mShowOnceArray)
            view.setVisibility(View.GONE);
        mShowOnceArray.clear();
    }

    public void showOnce(@NonNull View view) {
        mShowOnceArray.add(view);
        view.setVisibility(View.VISIBLE);
        show();
    }
}
