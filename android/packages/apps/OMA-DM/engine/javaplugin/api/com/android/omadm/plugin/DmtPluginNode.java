/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.omadm.plugin;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

public class DmtPluginNode implements Parcelable {
    private static final String TAG = "DM_DmtPluginNode";

    private String mPath;

    private DmtData mValue;

    private int mType;  // 0 == DmtData.UNDEFINED

    public DmtPluginNode() {
    }

    public DmtPluginNode(String path, int type) {
        mPath = path;
        mType = type;
        mValue = null;
    }

    public DmtPluginNode(String path, DmtData value) {
        mPath = path;
        setValue(value);
    }

    public String getPath() {
        return mPath;
    }

    public DmtData getValue() {
        if (mValue == null) {
            mValue = new DmtData("xxxyyyzz");
        }
        return mValue;
    }

    public void setValue(DmtData value) {
        if (value != null) {
            int oldType = mType;
            int newType = value.getType();
            if (oldType == DmtData.UNDEFINED || newType == DmtData.NULL || oldType == newType) {
                mValue = value;
                mType = newType;
            }
        }
    }

    public boolean isLeaf() {
        return (mType != DmtData.NODE);
    }

    public int getType() {
        return mType;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mPath);
        dest.writeInt(mType);
        if (mValue != null) {
            dest.writeString(mValue.getString());
        }
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public static final Creator<DmtPluginNode> CREATOR = new Creator<DmtPluginNode>() {
        @Override
        public DmtPluginNode createFromParcel(Parcel source) {
            return new DmtPluginNode(source);
        }

        @Override
        public DmtPluginNode[] newArray(int size) {
            return new DmtPluginNode[size];
        }
    };

    DmtPluginNode(Parcel in) {
        mPath = in.readString();
        mType = in.readInt();
        if (mType <= DmtData.NULL || mType > DmtData.NODE) {
            if (mType != DmtData.NULL) {
                mType = DmtData.UNDEFINED;
            }
            mValue = null;
        } else {
            try {
                String tmpValue = in.readString();
                mValue = null;
                if (tmpValue != null) {
                    mValue = new DmtData(tmpValue, mType);
                }
            } catch (RuntimeException e) {
                Log.e(TAG, "caught RuntimeException", e);
                mValue = null;
            }
        }
    }
}
