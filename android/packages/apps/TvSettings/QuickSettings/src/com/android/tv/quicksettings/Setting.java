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
import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;

import java.util.ArrayList;
import java.util.List;

public class Setting implements Parcelable {

    static final int TYPE_UNKNOWN = 0;
    static final int TYPE_INT = 1;
    static final int TYPE_STRING = 2;

    private String mTitle;
    private int mIntValue;
    private String mStringValue;
    private int mSettingType;

    private int mMaxValue;
    private List<String> mStringChoices = new ArrayList<String>();

    public Setting() {
    }

    public Setting(String title) {
        mTitle = title;
        mSettingType = TYPE_UNKNOWN;
    }

    public Setting(String title, int value) {
        this(title, value, 0);
    }

    public Setting(String title, int value, int max) {
        this(title);
        mIntValue = value;
        mMaxValue = max;
        mSettingType = TYPE_INT;
    }

    public Setting(String title, String value) {
        this(title);
        mStringValue = value;
        mSettingType = TYPE_STRING;
    }

    public int getType() {
        return mSettingType;
    }

    public String getTitle() {
        return mTitle;
    }

    public void setTitle(String title) {
        mTitle = title;
    }

    public int getMaxValue() {
        return mMaxValue;
    }

    public void setMaxValue(int max) {
        mMaxValue = max;
        mSettingType = TYPE_INT;
    }

    public List<String> getStringChoices() {
        return mStringChoices;
    }

    public void setStringChoices(List<String> choices) {
        mStringChoices = choices;
        mSettingType = TYPE_STRING;
    }

    public void addStringChoice(String choice) {
        mStringChoices.add(choice);
    }

    public int getIntValue() {
        return mIntValue;
    }

    public String getStringValue() {
        return mStringValue;
    }

    public void setValue(int value) {
        mIntValue = value;
        mSettingType = TYPE_INT;
    }

    public void setValue(String value) {
        mStringValue = value;
        mSettingType = TYPE_STRING;
    }

    public static Parcelable.Creator<Setting> CREATOR = new Parcelable.Creator<Setting>() {
        @Override
        public Setting createFromParcel(Parcel source) {
            Setting setting = new Setting();
            setting.mTitle = source.readString();
            setting.mIntValue = source.readInt();
            setting.mStringValue = source.readString();
            setting.mSettingType = source.readInt();
            setting.mMaxValue = source.readInt();
            source.readStringList(setting.mStringChoices);
            return setting;
        }

        @Override
        public Setting[] newArray(int size) {
            return new Setting[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mTitle);
        dest.writeInt(mIntValue);
        dest.writeString(mStringValue);
        dest.writeInt(mSettingType);
        dest.writeInt(mMaxValue);
        dest.writeStringList(mStringChoices);
    }
}
