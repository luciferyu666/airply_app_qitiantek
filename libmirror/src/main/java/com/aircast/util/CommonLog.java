package com.aircast.util;

import android.util.Log;

public class CommonLog {
    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();
    private static int logLevel = Log.VERBOSE;
    private static boolean isDebug = true;
    private String tag = "CommonLog";

    public CommonLog() {
    }

    public CommonLog(String tag) {
        this.tag = tag;
    }

    public static String bytesToHex(byte[] bytes, int len) {
        len = Math.min(len, bytes.length);

        char[] hexChars = new char[len * 2];
        for (int j = 0; j < len; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[j * 2 + 1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars);
    }

    public void setTag(String tag) {
        this.tag = tag;
    }

    private String getFunctionName() {
        StackTraceElement[] sts = Thread.currentThread().getStackTrace();

        if (sts == null) {
            return null;
        }


        for (StackTraceElement st : sts) {
            if (st.isNativeMethod()) {
                continue;
            }

            if (st.getClassName().equals(Thread.class.getName())) {
                continue;
            }

            if (st.getClassName().equals(this.getClass().getName())) {
                continue;
            }

            return "[" + Thread.currentThread().getId() + ": " + st.getFileName() + ":" + st.getLineNumber() + "]";
        }

        return null;
    }

    public void info(Object str) {
        if (logLevel <= Log.INFO) {
            String name = getFunctionName();
            String ls = (name == null ? str.toString() : (name + " - " + str));
            Log.i(tag, ls);
        }
    }

    public void i(Object str) {
        if (isDebug) {
            info(str);
        }
    }

    public void verbose(Object str) {
        if (logLevel <= Log.VERBOSE) {
            String name = getFunctionName();
            String ls = (name == null ? str.toString() : (name + " - " + str));
            Log.v(tag, ls);
        }
    }

    public void v(Object str) {
        if (isDebug) {
            verbose(str);
        }
    }

    public void warn(Object str) {
        if (logLevel <= Log.WARN) {
            String name = getFunctionName();
            String ls = (name == null ? str.toString() : (name + " - " + str));
            Log.w(tag, ls);
        }
    }

    public void w(Object str) {
        if (isDebug) {
            warn(str);
        }
    }

    public void error(Object str) {
        if (logLevel <= Log.ERROR) {
            String name = getFunctionName();
            String ls = (name == null ? str.toString() : (name + " - " + str));
            Log.e(tag, ls);
        }
    }

    public void error(Exception ex) {
        if (logLevel <= Log.ERROR) {
            StringBuffer sb = new StringBuffer();
            String name = getFunctionName();
            StackTraceElement[] sts = ex.getStackTrace();

            if (name != null) {
                sb.append(name + " - " + ex + "\r\n");
            } else {
                sb.append(ex + "\r\n");
            }

            if (sts != null && sts.length > 0) {
                for (StackTraceElement st : sts) {
                    if (st != null) {
                        sb.append("[ " + st.getFileName() + ":" + st.getLineNumber() + " ]\r\n");
                    }
                }
            }

            Log.e(tag, sb.toString());
        }
    }

    public void e(Object str) {
        if (isDebug) {
            error(str);
        }
    }

    public void e(Exception ex) {
        if (isDebug) {
            error(ex);
        }
    }

    public void debug(Object str) {
        if (logLevel <= Log.DEBUG) {
            String name = getFunctionName();
            String ls = (name == null ? str.toString() : (name + " - " + str));
            Log.d(tag, ls);
        }
    }

    public void d(Object str) {
        if (isDebug) {
            debug(str);
        }
    }
}