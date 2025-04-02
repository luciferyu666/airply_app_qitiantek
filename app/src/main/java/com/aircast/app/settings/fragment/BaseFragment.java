package com.aircast.app.settings.fragment;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.fragment.app.Fragment;

import com.aircast.app.settings.util.Utils;

public abstract class BaseFragment extends Fragment {
    public abstract int layoutId();

    public abstract void setupView();

    public abstract void initView();


    @Override
    public View onCreateView(LayoutInflater layoutInflater, ViewGroup viewGroup, Bundle bundle) {
        return View.inflate(getActivity(), layoutId(), null);
    }

    @Override
    public void onViewCreated(View view, Bundle bundle) {
        super.onViewCreated(view, bundle);
        setupView();
        initView();
    }


    public int BaseFragment(int i) {
        return Utils.dimOffset(i);
    }
}
