package com.aircast.util;

import android.content.Context;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import com.aircast.R;


public class ToastUtil {

    public static void show(Context context, int res) {
        View layout = LayoutInflater.from(context).inflate(R.layout.ap_tv_toast, null);
        TextView tv = (TextView) layout.findViewById(R.id.tip);
        tv.setText(res);

        Toast toast = new Toast(context);
        toast.setGravity(Gravity.BOTTOM, 0, 0);
        toast.setDuration(Toast.LENGTH_LONG);
        toast.setView(layout);
        toast.show();
    }
}
