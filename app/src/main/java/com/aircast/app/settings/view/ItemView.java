package com.aircast.app.settings.view;

import android.content.Context;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.View.OnHoverListener;
import android.view.View.OnKeyListener;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.aircast.app.R;
import com.aircast.app.settings.model.ItemBean;
import com.aircast.app.settings.util.Timeout;
import com.aircast.app.settings.util.Utils;


public class ItemView extends RelativeLayout implements OnClickListener, OnFocusChangeListener, OnHoverListener, OnKeyListener, Timeout.TimeoutListener {
    private static final String TAG = "ItemView";
    private static final int b = 30000;
    private OnClickListener clickListener;
    private OnFocusChangeListener focusListener;
    private ItemBean bean;
    private TextView tvDes;
    private ImageView leftImage;
    private ImageView rightView;

    public ItemView(Context context, ItemBean itemBean) {
        super(context);
        this.bean = itemBean;
        init();
    }

    private void init() {
        setTag(this.bean.title);
        setBackgroundResource(this.bean.backgroundDrawable);
        setFocusable(true);
        setFocusableInTouchMode(true);
        super.setOnClickListener(this);
        super.setOnFocusChangeListener(this);
        super.setOnHoverListener(this);
        super.setOnKeyListener(this);
        LayoutParams layoutParams = new LayoutParams(-2, -2);
        layoutParams.leftMargin = Utils.dimOffset((int) R.dimen.px_positive_66);
        layoutParams.addRule(15);
        TextView textView = new TextView(getContext());
        textView.setText(this.bean.title);
        textView.setTextColor(getResources().getColor(R.color.white));
        textView.setTextSize(0, (float) Utils.dimOffset((int) R.dimen.px_positive_32));
        addView(textView, layoutParams);
        switch (this.bean.viewType) {
            case TYPE_TEXT:
                addText();
                return;
            case TYPE_OPTION:
                addSelect();
                return;
            default:
                return;
        }
    }

    private void addText() {
        ImageView imageView = new ImageView(getContext());
        imageView.setId(20000);
        imageView.setScaleType(ScaleType.FIT_XY);
        imageView.setImageResource(R.mipmap.right_arrow);
        LayoutParams lp = new LayoutParams(Utils.dimOffset((int) R.dimen.px_positive_21), Utils.dimOffset((int) R.dimen.px_positive_36));
        lp.addRule(15);
        lp.setMargins(0, 0, Utils.dimOffset((int) R.dimen.px_positive_53), 0);
        lp.addRule(11);
        addView(imageView, lp);
        this.tvDes = new TextView(getContext());
        this.tvDes.setGravity(17);
        this.tvDes.setTextColor(getResources().getColor(this.bean.contentColor));
        this.tvDes.setText(this.bean.content);
        this.tvDes.setTextSize(0, (float) Utils.dimOffset((int) R.dimen.px_positive_32));
        LayoutParams layoutParams2 = new LayoutParams(-2, -2);
/*        if (this.e.isVerion && i.a().b) {
            this.f.setText(R.string.soft_local_update_info);
            this.f.setTextSize(0, (float) p.b((int) R.dimen.px_positive_22));
            this.f.setBackgroundResource(R.drawable.new_version_bg_n);
            this.f.setPadding(p.b((int) R.dimen.px_positive_20), 0, p.b((int) R.dimen.px_positive_20), 0);
            layoutParams2.height = p.b((int) R.dimen.px_positive_44);
        }*/
        layoutParams2.addRule(15);
        layoutParams2.addRule(0, 20000);
        layoutParams2.rightMargin = Utils.dimOffset((int) R.dimen.px_positive_28);
        addView(this.tvDes, layoutParams2);
    }

    private void addSelect() {
        this.rightView = new ImageView(getContext());
        this.rightView.setId(30001);
        this.rightView.setOnClickListener(this);
        this.rightView.setScaleType(ScaleType.FIT_XY);
        this.rightView.setImageResource(R.drawable.icon_zx_r);
        LayoutParams lp = new LayoutParams(Utils.dimOffset((int) R.dimen.px_positive_45), Utils.dimOffset((int) R.dimen.px_positive_45));
        lp.addRule(15);
        lp.setMargins(0, 0, Utils.dimOffset((int) R.dimen.px_positive_53), 0);
        lp.addRule(11);
        addView(this.rightView, lp);
        this.tvDes = new TextView(getContext());
        this.tvDes.setId(30100);
        this.tvDes.setGravity(17);
        this.tvDes.setTextColor(getResources().getColor(this.bean.contentColor));
        this.tvDes.setText(this.bean.content);
        this.tvDes.setTextSize(0, (float) Utils.dimOffset((int) R.dimen.px_positive_32));
        LayoutParams layoutParams2 = new LayoutParams(Utils.dimOffset((int) R.dimen.px_positive_118), -2);
        lp.setMargins(0, 0, Utils.dimOffset((int) R.dimen.px_positive_53), 0);
        layoutParams2.addRule(15);
        layoutParams2.addRule(0, 30001);
        addView(this.tvDes, layoutParams2);
        this.leftImage = new ImageView(getContext());
        this.leftImage.setId(30002);
        this.leftImage.setOnClickListener(this);
        this.leftImage.setScaleType(ScaleType.FIT_XY);
        this.leftImage.setImageResource(R.drawable.icon_zx_l);
        lp = new LayoutParams(Utils.dimOffset((int) R.dimen.px_positive_45), Utils.dimOffset((int) R.dimen.px_positive_45));
        lp.addRule(15);
        lp.addRule(0, 30100);
        addView(this.leftImage, lp);
    }

    public void updateDes() {
        this.tvDes.setText(this.bean.content);
    }

    public void setDesText(String str) {
        this.tvDes.setText(str);
    }

    public void setOnClickListener(OnClickListener onClickListener) {
        this.clickListener = onClickListener;
    }

    public void setOnFocusChangeListener(OnFocusChangeListener onFocusChangeListener) {
        this.focusListener = onFocusChangeListener;
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case 30001:
                clickRight();
                break;
            case 30002:
                clickLeft();
                break;
            default:
                doSelect();
                break;
        }
        if (this.clickListener != null) {
            this.clickListener.onClick(view);
        }
    }

    public void onFocusChange(View view, boolean z) {
        if (this.focusListener != null) {
            this.focusListener.onFocusChange(view, z);
        }
        //Utils.anim2(view, z ? 1.01f : 1.0f);
    }

    public boolean onHover(View view, MotionEvent motionEvent) {
        if (motionEvent.getAction() == 9) {
            view.requestFocus();
        }
        return false;
    }

    public boolean onKey(View view, int i, KeyEvent keyEvent) {
        switch (keyEvent.getKeyCode()) {
            case KeyEvent.KEYCODE_DPAD_LEFT:
                if (keyEvent.getAction() == 0) {
                    clickLeft();
                    break;
                }
                break;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                if (keyEvent.getAction() == 0) {
                    clickRight();
                    break;
                }
                break;
        }
        return false;
    }

    private void clickLeft() {
        if (this.leftImage != null) {
            doSelect();
            this.leftImage.setVisibility(View.INVISIBLE);
            Timeout.getInstance().add("mLeftArrowIv", 200, this);
        }
    }

    private void clickRight() {
        if (this.rightView != null) {
            doSelect();
            this.rightView.setVisibility(View.INVISIBLE);
            Timeout.getInstance().add("mRightArrowIv", 200, this);
        }
    }

    private void doSelect() {
        if (this.bean.listener != null) {
            this.bean.listener.onSelectClick(this.bean);
        }
    }

    public void OnTimeout(String str) {
        if ("mLeftArrowIv".equals(str)) {
            this.leftImage.setVisibility(View.VISIBLE);
        } else if ("mRightArrowIv".equals(str)) {
            this.rightView.setVisibility(View.VISIBLE);
        }
    }

    public void OnDelete(String str) {
    }

    public void OnRemove(String str) {
    }

    public enum Type {
        TYPE_TEXT,
        TYPE_OPTION
    }

    public interface SelectListener {  //listener
        void onSelectClick(ItemBean itemBean);
    }
}
