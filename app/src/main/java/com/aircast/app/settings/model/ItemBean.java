package com.aircast.app.settings.model;


import com.aircast.app.R;
import com.aircast.app.settings.view.ItemView;
import com.aircast.app.settings.view.ItemView.Type;

public class ItemBean {
    public int backgroundDrawable;
    public String content = "";
    public int contentColor = R.color.white;
    public boolean isVersion;
    public ItemView itemView;
    public int dividerHeight;
    public ItemView.SelectListener listener;
    public String option1;
    public String option2;
    public String title;
    public Type viewType;
}
