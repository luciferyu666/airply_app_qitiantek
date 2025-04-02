package com.aircast.app.settings.util;

import android.os.Handler;

import com.aircast.app.settings.model.TimeOutCheckBean;

import java.util.HashMap;
import java.util.Map;

public class Timeout {
    private static volatile Timeout timeout;
    private Map<String, TimeOutCheckBean> checkBeanMap = new HashMap();
    private Map<String, Runnable> runnableMap = new HashMap();
    private Handler handler = new Handler();

    private Timeout() {
    }

    public static Timeout getInstance() {
        if (timeout == null) {
            synchronized (Timeout.class) {
                if (timeout == null) {
                    timeout = new Timeout();
                }
            }
        }
        return timeout;
    }

    public static void clear() {
        if (timeout != null && timeout.runnableMap != null) {
            timeout.runnableMap.clear();
            timeout = null;
        }
    }

    public void add(String str, long delayMillis, TimeoutListener aVar) {
        TimeOutCheckBean timeOutCheckBean = new TimeOutCheckBean();
        timeOutCheckBean.id = str;
        timeOutCheckBean.outTime = delayMillis;
        timeOutCheckBean.timeOutListener = aVar;
        this.checkBeanMap.put(str, timeOutCheckBean);
        MyRunnable anonymousClass1 = new MyRunnable() {

            public void run() {
                if (this.timeOutCheckBean.timeOutListener != null) {
                    this.timeOutCheckBean.timeOutListener.OnTimeout(this.timeOutCheckBean.id);
                }
                Timeout.this.checkBeanMap.remove(this.timeOutCheckBean.id);
            }
        };
        anonymousClass1.timeOutCheckBean = timeOutCheckBean;
        this.runnableMap.put(timeOutCheckBean.id, anonymousClass1);
        this.handler.postDelayed(anonymousClass1, delayMillis);
    }

    public void remove(String str) {
        this.handler.removeCallbacks((Runnable) this.runnableMap.get(str));
        TimeOutCheckBean timeOutCheckBean = (TimeOutCheckBean) this.checkBeanMap.get(str);
        if (timeOutCheckBean != null) {
            if (timeOutCheckBean.timeOutListener != null) {
                timeOutCheckBean.timeOutListener.OnRemove(str);
            }
            this.checkBeanMap.remove(str);
        }
    }

    public void delete(String str) {
        this.handler.removeCallbacks((Runnable) this.runnableMap.get(str));
        TimeOutCheckBean timeOutCheckBean = (TimeOutCheckBean) this.checkBeanMap.get(str);
        if (timeOutCheckBean != null) {
            if (timeOutCheckBean.timeOutListener != null) {
                timeOutCheckBean.timeOutListener.OnDelete(str);
            }
            this.checkBeanMap.remove(str);
        }
    }

    public interface TimeoutListener {
        void OnTimeout(String str);

        void OnDelete(String str);

        void OnRemove(String str);
    }

    static abstract class MyRunnable implements Runnable {
        public TimeOutCheckBean timeOutCheckBean;

        MyRunnable() {
        }
    }
}
