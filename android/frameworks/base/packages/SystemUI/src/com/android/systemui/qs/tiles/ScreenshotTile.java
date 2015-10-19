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

package com.android.systemui.qs.tiles;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.provider.Settings.Global;
import android.util.Log;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Parcel;
import android.os.IRemoteCallback;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;
import android.os.UserHandle;

import com.android.systemui.R;
import com.android.systemui.qs.QSTile;
import com.android.systemui.statusbar.policy.ScreenRecordController;
import com.android.systemui.statusbar.policy.ScreenRecordController.ScreenRecordControllerCallback;


/** Quick settings tile: Airplane mode **/
public class ScreenshotTile extends QSTile<QSTile.BooleanState> 
{
		private Host mhost;
		final Object mScreenshotLock = new Object();
		ServiceConnection mScreenshotConnection = null;
		Handler mHandler;

    public ScreenshotTile(Host host) {
    super(host);
		mhost = host;
		mHandler = new Handler();
    }

    @Override
    protected BooleanState newTileState() {
        return new BooleanState();
    }

		@Override
    public void setListening(boolean listening) {
    }

    @Override
    public void handleClick() {
    		mhost.collapsePanels();
    		mHandler.postDelayed(mScreenshotRunnable,1000);
    }
    
    private final Runnable mScreenshotRunnable = new Runnable() {
        @Override
        public void run() {
            takeScreenshot();
        }
    };

    final Runnable mScreenshotTimeout = new Runnable() {
        @Override public void run() {
            synchronized (mScreenshotLock) {
                if (mScreenshotConnection != null) {
                    mContext.unbindService(mScreenshotConnection);
                    mScreenshotConnection = null;
                }
            }
        }
    };

   // Assume this is called from the Handler thread.
    private void takeScreenshot() {
        synchronized (mScreenshotLock) {
            if (mScreenshotConnection != null) {
                return;
            }
            ComponentName cn = new ComponentName("com.android.systemui",
                    "com.android.systemui.screenshot.TakeScreenshotService");
            Intent intent = new Intent();
            intent.setComponent(cn);
            ServiceConnection conn = new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                    synchronized (mScreenshotLock) {
                        if (mScreenshotConnection != this) {
                            return;
                        }
                        Messenger messenger = new Messenger(service);
                        Message msg = Message.obtain(null, 1);
                        final ServiceConnection myConn = this;
                        Handler h = new Handler(mHandler.getLooper()) {
                            @Override
                            public void handleMessage(Message msg) {
                                synchronized (mScreenshotLock) {
                                    if (mScreenshotConnection == myConn) {
                                        mContext.unbindService(mScreenshotConnection);
                                        mScreenshotConnection = null;
                                        mHandler.removeCallbacks(mScreenshotTimeout);
                                    }
                                }
                            }
                        };
                        msg.replyTo = new Messenger(h);
                        msg.arg1 = msg.arg2 = 1;
                        try {
                            messenger.send(msg);
                        } catch (RemoteException e) {
                        }
                    }
                }
                @Override
                public void onServiceDisconnected(ComponentName name) {}
            };
            if (mContext.bindServiceAsUser(
                    intent, conn, Context.BIND_AUTO_CREATE, UserHandle.CURRENT)) {
                mScreenshotConnection = conn;
                mHandler.postDelayed(mScreenshotTimeout, 10000);
            }
        }
    }

    @Override
    protected void handleUpdateState(BooleanState state, Object arg) {		
        state.visible = true;
				state.icon = ResourceIcon.get(R.drawable.ic_qs_screen_shot);
				state.label = mContext.getString(R.string.quick_settings_screen_shot);
        state.contentDescription =  mContext.getString(R.string.quick_settings_screen_shot);
    }

    @Override
    protected String composeChangeAnnouncement() {
        if (mState.value) {
            return mContext.getString(R.string.quick_settings_screen_shot);
        } else {
            return mContext.getString(R.string.quick_settings_screen_shot);
        }
    }
}
