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

package com.android.omadm.plugin.diagmon;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import com.android.omadm.plugin.IDmtPlugin;

public class DiagmonService extends Service {
    private static final String TAG = "DiagmonService";

    private IDmtPlugin.Stub mBinder;

    @Override
    public IBinder onBind(Intent intent) {
        
    	if (DiagmonPlugin.class.getName().equals(intent.getAction())) 
        {
            String rootPath = intent.getStringExtra("rootPath");
            Log.d(TAG, "rootPath =" + rootPath);
    		if (mBinder == null)
            {
            	Log.d(TAG, "mBinder is null, create it!");
            	mBinder = new DiagmonPlugin(this);            	
            }else
            {
            	Log.d(TAG, "mBinder already exists!");           	
            }
    		return mBinder;
        }
    	Log.d(TAG, "Invalid action name, return null!");
        return null;
    }

    @Override
    public boolean onUnbind(Intent intent)
    {
    	super.onUnbind(intent);
    	Log.d(TAG, "Enter onUnbind");
    	mBinder = null;
    	return true;
    }
}
