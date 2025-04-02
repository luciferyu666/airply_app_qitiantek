package com.aircast.app.settings.util;

import android.app.Dialog;
import android.content.Context;
import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

import com.aircast.app.R;


public class Dialogs {

    private static Dialog init(Context context, String str, String str2, boolean z, boolean z2, boolean z3, boolean z4, String str3, String str4, String str5, ClickListener aVar) {
        final Dialog dialog = new Dialog(context, R.style.custom_dialog);
        View inflate = View.inflate(context, R.layout.dialog_confirm, null);
        dialog.setContentView(inflate);
        TextView title = (TextView) inflate.findViewById(R.id.dialog_confirm_title);
        title.setText(str);
        TextView body = (TextView) inflate.findViewById(R.id.dialog_confirm_body);
        body.setText(str2);
        Button okbtn = (Button) inflate.findViewById(R.id.dialog_cofirm_ok_btn);
        final ClickListener aVar2 = aVar;
        okbtn.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                if (aVar2 != null) {
                    aVar2.click(dialog, 0);
                }
                dialog.dismiss();
            }
        });
        if (!TextUtils.isEmpty(str5)) {
            okbtn.setText(str5);
        }
        Button button2 = (Button) inflate.findViewById(R.id.dialog_cofirm_cancel_btn);
        //aVar2 = aVar;
        button2.setOnClickListener(new OnClickListener() {
            public void onClick(View view) {
                if (aVar2 != null) {
                    aVar2.click(dialog, 1);
                }
                dialog.dismiss();
            }
        });
        if (!TextUtils.isEmpty(str4)) {
            button2.setText(str4);
        }
        if (!z) {
            title.setVisibility(View.GONE);
        }
        if (!z2) {
            body.setVisibility(View.INVISIBLE);
        }
        if (!z3) {
            okbtn.setVisibility(View.GONE);
        }
        if (!z4) {
            okbtn.setVisibility(View.GONE);
        }
        title = (TextView) inflate.findViewById(R.id.dialog_for_tag);
        if (!TextUtils.isEmpty(str3)) {
            title.setText(str3);
        }
        return dialog;
    }

    public static Dialog init(Context context, String str, ClickListener aVar) {
        return init(context, "", str, false, true, true, true, null, null, null, aVar);
    }

    public static Dialog initOK(Context context, String str, ClickListener aVar) {
        return init(context, "", str, false, true, true, true, null, "我知道了", "去设置", aVar);
    }

    public static Dialog initInfo(Context context, String str, String str2, ClickListener aVar) {
        return init(context, "", str, false, true, true, true, str2, null, null, aVar);
    }

    public static Dialog init(Context context, View view) {
        Dialog dialog = new Dialog(context);
        dialog.getWindow().requestFeature(1);
        dialog.getWindow().setBackgroundDrawableResource(17170445);
        dialog.setContentView(view);
        return dialog;
    }

    public interface ClickListener {
        void click(Dialog dialog, int i);
    }


    public interface EditListener {
        void edit(int i, String str);
    }
}
