/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.tv.quicksettings;

import android.content.Intent;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import java.util.ArrayList;

public class PanelAdapter extends RecyclerView.Adapter<PanelAdapter.ViewHolder> {

    static class ViewHolder extends RecyclerView.ViewHolder {
        private final TextView mTitle;
        private final TextView mValue;
        private Setting mSetting;

        public ViewHolder(View itemView) {
            super(itemView);
            mTitle = (TextView) itemView.findViewById(R.id.setting_title);
            mValue = (TextView) itemView.findViewById(R.id.setting_value);
        }

        public void setSetting(Setting setting) {
            mSetting = setting;
        }
    }

    private static final String TAG = "QuickSettings";

    private final SettingClickedListener mSettingClickedListener;
    private ArrayList<Setting> mSettings;

    public PanelAdapter(ArrayList<Setting> settings, SettingClickedListener settingClickedListener) {
        mSettings = settings;
        mSettingClickedListener = settingClickedListener;
    }

    @Override
    public ViewHolder onCreateViewHolder(final ViewGroup parent, int viewType) {
        View v = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.setting, parent, false);
        final ViewHolder vh = new ViewHolder(v);
        v.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Setting s = vh.mSetting;
                if (s != null) {
                    mSettingClickedListener.onSettingClicked(s);
                }
            }
        });
        return vh;
    }

    @Override
    public void onBindViewHolder(ViewHolder holder, int position) {
        Setting s = mSettings.get(position);
        holder.setSetting(s);
        holder.mTitle.setText(s.getTitle());
        switch (s.getType()) {
            case Setting.TYPE_INT:
                holder.mValue.setText(Integer.toString(s.getIntValue()));
                break;
            case Setting.TYPE_STRING:
                holder.mValue.setText(s.getStringValue());
                break;
            default:
                holder.mValue.setText("");
                break;
        }
        if (TextUtils.isEmpty(holder.mValue.getText())) {
            holder.mValue.setVisibility(View.GONE);
        } else {
            holder.mValue.setVisibility(View.VISIBLE);
        }
    }

    @Override
    public void onViewRecycled(ViewHolder holder) {
        holder.setSetting(null);
    }

    @Override
    public int getItemCount() {
        return mSettings.size();
    }

    public void setSettings(ArrayList<Setting> settings) {
        mSettings = settings;
        notifyDataSetChanged();
    }
}
