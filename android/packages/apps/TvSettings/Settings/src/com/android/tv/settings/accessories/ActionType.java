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

package com.android.tv.settings.accessories;

import com.android.tv.settings.ActionBehavior;
import com.android.tv.settings.ActionKey;
import com.android.tv.settings.R;
import com.android.tv.settings.dialog.old.Action;

import android.content.res.Resources;

enum ActionType {
    /*
     * General
     */
    OK(R.string.title_ok),
    CANCEL(R.string.title_cancel),

    /*
     * Bluetooth Device Settings
     */
    BLUETOOTH_DEVICE_OVERVIEW(R.string.accessory_options),
    BLUETOOTH_DEVICE_RENAME(R.string.accessory_rename),
    BLUETOOTH_DEVICE_UNPAIR(R.string.accessory_unpair);


    private final int mTitleResource;
    private final int mDescResource;

    private ActionType(int titleResource) {
        mTitleResource = titleResource;
        mDescResource = 0;
    }

    private ActionType(int titleResource, int descResource) {
        mTitleResource = titleResource;
        mDescResource = descResource;
    }

    String getTitle(Resources resources) {
        return resources.getString(mTitleResource);
    }

    String getDesc(Resources resources) {
        if (mDescResource != 0) {
            return resources.getString(mDescResource);
        }
        return null;
    }

    Action toAction(Resources resources) {
        return toAction(resources, getDesc(resources));
    }

    Action toAction(Resources resources, String description) {
        return new Action.Builder()
                .key(getKey(this, ActionBehavior.INIT))
                .title(getTitle(resources))
                .description(description)
                .build();
    }

    private String getKey(ActionType t, ActionBehavior b) {
        return new ActionKey<ActionType, ActionBehavior>(t, b).getKey();
    }
}
