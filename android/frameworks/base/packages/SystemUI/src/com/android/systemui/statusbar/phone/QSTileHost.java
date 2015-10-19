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

package com.android.systemui.statusbar.phone;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.net.Uri;
import android.os.Process;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.provider.Settings.Secure;
import android.provider.Settings.System;

import android.util.Log;

import com.android.systemui.R;
import com.android.systemui.qs.QSTile;
import com.android.systemui.qs.tiles.AirplaneModeTile;
import com.android.systemui.qs.tiles.BluetoothTile;
import com.android.systemui.qs.tiles.CastTile;
import com.android.systemui.qs.tiles.CellularTile;
import com.android.systemui.qs.tiles.ColorInversionTile;
import com.android.systemui.qs.tiles.FlashlightTile;
import com.android.systemui.qs.tiles.HotspotTile;
import com.android.systemui.qs.tiles.IntentTile;
import com.android.systemui.qs.tiles.LocationTile;
import com.android.systemui.qs.tiles.RotationLockTile;
import com.android.systemui.qs.tiles.WifiTile;
import com.android.systemui.settings.CurrentUserTracker;
import com.android.systemui.statusbar.policy.BluetoothController;
import com.android.systemui.statusbar.policy.CastController;
import com.android.systemui.statusbar.policy.FlashlightController;
import com.android.systemui.statusbar.policy.KeyguardMonitor;
import com.android.systemui.statusbar.policy.LocationController;
import com.android.systemui.statusbar.policy.NetworkController;
import com.android.systemui.statusbar.policy.RotationLockController;
import com.android.systemui.statusbar.policy.HotspotController;
import com.android.systemui.statusbar.policy.SecurityController;
import com.android.systemui.statusbar.policy.UserSwitcherController;
import com.android.systemui.statusbar.policy.ZenModeController;
/**
  *NEW_FEATURE: 
  *Add screenrecord&screenshot in SystemUI
  *new file:ScreenrecordTile and ScreenRecordController
  ************************************
  *      
  *ActionsCode(author:fanguoyong, change_code)
  */
import com.android.systemui.qs.tiles.ScreenrecordTile;
import com.android.systemui.qs.tiles.ScreenshotTile;
import com.android.systemui.statusbar.policy.ScreenRecordController;
import android.os.SystemProperties;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import java.io.File;
import java.io.BufferedReader;
import java.io.Reader;
import java.io.FileReader;
import java.io.IOException;



/** Platform implementation of the quick settings tile host **/
public class QSTileHost implements QSTile.Host {
    private static final String TAG = "QSTileHost";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    private static final String TILES_SETTING = "sysui_qs_tiles";

    private final Context mContext;
    private final PhoneStatusBar mStatusBar;
    private final LinkedHashMap<String, QSTile<?>> mTiles = new LinkedHashMap<>();
    private final Observer mObserver = new Observer();
    private final BluetoothController mBluetooth;
    private final LocationController mLocation;
    private final RotationLockController mRotation;
    private final NetworkController mNetwork;
    private final ZenModeController mZen;
    private final HotspotController mHotspot;
    private final CastController mCast;
    private final Looper mLooper;
    private final CurrentUserTracker mUserTracker;
    private final FlashlightController mFlashlight;
    private final UserSwitcherController mUserSwitcherController;
    private final KeyguardMonitor mKeyguard;
    private final SecurityController mSecurity;
	/**
	  *NEW_FEATURE: 
	  *Add screenrecord&screenshot in SystemUI
	  ************************************
	  *      
	  *ActionsCode(author:fanguoyong, change_code)
	  */
	private final ScreenRecordController mscreenrecord;

    private Callback mCallback;

	/**
	  *NEW_FEATURE: 
	  *Add screenrecord&screenshot in SystemUI
	  ************************************
	  *      
	  *ActionsCode(author:fanguoyong, change_code)
	  */
    public QSTileHost(Context context, PhoneStatusBar statusBar,
            BluetoothController bluetooth, LocationController location,
            RotationLockController rotation, NetworkController network,
            ZenModeController zen, HotspotController hotspot,
            CastController cast, FlashlightController flashlight,
            UserSwitcherController userSwitcher, KeyguardMonitor keyguard,
            SecurityController security,
            /*ActionsCode(author:fanguoyong, change_code)*/ScreenRecordController screenrecord) {
        mContext = context;
        mStatusBar = statusBar;
        mBluetooth = bluetooth;
        mLocation = location;
        mRotation = rotation;
        mNetwork = network;
        mZen = zen;
        mHotspot = hotspot;
        mCast = cast;
        mFlashlight = flashlight;
        mUserSwitcherController = userSwitcher;
        mKeyguard = keyguard;
        mSecurity = security;
        //ActionsCode(author:fanguoyong, change_code)
        mscreenrecord = screenrecord;

        final HandlerThread ht = new HandlerThread(QSTileHost.class.getSimpleName(),
                Process.THREAD_PRIORITY_BACKGROUND);
        ht.start();
        mLooper = ht.getLooper();

        mUserTracker = new CurrentUserTracker(mContext) {
            @Override
            public void onUserSwitched(int newUserId) {
                recreateTiles();
                for (QSTile<?> tile : mTiles.values()) {
                    tile.userSwitch(newUserId);
                }
                mSecurity.onUserSwitched(newUserId);
                mNetwork.onUserSwitched(newUserId);
                mObserver.register();
            }
        };
        recreateTiles();

        mUserTracker.startTracking();
        mObserver.register();
    }

    @Override
    public void setCallback(Callback callback) {
        mCallback = callback;
    }

    @Override
    public Collection<QSTile<?>> getTiles() {
        return mTiles.values();
    }

    @Override
    public void startSettingsActivity(final Intent intent) {
        mStatusBar.postStartSettingsActivity(intent, 0);
    }

    @Override
    public void warn(String message, Throwable t) {
        // already logged
    }

    @Override
    public void collapsePanels() {
        mStatusBar.postAnimateCollapsePanels();
    }

    @Override
    public Looper getLooper() {
        return mLooper;
    }

    @Override
    public Context getContext() {
        return mContext;
    }

    @Override
    public BluetoothController getBluetoothController() {
        return mBluetooth;
    }

    @Override
    public LocationController getLocationController() {
        return mLocation;
    }

    @Override
    public RotationLockController getRotationLockController() {
        return mRotation;
    }

    @Override
    public NetworkController getNetworkController() {
        return mNetwork;
    }

    @Override
    public ZenModeController getZenModeController() {
        return mZen;
    }

    @Override
    public HotspotController getHotspotController() {
        return mHotspot;
    }

    @Override
    public CastController getCastController() {
        return mCast;
    }

    @Override
    public FlashlightController getFlashlightController() {
        return mFlashlight;
    }

    @Override
    public KeyguardMonitor getKeyguardMonitor() {
        return mKeyguard;
    }

    public UserSwitcherController getUserSwitcherController() {
        return mUserSwitcherController;
    }

    public SecurityController getSecurityController() {
        return mSecurity;
    }
    
    /**
		 *NEW_FEATURE: 
		 *Add screenrecord in SystemUI
		 *add method getScreenRecordController()
		 ************************************
		 *      
		 *ActionsCode(author:fanguoyong, new_method))
		 */
		public ScreenRecordController getScreenRecordController() {
        return mscreenrecord;
    }

    private void recreateTiles() {
        if (DEBUG) Log.d(TAG, "Recreating tiles");
        final List<String> tileSpecs = loadTileSpecs();
        for (Map.Entry<String, QSTile<?>> tile : mTiles.entrySet()) {
            if (!tileSpecs.contains(tile.getKey())) {
                if (DEBUG) Log.d(TAG, "Destroying tile: " + tile.getKey());
                tile.getValue().destroy();
            }
        }
        final LinkedHashMap<String, QSTile<?>> newTiles = new LinkedHashMap<>();
		String use_usb_wifi_bt_dongle = SystemProperties.get("ro.use.usb.wifi_bt.dongle");
		
        for (String tileSpec : tileSpecs) {

			//change the tile according to whether insert usb wifi&bt dongle. Add by ian.jiang
			if( (use_usb_wifi_bt_dongle != null)&&(!use_usb_wifi_bt_dongle.isEmpty())&&(use_usb_wifi_bt_dongle.equals("true")) )
			{
				if( (tileSpec.equals("wifi"))&&(!IsInsertUsbWifiDongle()) )
				{
					Log.d(TAG,"<===not add wifi tile");
					continue;
				}

				if( (tileSpec.equals("bt"))&&(!IsInsertUsbBTDongle()) )
				{
					Log.d(TAG,"<===not add bt tile");
					continue;
				}
			}
			
            if (mTiles.containsKey(tileSpec)) {
                newTiles.put(tileSpec, mTiles.get(tileSpec));
            } else {
                if (DEBUG) Log.d(TAG, "Creating tile: " + tileSpec);
                try {
                    newTiles.put(tileSpec, createTile(tileSpec));
                } catch (Throwable t) {
                    Log.w(TAG, "Error creating tile for spec: " + tileSpec, t);
                }
            }
        }
        if (mTiles.equals(newTiles)) return;
        mTiles.clear();
        mTiles.putAll(newTiles);
        if (mCallback != null) {
            mCallback.onTilesChanged();
        }
    }

    /**
		 *NEW_FEATURE: 
		 *Add screenrecord&screenshot in SystemUI
		 ************************************
		 *      
		 *ActionsCode(author:fanguoyong, change_code)
		 */
    private QSTile<?> createTile(String tileSpec) {
        if (tileSpec.equals("wifi")) return new WifiTile(this);
        else if (tileSpec.equals("bt")) return new BluetoothTile(this);
        else if (tileSpec.equals("inversion")) return new ColorInversionTile(this);
        else if (tileSpec.equals("cell")) return new CellularTile(this);
        else if (tileSpec.equals("airplane")) return new AirplaneModeTile(this);
        else if (tileSpec.equals("rotation")) return new RotationLockTile(this);
        else if (tileSpec.equals("flashlight")) return new FlashlightTile(this);
        else if (tileSpec.equals("location")) return new LocationTile(this);
        else if (tileSpec.equals("cast")) return new CastTile(this);
        else if (tileSpec.equals("hotspot")) return new HotspotTile(this);
        else if (tileSpec.startsWith(IntentTile.PREFIX)) return IntentTile.create(this,tileSpec);
        //ActionsCode(author:fanguoyong, change_code)
        else if (tileSpec.equals("capture")) return new ScreenshotTile(this);
        else if (tileSpec.equals("screenrecord")) return new ScreenrecordTile(this);
        else throw new IllegalArgumentException("Bad tile spec: " + tileSpec);
    }

    /**
		 *NEW_FEATURE: 
		 *Add screenrecord&screenshot in SystemUI
		 *read property "ro.systemui.screenrecord" & "ro.systemui.screenshot" in android.prop file
		 ************************************
		 *      
		 *ActionsCode(author:fanguoyong, change_code)
		 */
    private List<String> loadTileSpecs() {
        final Resources res = mContext.getResources();
        final String defaultTileList = res.getString(R.string.quick_settings_tiles_default);
        String tileList = Secure.getStringForUser(mContext.getContentResolver(), TILES_SETTING,
                mUserTracker.getCurrentUserId());
        if (tileList == null) {
            tileList = res.getString(R.string.quick_settings_tiles);
            if (DEBUG) Log.d(TAG, "Loaded tile specs from config: " + tileList);
        } else {
            if (DEBUG) Log.d(TAG, "Loaded tile specs from setting: " + tileList);
        }
        final ArrayList<String> tiles = new ArrayList<String>();
        boolean addedDefault = false;
        for (String tile : tileList.split(",")) {
            tile = tile.trim();
            if (tile.isEmpty()) continue;
            if (tile.equals("default")) {
                if (!addedDefault) {
                    tiles.addAll(Arrays.asList(defaultTileList.split(",")));
                    addedDefault = true;
                }
            } else {
                tiles.add(tile);
            }
        }        
        //ActionsCode(author:fanguoyong, change_code)
        boolean screenrecordflag = SystemProperties.get("ro.systemui.screenrecord", "enable").equals("enable");
        if (screenrecordflag) {
			tiles.add("screenrecord");
		}
		boolean screenshotflag = SystemProperties.get("ro.systemui.capture", "enable").equals("enable");
        if (screenshotflag) {
			tiles.add("capture");
		}
        
        return tiles;
    }

    private class Observer extends ContentObserver {
        private boolean mRegistered;

        public Observer() {
            super(new Handler(Looper.getMainLooper()));
        }

        public void register() {
            if (mRegistered) {
                mContext.getContentResolver().unregisterContentObserver(this);
            }
            mContext.getContentResolver().registerContentObserver(Secure.getUriFor(TILES_SETTING),
                    false, this, mUserTracker.getCurrentUserId());
            mRegistered = true;
        }

        @Override
        public void onChange(boolean selfChange, Uri uri) {
            recreateTiles();
        }
    }

    /**
		 *NEW_FEATURE: 
		 *damially change wifi&bt in Panel Bar
		 ************************************
		 *      
		 *ActionsCode(author:ian.jiang, change_code)
		 */
	public void UpdateTiles(){
		recreateTiles();
	}
	
	public boolean IsInsertUsbWifiDongle() {
		File file = new File("/sys/kernel/debug/usb/devices");
		BufferedReader reader = null;
		boolean ret = false;
		try {

			reader = new BufferedReader(new FileReader(file));
			String tempString = null;
			int line = 1;

			while ((tempString = reader.readLine()) != null) {
				
				//Log.e("ian.jiang","line " + line + ": " + tempString);	
				if(tempString.contains("Product=802.11")) {
					ret = true;
					break;
				}

				line++;
			}
			reader.close();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if (reader != null) {
				try {
					reader.close();
				} catch (IOException e1) {
				}
			}
		}

		if(ret)
			return true;
		else
			return false;

	}

	public boolean IsInsertUsbBTDongle() {
		File file = new File("/sys/kernel/debug/usb/devices");
		BufferedReader reader = null;
		boolean ret = false;
		try {

			reader = new BufferedReader(new FileReader(file));
			String tempString = null;
			int line = 1;

			while ((tempString = reader.readLine()) != null) {
				
				if(tempString.contains("Product=802.11n WLAN Adapte")) {
					ret = true;
					break;
				}

				line++;
			}
			reader.close();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if (reader != null) {
				try {
					reader.close();
				} catch (IOException e1) {
				}
			}
		}

		if(ret)
			return true;
		else
			return false;

	}


	
}
