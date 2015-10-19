/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.launcher2;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import com.android.launcher2.Launcher;
import android.os.SystemProperties;
import android.content.SharedPreferences;
import android.provider.Settings;

/**
 *
 * hold still wallpaper, wallpaper as launcher background
 * when receiver set static wallpaper, launcher set wallpaperServer unvisible,
 * and set wallpaper bitmap as launcher background.
 *
 ************************************
 *      
 *ActionsCode(phchen, new_method)
 */
public class WallpaperChangedReceiver extends BroadcastReceiver {
    public void onReceive(Context context, Intent data) {
        Launcher.mWallpaperChange = Launcher.STATIC_WALLPAPER;

        if ((LauncherApplication.mLauncher != null) && (LauncherApplication.mLauncher.usedStaticWallpaper())) {
            LauncherApplication.mLauncher.getDragLayer().invalidate();
        }
        Settings.Global.putInt(context.getContentResolver(),"launcher_wallpaper_status", 1);
    }
}
