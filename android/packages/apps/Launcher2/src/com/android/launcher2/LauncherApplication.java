/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.app.Application;
import android.app.SearchManager;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.LauncherApps;
import android.content.res.Configuration;
import android.database.ContentObserver;
import android.os.Handler;
import android.os.SystemProperties;
import android.util.Log;
import android.util.Xml;
import com.android.launcher.R;
import java.io.IOException;
import java.io.InputStream;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Iterator;
import org.xml.sax.SAXException;
import android.hardware.display.DisplayManagerGlobal;
import android.view.Display;
import android.view.DisplayInfo;

public class LauncherApplication extends Application {
    static final String TAG = "LauncherApplication";
    private LauncherModel mModel;
    private IconCache mIconCache;
    private static String DEFAULT_CLING_ENABLE = "enable";
    private static String DEFAULT_CLING_DISENABLE = "disable";
    private static int DEFAULT_XY = 0;
    private static String DEFAULT_HIDEACTIVITY_ENABLE = "enable";
    private static String DEFAULT_HIDEACTIVITY_DISENABLE = "disable";
    private static String DEFAULT_SWIPE_ENABLE = "enable";
    private static String DEFAULT_SWIPE_DISENABLE = "disable";
    private WidgetPreviewLoader.CacheDb mWidgetPreviewCacheDb;
    private static boolean sIsScreenLarge;
    private static boolean mStillWallpaper;
    private static float sScreenDensity;
    private static int sLongPressTimeout = 300;
    private static final String sSharedPreferencesKey = "com.android.launcher2.prefs";
    WeakReference<LauncherProvider> mLauncherProvider;

    public static Launcher mLauncher = null;
    public static int mLogicalWidth;
    public static int mLogicalHeight;
	
    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    *ActionsCode(author:phchen, change_code)
    */
    @Override
    public void onCreate() {
        super.onCreate();

        // set sIsScreenXLarge and sScreenDensity *before* creating icon cache
        sIsScreenLarge = getResources().getBoolean(R.bool.is_large_screen);
        sScreenDensity = getResources().getDisplayMetrics().density;

        //ActionsCode(phchen, NEW FEATURE: hold still wallpaper ),
        mStillWallpaper = SystemProperties.getBoolean("ro.still.wallpaper" , true);
        DisplayInfo di = DisplayManagerGlobal.getInstance().getDisplayInfo(Display.DEFAULT_DISPLAY);
        if (di.logicalWidth > di.logicalHeight) {
            mLogicalWidth = di.logicalWidth;
            mLogicalHeight = di.logicalHeight;
        }else {
            mLogicalWidth = di.logicalHeight;
            mLogicalHeight = di.logicalWidth;
        }
        
        recreateWidgetPreviewDb();
        mIconCache = new IconCache(this);
        mModel = new LauncherModel(this, mIconCache);
        LauncherApps launcherApps = (LauncherApps)
                getSystemService(Context.LAUNCHER_APPS_SERVICE);
        launcherApps.registerCallback(mModel.getLauncherAppsCallback());

        // Register intent receivers
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_LOCALE_CHANGED);
        filter.addAction(Intent.ACTION_CONFIGURATION_CHANGED);
        registerReceiver(mModel, filter);
        filter = new IntentFilter();
        filter.addAction(SearchManager.INTENT_GLOBAL_SEARCH_ACTIVITY_CHANGED);
        registerReceiver(mModel, filter);
        filter = new IntentFilter();
        filter.addAction(SearchManager.INTENT_ACTION_SEARCHABLES_CHANGED);
        registerReceiver(mModel, filter);

        // Register for changes to the favorites
        ContentResolver resolver = getContentResolver();
        resolver.registerContentObserver(LauncherSettings.Favorites.CONTENT_URI, true,
                mFavoritesObserver);
		
        //ActionsCode(authro:lizihao, comment:client hide config : enable display , disable hide)
        String mClientEnable = SystemProperties.get("ro.launcher.config.cling", DEFAULT_CLING_ENABLE);
        if(mClientEnable.equals(DEFAULT_CLING_ENABLE)){
            Utilities.mClingsEnable = true;
        } else if(mClientEnable.equals(DEFAULT_CLING_DISENABLE)){
            Utilities.mClingsEnable = false;
        }
        
        //ActionsCode(authro:lizihao, comment:swipe config, enable swipe , disable not swipe)
        String mSwipeEnable = SystemProperties.get("ro.launcher.swipe" , DEFAULT_SWIPE_DISENABLE);
	//	String mSwipeEnable = DEFAULT_SWIPE_ENABLE;
        if(mSwipeEnable.equals(DEFAULT_SWIPE_ENABLE)){
            Utilities.MIN_SNAP_VELOCITY = 6000;
        } else{
            Utilities.MIN_SNAP_VELOCITY = 1500;
        }
		
        //ActionsCode(authro:lizihao, comment:hide activity)
        String mHideActivity = SystemProperties.get("ro.launcher.hideactivity", DEFAULT_HIDEACTIVITY_DISENABLE);
        if(mHideActivity.equals(DEFAULT_HIDEACTIVITY_ENABLE)){
            Utilities.mHideActivityEnable = true;
            try {
                InputStream is = this.getResources().openRawResource(R.raw.hideactivity);
                ParseHidePackage mParseHidePackage = new ParseHidePackage();
                android.util.Xml.parse(is, Xml.Encoding.UTF_8, mParseHidePackage);
                Utilities.mActivityName = mParseHidePackage.getPackageList();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            } catch (SAXException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        } else if(mHideActivity.equals(DEFAULT_HIDEACTIVITY_DISENABLE)){
            Utilities.mHideActivityEnable = false;
        }
		
		String mXYConfig = SystemProperties.get("ro.launcher.xyconfig", "disable");
		if(mXYConfig.equals("enable")) {
		    //ActionsCode(authro:lizihao, comment: launcher init allapp ,hotseat and workspace config)
			Utilities.mAllAppsLandX = SystemProperties.getInt("ro.launcher.allapp.landX", DEFAULT_XY);
			Utilities.mAllAppsLandY = SystemProperties.getInt("ro.launcher.allapp.landY", DEFAULT_XY);
			Utilities.mAllAppsPortX = SystemProperties.getInt("ro.launcher.allapp.portX", DEFAULT_XY);
			Utilities.mAllAppsPortY = SystemProperties.getInt("ro.launcher.allapp.portY", DEFAULT_XY);
			
			if((Utilities.mAllAppsLandX == DEFAULT_XY) && (Utilities.mAllAppsLandY == DEFAULT_XY)
				&& (Utilities.mAllAppsPortX == DEFAULT_XY) && (Utilities.mAllAppsPortY == DEFAULT_XY)){
				Utilities.mCellConfig = false;
			} else{
				Utilities.mCellConfig = true;
			}
			
			Utilities.mWorkspaceLandX = SystemProperties.getInt("ro.launcher.workspace.landX", DEFAULT_XY);
			Utilities.mWorkspaceLandY = SystemProperties.getInt("ro.launcher.workspace.landY", DEFAULT_XY);
			Utilities.mWorkspacePortX = SystemProperties.getInt("ro.launcher.workspace.portX", DEFAULT_XY);
			Utilities.mWorkspacePortY = SystemProperties.getInt("ro.launcher.workspace.portY", DEFAULT_XY);
			
			if((Utilities.mWorkspaceLandX == DEFAULT_XY) && (Utilities.mWorkspaceLandY == DEFAULT_XY)
					&& (Utilities.mWorkspacePortX == DEFAULT_XY) && (Utilities.mWorkspacePortY == DEFAULT_XY)){
				Utilities.mWorkspaceConfig = false;
			} else{
				Utilities.mWorkspaceConfig = true;
			}
			
			Utilities.mHotseatCellCount = SystemProperties.getInt("ro.launcher.hotseatcellcount", DEFAULT_XY);
			Utilities.mHotseatAllappsIndex = SystemProperties.getInt("ro.launcher.hotseatallappsindex", DEFAULT_XY);
			Utilities.mHotseatLandY = SystemProperties.getInt("ro.launcher.hotseat.landY", DEFAULT_XY);
			Utilities.mHotseatPortY = SystemProperties.getInt("ro.launcher.hotseat.portY", DEFAULT_XY);
			
			if((Utilities.mHotseatCellCount == 0) && (Utilities.mHotseatAllappsIndex == 0) && (Utilities.mHotseatLandY == 0) && (Utilities.mHotseatPortY == 0)) {
				Utilities.mHotseatConfig = false;
			} else {
				Utilities.mHotseatConfig = true;
				Utilities.mWorkspaceLandY = Utilities.mWorkspaceLandY - Utilities.mHotseatLandY;
				Utilities.mWorkspacePortY = Utilities.mWorkspacePortY - Utilities.mHotseatPortY;
			}
		}

        Utilities.mUserTransitionAnimationScale = SystemProperties.getInt("ro.wm.transitionscale",50) / 100.0f;
    }

    public void recreateWidgetPreviewDb() {
        mWidgetPreviewCacheDb = new WidgetPreviewLoader.CacheDb(this);
    }

    /**
     * There's no guarantee that this function is ever called.
     */
    @Override
    public void onTerminate() {
        super.onTerminate();

        unregisterReceiver(mModel);

        ContentResolver resolver = getContentResolver();
        resolver.unregisterContentObserver(mFavoritesObserver);
    }

    /**
     * Receives notifications whenever the user favorites have changed.
     */
    private final ContentObserver mFavoritesObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            // If the database has ever changed, then we really need to force a reload of the
            // workspace on the next load
            mModel.resetLoadedState(false, true);
            mModel.startLoaderFromBackground();
        }
    };

    LauncherModel setLauncher(Launcher launcher) {
        mModel.initialize(launcher);
        //ActionsCode(phchen, NEW FEATURE: hold still wallpaper, wallpaper in launcher ),
        mLauncher = null;
        mLauncher = launcher;
        return mModel;
    }

    IconCache getIconCache() {
        return mIconCache;
    }

    LauncherModel getModel() {
        return mModel;
    }

    WidgetPreviewLoader.CacheDb getWidgetPreviewCacheDb() {
        return mWidgetPreviewCacheDb;
    }

    void setLauncherProvider(LauncherProvider provider) {
        mLauncherProvider = new WeakReference<LauncherProvider>(provider);
    }

    LauncherProvider getLauncherProvider() {
        return mLauncherProvider.get();
    }

    public static String getSharedPreferencesKey() {
        return sSharedPreferencesKey;
    }

    public static boolean isScreenLarge() {
    	//ActionsCode(phchen, NEW FEATURE: hold still wallpaper )
        return sIsScreenLarge && (mStillWallpaper == false);
    }
    
    /**
    *
    *xxxxx
    *xxxxx
    *
    ************************************
    *      
    * ActionsCode(phchen, new_method)
    * ActionsCode(phchen, NEW FEATURE: still wallpaper ),
    */
    public static boolean isStillWallpaper() {
        return mStillWallpaper;
    }

    public static boolean isScreenLandscape(Context context) {
        return context.getResources().getConfiguration().orientation ==
            Configuration.ORIENTATION_LANDSCAPE;
    }

    public static float getScreenDensity() {
        return sScreenDensity;
    }

    public static int getLongPressTimeout() {
        return sLongPressTimeout;
    }
}
