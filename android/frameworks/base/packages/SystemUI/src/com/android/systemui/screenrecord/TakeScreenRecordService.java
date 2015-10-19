/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.systemui.screenrecord;

import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.os.IBinder;
import android.os.Binder;
import android.os.Message;
import android.os.Messenger;
import android.util.Slog;
import android.os.RemoteException;
import android.util.Log;
public class TakeScreenRecordService extends Service {
    private static final String TAG = "TakeScreenRecordService";

    private static GlobalScreenRecord mScreenRecorder;

	@Override
	public void onCreate() {
		super.onCreate();
		Log.d(TAG, "onCreate()");
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		Log.d(TAG, "onDestroy()");
	}
	
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case 1:
                {
                	
                    final Messenger callback = msg.replyTo;
                    if (mScreenRecorder == null) {
                        mScreenRecorder = new GlobalScreenRecord(TakeScreenRecordService.this);
                    }
                    mScreenRecorder.startScreenRecord(new Runnable() {
                        @Override public void run() {                        	
                            Message reply = Message.obtain(null, 1);
                            try {
                                callback.send(reply);
                            } catch (RemoteException e) {
                            }
                            Log.d(TAG, "startScreenRecord  finished ");
                        }
                    }, msg.arg1 > 0, msg.arg2 > 0);
                    
                    break;
                 }
                 case 2:
                 {
                   	final Messenger callback = msg.replyTo;
                    if (mScreenRecorder == null) {
                        mScreenRecorder = new GlobalScreenRecord(TakeScreenRecordService.this);
                    }
                    mScreenRecorder.stopScreenRecord(new Runnable() {
                        @Override public void run() {
                            Message reply = Message.obtain(null, 2);
                            try {
                                callback.send(reply);
                            } catch (RemoteException e) {
                            }
                            Log.d(TAG, "stopScreenRecord  finished ");
                            onDestroy();
                        }
                        
                    }, msg.arg1 > 0, msg.arg2 > 0);
                    
                    break;
                 }                
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        Log.d(TAG, "onBind()");
        return new Messenger(mHandler).getBinder();
    }
}
