package com.aircast.app.ui;

import android.os.Bundle;
import android.text.InputFilter;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;

import androidx.fragment.app.FragmentActivity;

import com.aircast.app.R;
import com.aircast.app.settings.util.SimpleFilter;
import com.aircast.center.MediaRenderProxy;
import com.aircast.settings.Setting;


public class RenameActivity extends FragmentActivity implements OnClickListener {
    private String str;
    private EditText txt;

    public void onCreate(Bundle bundle) {
        super.onCreate(bundle);
        setContentView(R.layout.tv_activity_rename);
        this.txt = findViewById(R.id.rename_et);
        this.txt.setFilters(new InputFilter[]{new SimpleFilter(20)});
        this.str = Setting.getInstance().getName();
        this.txt.setText(this.str);
        this.txt.setSelection(this.txt.getText().length());
        findViewById(R.id.ok_tv).setOnClickListener(this);
        findViewById(R.id.cancel_tv).setOnClickListener(this);
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.cancel_tv:
                finish();
                return;
            case R.id.ok_tv:
                changeName();
                return;
            default:
        }
    }

    private void changeName() {
        String trim = this.txt.getText().toString().trim();
        if (!(TextUtils.isEmpty(trim) || TextUtils.isEmpty(this.str) || this.str.equals(trim))) {
            if (trim.contains("&")) {
                return;
            }

            Setting.getInstance().setName(trim);
            restartService();
        }
        finish();
    }

    private void restartService() {
        MediaRenderProxy.getInstance().restartEngine();

    }
}
