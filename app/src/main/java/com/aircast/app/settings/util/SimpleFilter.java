package com.aircast.app.settings.util;

import android.text.InputFilter;
import android.text.Spanned;

public class SimpleFilter implements InputFilter {
    int length;

    public SimpleFilter(int i) {
        this.length = i;
    }

    public CharSequence filter(CharSequence charSequence, int i, int i2, Spanned spanned, int i3, int i4) {
        if ((spanned.toString().length() + Utils.regex(spanned.toString())) + (charSequence.toString().length() + Utils.regex(charSequence.toString())) > this.length) {
            return "";
        }
        return charSequence;
    }
}
