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

package com.android.tv.settings.device.storage;

import com.android.tv.settings.R;
import com.android.tv.settings.dialog.old.Action;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RectShape;

public enum StorageItem {
    AVAILABLE(R.string.storage_available, R.color.storage_avail,
            R.drawable.storage_indicator_available),
    APPS(R.string.storage_apps_usage, R.color.storage_apps_usage,
            R.drawable.storage_indicator_apps),
    PICTURES_VIDEO(R.string.storage_dcim_usage, R.color.storage_dcim,
            R.drawable.storage_indicator_dcim),
    AUDIO(R.string.storage_music_usage, R.color.storage_music, R.drawable.storage_indicator_music),
    DOWNLOADS(R.string.storage_downloads_usage, R.color.storage_downloads,
            R.drawable.storage_indicator_downloads),
    CACHED_DATA(R.string.storage_media_cache_usage, R.color.storage_cache,
            R.drawable.storage_indicator_cache),
    MISC(R.string.storage_media_misc_usage, R.color.storage_misc,
            R.drawable.storage_indicator_misc);

    private final int mTitleResource;
    private final int mColorResource;
    private final int mIndicatorResource;

    private StorageItem(int titleResource, int colorResource, int indicatorResource) {
        mTitleResource = titleResource;
        mColorResource = colorResource;
        mIndicatorResource = indicatorResource;
    }

    public int getColor(Resources resources) {
        return resources.getColor(mColorResource);
    }

    Action toAction(Resources resources, String description) {
        return new Action.Builder()
                .key(name())
                .title(resources.getString(mTitleResource))
                .description(description)
                .drawableResource(mIndicatorResource)
                .infoOnly(true)
                .build();
    }
}
