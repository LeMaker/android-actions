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

import com.android.systemui.R;
import com.android.systemui.qs.QSTile;
import com.android.systemui.statusbar.policy.ScreenRecordController;
import com.android.systemui.statusbar.policy.ScreenRecordController.ScreenRecordControllerCallback;


/** Quick settings tile: Airplane mode **/
public class ScreenrecordTile extends QSTile<QSTile.BooleanState> implements 
			ScreenRecordControllerCallback {

		private final ScreenRecordController mScreenRecordController;
		private Host mhost;

    public ScreenrecordTile(Host host) {
    super(host);
		mScreenRecordController = host.getScreenRecordController();
		mScreenRecordController.addScreenRecordControllerCallback(this);
		mhost = host;
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
    		mhost.collapsePanels();;
        setEnabled(!mState.value);
    }

    private void setEnabled(boolean enabled) {
    		final boolean recording = mScreenRecordController.isScreenRecording();
    		
    		if(recording && !enabled){
        		mScreenRecordController.stopScreenRecord();
        }else if(!recording && enabled){
            mScreenRecordController.startScreenRecord();
        }

				refreshState(mScreenRecordController.isScreenRecording());	
    }

    @Override
    protected void handleUpdateState(BooleanState state, Object arg) {
        if (arg instanceof Boolean) {
            state.value = (Boolean) arg;
        }
		
        state.visible = true;
        if (!state.value) {      /*false,close state*/
            state.icon =  ResourceIcon.get(R.drawable.ic_qs_screen_record_start);
			state.label = mContext.getString(
					R.string.quick_settings_start_screen_record_label);
            state.contentDescription =  mContext.getString(
                    R.string.quick_settings_start_screen_record_label);
        } else {
            state.icon = ResourceIcon.get(R.drawable.ic_qs_screen_record_stop);
			state.label = mContext.getString(
					R.string.quick_settings_stop_screen_record_label);
            state.contentDescription =  mContext.getString(
                    R.string.quick_settings_stop_screen_record_label);
        }
    }

    @Override
    protected String composeChangeAnnouncement() {
        if (mState.value) {
            return mContext.getString(R.string.quick_settings_start_screen_record_label);
        } else {
            return mContext.getString(R.string.quick_settings_stop_screen_record_label);
        }
    }

	@Override
    public void onScreenRecordStateChanged(boolean recording, boolean affordanceVisible) {
        if(recording)
			refreshState(true);
		else
			refreshState(false);
    }
}
