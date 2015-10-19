/* //device/content/providers/media/src/com/android/providers/media/MediaScannerReceiver.java
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

package com.android.providers.media;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.IOException;

public class MediaScannerReceiver extends BroadcastReceiver {
    private final static String TAG = "MediaScannerReceiver";
	//ActionsCode(author:lishiyuan, add debug info)
    private final static boolean LOCAL_LOGD = true;
    private Uri FILES_URI = Uri.parse("content://media/external/file/");

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        final Uri uri = intent.getData();
        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            // Scan both internal and external storage
            scan(context, MediaProvider.INTERNAL_VOLUME);
            /**
             *actions_code(lishiyuan, NEW_FEATURE:when system reboot,scan /mnt/media directory)
             */
            scanDirectory(context,"/mnt/media/");
            scan(context, MediaProvider.EXTERNAL_VOLUME);
            /*
             *actions_code(lishiyuan, NEW_FEATURE:when system receives ACTION_BOOT_COMPLETED , 
			 *            delete "_data=\"\""  of item from external.db
             */
			//add by wgh
            try {
                int cnt = context.getContentResolver().delete(FILES_URI, "_data=\"\"", null);
                if(LOCAL_LOGD)
                    Log.d(TAG, "____delete records: " + cnt);
            } catch (Exception e) {
                //nothing
            }
            //end by wgh
        } else {
            if (uri.getScheme().equals("file")) {
                // handle intents related to external storage
                String path = uri.getPath();
                String externalStoragePath = Environment.getExternalStorageDirectory().getPath();
                //actions_code(lishiyuan, NEW_FEATURE:fix BUG00152754 crf,scan file for tfcard and uhost)
                String tfcardStoragePath = Environment.getTfcardStorageDirectory().getPath();
                String uhostStoragePath = Environment.getUhostStorageDirectory().getPath();
                String legacyPath = Environment.getLegacyExternalStorageDirectory().getPath();

                try {
                    path = new File(path).getCanonicalPath();
                } catch (IOException e) {
                    Log.e(TAG, "couldn't canonicalize " + path);
                    return;
                }
                if (path.startsWith(legacyPath)) {
                    path = externalStoragePath + path.substring(legacyPath.length());
                }

                Log.d(TAG, "action: " + action + " path: " + path);
                if (Intent.ACTION_MEDIA_MOUNTED.equals(action)) {
                    // scan whenever any volume is mounted
                    scan(context, MediaProvider.EXTERNAL_VOLUME);
                } else if (Intent.ACTION_MEDIA_SCANNER_SCAN_FILE.equals(action) &&
                        path != null && (path.startsWith(externalStoragePath + "/")
                            || path.startsWith(tfcardStoragePath + "/")
                            || path.startsWith(uhostStoragePath + "/"))) {
                    scanFile(context, path);
                } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_SCAN_DIRECTORY)) {
                   /**
                    * actions_code(lishiyuan,new_feature: support to scan directory while android
                    *              donot suppot originally)
	                */
                    if(LOCAL_LOGD)
		                Log.d(TAG,"start directory "+path);
					if(path != null){
					        scanDirectory(context, path);
					}
                }
            }
        }
    }

    private void scan(Context context, String volume) {
        Bundle args = new Bundle();
        args.putString("volume", volume);
        context.startService(
                new Intent(context, MediaScannerService.class).putExtras(args));
    }    

    private void scanFile(Context context, String path) {
        Bundle args = new Bundle();
        args.putString("filepath", path);
        context.startService(
                new Intent(context, MediaScannerService.class).putExtras(args));
    }    
	/**
	 * actions_code(lishiyuan,new_feature: support to scan directory while android
	 *              donot suppot originally) 
	 *scan directory
	 * add by 3307 2012.7.26
	**/
	private void scanDirectory(Context context, String path) {
        Bundle args = new Bundle();
        args.putString("directorypath", path);
        context.startService(
                new Intent(context, MediaScannerService.class).putExtras(args));
    }
}
