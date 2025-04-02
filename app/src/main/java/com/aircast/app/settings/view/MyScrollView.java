package com.aircast.app.settings.view;

import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.widget.ScrollView;
import android.widget.Scroller;

public class MyScrollView extends ScrollView {
    private ScrollChangedListener listener;
    private Scroller scroller;

    public MyScrollView(Context context) {
        super(context);
        init(context);
    }

    public MyScrollView(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
        init(context);
    }

    public static void setTransitionGroup(ViewGroup viewGroup) {
        try {
            if (Build.VERSION.SDK_INT >= 21) {
                viewGroup.setTransitionGroup(true);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void init(Context context) {
        this.scroller = new Scroller(context);
        setTransitionGroup((ViewGroup) this);
    }

    public void setOnScrollListener(ScrollChangedListener aVar) {
        this.listener = aVar;
    }

    public void onScrollChanged(int i, int i2, int i3, int i4) {
        super.onScrollChanged(i, i2, i3, i4);
        if (this.listener != null) {
            this.listener.OnChange(i, i2, i3, i4);
        }
    }

    public void startScroll(int i, int i2, int i3) {
        this.scroller.startScroll(getScrollX(), getScrollY(), i, i2, i3);
        invalidate();
    }

    public void computeScroll() {
        if (this.scroller.computeScrollOffset()) {
            scrollTo(this.scroller.getCurrX(), this.scroller.getCurrY());
            postInvalidate();
        }
        super.computeScroll();
    }

    public interface ScrollChangedListener {
        void OnChange(int i, int i2, int i3, int i4);
    }
}
