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

package com.android.omadm.service;

public final class DMSettingsHelper {
    public static final String TAG = "DMSettingsHelper";
    public static final String AUTHORITY = "com.android.omadm.service";
    public static final String DM_TREE = "dmtree";
    public static final String DM_TREE_STATUS = "dmtreestatus";
    public static final String DM_DMFLEXS = "flexs";
    public static final String DATABASE_NAME = "DmConfigure.db";

    public static final String BLOB = "blob"; //only for DM bootstrap

    // columns for data
    public static final String COL_ROOT_NODE = "rootnode";
    public static final String COL_SERVER_ID = "serverid";

    // DMT availability. Fields for the cursor and values
    public static final String COL_DM_AVAILABILITY = "dmavailability";
    public static final String AVAILABLE = "available";
    public static final String LOCKED = "locked";

    private DMSettingsHelper() {
    }

    // TODO: For now, assume the device supports LTE.
    public static boolean isPhoneTypeLTE() {
        return true;
    }
}
