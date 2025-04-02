package com.aircast.app.settings.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.aircast.app.R;
import com.aircast.app.settings.util.Utils;

import java.util.ArrayList;
import java.util.List;

public abstract class FocusListView<T> extends RelativeLayout {
    private List<T> list = new ArrayList();
    private Context context;
    private LinearLayout linearLayout;
    private int d = 0;

    public FocusListView(Context context) {
        super(context);
        this.context = context;
        init();
    }

    public FocusListView(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
        this.context = context;
        init();
    }

    public FocusListView(Context context, AttributeSet attributeSet, int i) {
        super(context, attributeSet, i);
        this.context = context;
        init();
    }

    public abstract void a(int i, T t);

    public abstract void a(TextView textView, int i, T t);

    public abstract void b(int i, T t);

    private void init() {
        setClipChildren(false);
        this.linearLayout = new LinearLayout(this.context);
        this.linearLayout.setOrientation(1);
        this.linearLayout.setClipChildren(false);
        LayoutParams layoutParams = new LayoutParams(-2, -2);
        layoutParams.addRule(14);
        addView(this.linearLayout, layoutParams);
    }

    private void b() {
        this.linearLayout.removeAllViews();
        if (this.list != null && this.list.size() > 0) {
            for (int i = 0; i < this.list.size(); i++) {
                final TextView textView = new TextView(this.context);
                textView.setTextColor(-1);
                textView.setTextSize(0, (float) Utils.dimOffset((int) R.dimen.px_positive_36));
                textView.setGravity(17);
                textView.setBackgroundResource(R.drawable.selector_setting_item);
                LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(Utils.dimOffset((int) R.dimen.px_positive_800), Utils.dimOffset((int) R.dimen.px_positive_102));
                layoutParams.topMargin = Utils.dimOffset((int) R.dimen.px_positive_8);
                if (i == this.list.size() - 1) {
                    layoutParams.bottomMargin = layoutParams.topMargin * 2;
                } else {
                    layoutParams.bottomMargin = layoutParams.topMargin;
                }
                this.linearLayout.addView(textView, layoutParams);
                textView.setFocusable(true);
                textView.setFocusableInTouchMode(true);
                textView.setClickable(true);
                if (i == this.d) {
                    textView.requestFocus();
                }
                final T obj = this.list.get(i);
                final int ii = i;
                a(textView, i, obj);
                textView.setOnFocusChangeListener(new OnFocusChangeListener() {

                    public void onFocusChange(View view, boolean z) {
                        FocusListView.this.b(ii, obj);
                        Utils.anim2(view, z ? 1.03f : 1.0f);
                    }
                });
                textView.setOnClickListener(new OnClickListener() {
                    public void onClick(View view) {
                        FocusListView.this.a(ii, obj);
                    }
                });
                textView.setOnHoverListener(new OnHoverListener() {
                    public boolean onHover(View view, MotionEvent motionEvent) {
                        if (motionEvent.getAction() == 9) {
                            textView.requestFocus();
                        } else if (motionEvent.getAction() == 10) {
                        }
                        return false;
                    }
                });
            }
        }
    }

    public void setDataList(List<T> list) {
        this.list.clear();
        if (list != null && list.size() > 0) {
            this.list.addAll(list);
        }
        b();
    }

    public void setDataArr(T[] tArr) {
        a((T[]) tArr, 0);
    }

    public void a(T[] tArr, int i) {
        this.list.clear();
        if (tArr != null && tArr.length > 0) {
            for (T add : tArr) {
                this.list.add(add);
            }
        }
        this.d = i;
        b();
    }
}
