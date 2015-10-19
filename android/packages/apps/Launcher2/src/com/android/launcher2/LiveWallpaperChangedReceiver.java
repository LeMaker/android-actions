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
 * when receiver set live wallpaper, launcher set wallpaperServer visible,
 * and set launcher background nothing.
 *
 ************************************
 *      
 *ActionsCode(phchen, new_method)
 */
public class LiveWallpaperChangedReceiver extends BroadcastReceiver {
    public void onReceive(Context context, Intent data) {
        Launcher.mWallpaperChange = Launcher.LIVE_WALLPAPER;
        if (LauncherApplication.mLauncher != null) {
   	        LauncherApplication.mLauncher.updateWallpaperVisibility(true);
        }
        Settings.Global.putInt(context.getContentResolver(),"launcher_wallpaper_status", 2);	
    }
}
