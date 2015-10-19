/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.systemui.statusbar.policy;

import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.atomic.AtomicInteger;
import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.io.FileInputStream;
import java.io.FilenameFilter;
import java.util.regex.Pattern;
import android.os.UserHandle;
import android.util.Log;
import android.os.IRemoteCallback;
import android.os.Environment;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.IBinder;
import android.os.UserHandle;
import android.os.Handler;              
import android.os.Message;              
import android.os.RemoteException;      
import android.os.storage.StorageVolume;
import android.os.StatFs;
import android.widget.Toast;
import android.content.ServiceConnection;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import com.android.systemui.R;
import android.app.ActivityManagerNative;

public final class ScreenRecordController {
	private static final String TAG = "ScreenRecordController";
	//ActionsCode(author:fanguoyong,change_code):change /mnt/sdcard -> /storage/sdcard
	private static final String MEDIA_SDCARD = "/storage/sdcard";
	private static final String SCREENRECORD_DIR_NAME = "ScreenRecords";
	/*
	 *bugfix:BUG00260441
	 *reduce MAX_SCREEN_RECORD_SIZE to stop screenrecord before MPEG4writer stop.
	 *ActionsCode(author:fanguoyong, change_code)
	 */
	private static final int MAX_SCREEN_RECORD_SIZE = 2000000000;//1.86*1024*1024*1024;
	
    private final Context mContext;
	
	private static File mScreenRecordDir = null;
    
    final Object mScreenRecordLock = new Object();    
  
    ServiceConnection mScreenRecordConnection = null;
    
    private boolean mIsRecording = false;
    
    private boolean mExternalStorageReady = true;
	//ActionsCode(fanguoyong,BUGFIX:BUG00280171) record the screenrecord userhandle
	private UserHandle mUserHandler = null;
    
    private Handler mHandler = new Handler();
 
    BroadcastReceiver mShutdownReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (Intent.ACTION_SHUTDOWN.equals(intent.getAction())) {
        	    if (mIsRecording) {
	                Log.d(TAG, "received shutdown msg, stop record");
	                stopScreenRecord();
              	}
            }
         }
    }; 
        
    BroadcastReceiver mMountReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
               String action = intent.getAction();
               StorageVolume mStorageVolume = intent.getParcelableExtra(StorageVolume.EXTRA_STORAGE_VOLUME);
               String extStoragePath = mStorageVolume == null ?  Environment.getLegacyExternalStorageDirectory().toString(): mStorageVolume.getPath();
               if(MEDIA_SDCARD.equals(extStoragePath)){
	                if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
	                	if(mIsRecording){
	                		Log.d(TAG, "stop screen record because ACTION_MEDIA_EJECT" + extStoragePath + "not mExternalStorageReady false");
	 						stopScreenRecord();
	 					}	 					
	 					mExternalStorageReady = false; 					
	                }else if(action.equals(Intent.ACTION_MEDIA_MOUNTED)){
	                	mExternalStorageReady = true;
	                }
            	}
            }
    };
    
    private final Runnable mScreenRecordStartRunnable = new Runnable() {
        @Override
        public void run() {
        	//ActionsCode(fanguoyong,BUGFIX:BUG00280171) record the screenrecord userhandle
        	try {
            	mUserHandler = ActivityManagerNative.getDefault().getCurrentUser().getUserHandle();
			} catch (RemoteException e) {
                Log.e(TAG, "Couldn't get current user id ", e);
            }
            takeScreenRecord(1);
        }
    };
    
    private final Runnable mScreenRecordStopRunnable = new Runnable() {
        @Override
        public void run() {
            takeScreenRecord(2);
        }
    };   
   
	// Assume this is called from the Handler thread.
    private void takeScreenRecord(final int start) {
        synchronized (mScreenRecordLock) { 
        	if(mScreenRecordConnection != null) {
                return;
            }       	
            ComponentName cn = new ComponentName("com.android.systemui","com.android.systemui.screenrecord.TakeScreenRecordService");
            Intent intent = new Intent();
            intent.setComponent(cn);
            ServiceConnection conn = new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder service) {
                    synchronized (mScreenRecordLock) {                    
                        Message msg = Message.obtain(null, start);
                        final ServiceConnection myConn = this;
                        Handler h = new Handler(mHandler.getLooper()) {
                            @Override
                            public void handleMessage(Message msg) {
                                synchronized (mScreenRecordLock) { 
                                        if (mScreenRecordConnection != null) {
                                            mContext.unbindService(mScreenRecordConnection);
                                            mScreenRecordConnection = null;
                                        }
                                }
                            }
                        };
                        msg.replyTo = new Messenger(h);
                        msg.arg1 = msg.arg2 = 1;
                        Messenger messenger = new Messenger(service);
                        try {
                            messenger.send(msg);
                        } catch (RemoteException e) {
                        }   
                    }
                }
                @Override
                public void onServiceDisconnected(ComponentName name) {}
            };
			//ActionsCode(fanguoyong,BUGFIX:BUG00280171) start screenrecord service as mUserHandler
            if (mContext.bindServiceAsUser(
                    intent, conn, Context.BIND_AUTO_CREATE, mUserHandler)) {
                mScreenRecordConnection = conn;                
            }
        }
    }
    
        //add by Actions for ScreenRecord
    private GetSDAvailableSizeThread mGetSDAvailableSizeThread;
    
    private long getAvailableSDSize(){
        try {
            File path = Environment.getExternalStorageDirectory();
            StatFs stat = new StatFs(path.getPath()); 
            long blockSize = stat.getBlockSizeLong(); 
            long availableBlocks = stat.getAvailableBlocksLong();
            return availableBlocks * blockSize;
        } catch (Exception ignore) {
            return 0;
        }  
	}

	private void CheckScreenRecordVedioSize(){
		
        FileInputStream inputStream = null;
        File vedioFolder = new File(mScreenRecordDir.getPath());
        String VedioName;
        String tmpExpandName = ".tmp";
        File vedioFile;
		String RecordVideoFilePath; 

		if(!vedioFolder.exists()){
			Log.e(TAG, "new tmp mp4 path (" + mScreenRecordDir + ") not exist.");
			return;
		}

		File[] files = vedioFolder.listFiles();
        	
        for (File file : files) {
        	if((file != null) && (!file.isDirectory())){
        		VedioName = file.getName();
        
        		if(VedioName.endsWith(tmpExpandName)){
					RecordVideoFilePath = new File(mScreenRecordDir, VedioName).getAbsolutePath();
					Log.i(TAG, "new screen record vedio path is " + RecordVideoFilePath);
        			vedioFile = new File(RecordVideoFilePath);
                    try {
                        inputStream = new FileInputStream(vedioFile);
                    } catch (IOException e) {
                        Log.e(TAG, "open " + VedioName + " failed.");
                    } 						
        			break;
        		}
        	}
        }
        
        if(inputStream != null){
            try {
        
        		 Log.i(TAG, "new screen record vedio size is " + inputStream.available());
        		 if(inputStream.available() >= MAX_SCREEN_RECORD_SIZE){
        		 	inputStream.close();
        
        			stopScreenRecord();
					Log.i(TAG, "new mp4 size more than 2G, stop and exit screen record.");
        		 }
            } catch (IOException e) {
            
            } 
        }
		/*
		 *BUGFIX:BUG00253868 
		 *if there is no recording file,exit
		 *ActionsCode(author:fanguoyong, bug_fix)
		 */
		else {
     		stopScreenRecord();
			Log.e(TAG, "this is no mp4 file to save, stop and exit screen record.");
		}		
		//close inputStream
		if(inputStream != null)
			try {
				inputStream.close();	
			} catch (IOException e) {
            
            } 
	}
    
	private long MIN_SD_SIZE = 100 * 1024 * 1024;
    private Handler mGetSDAvailableSizehandle = new Handler() {
        public void handleMessage(Message msg) {

			CheckScreenRecordVedioSize();
			
            long size = getAvailableSDSize();

        	Log.d(TAG, "handle getSize Message :" + size);
        	if (size <= MIN_SD_SIZE)
        	{
                Log.d(TAG, "stopScreenRecord");
                Toast.makeText(mContext, R.string.screenrecord_no_available_space, Toast.LENGTH_SHORT).show();
				/*BUFFIX:BUG00273124,stop screenrecord when this is no Available SD Size*/
				stopScreenRecord();
        	}
        	super.handleMessage(msg);
        }
    };
	
    private class GetSDAvailableSizeThread extends Thread {
    	private AtomicInteger  mIsRunning = new AtomicInteger(0);
 
    	public void start() {
    		mIsRunning.set(1);
    		super.start();
    	}
    	
    	public void cancel() {
    		mIsRunning.set(0);
    	}

		@Override
		public void run() {
			while(mIsRunning.get() != 0) {
				try {
	                Thread.sleep(10000); 
	                if (mIsRunning.get()== 0) {
	                	break;
	                }
					
	                Message message = new Message();
	                message.what = 1;
	                if (mGetSDAvailableSizehandle != null) {
	                	mGetSDAvailableSizehandle.sendMessage(message); 
	                }
	            } catch (InterruptedException e) {
	                e.printStackTrace();
	            }
			}
		}
    	
    }

    private final CopyOnWriteArrayList<ScreenRecordControllerCallback> mCallbacks =
            new CopyOnWriteArrayList<ScreenRecordControllerCallback>();


    public interface ScreenRecordControllerCallback {
        public void onScreenRecordStateChanged(boolean rotationLocked, boolean affordanceVisible);
    }
    
    
    public ScreenRecordController(Context context) {
        mContext = context;
        //add by (Actions)
        //register for shutdown event to process screenRecord
        IntentFilter filter = new IntentFilter(Intent.ACTION_SHUTDOWN);
        context.registerReceiver(mShutdownReceiver, filter);
        
            
	    IntentFilter intentFilter = new IntentFilter(Intent.ACTION_MEDIA_MOUNTED);
	    intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
	    intentFilter.addDataScheme("file");
	    context.registerReceiver(mMountReceiver, intentFilter);
       
    }
    
    public void addScreenRecordControllerCallback(ScreenRecordControllerCallback callback) {
        mCallbacks.add(callback);
    }


    private void notifyChanged() {
        for (ScreenRecordControllerCallback callback : mCallbacks) {
            callback.onScreenRecordStateChanged(mIsRecording,true);
        }
    }
    
    public boolean isScreenRecording() {
		return mIsRecording;
    }

    public boolean startScreenRecord() {
        if (mIsRecording || !mExternalStorageReady ) {
        	Log.d(TAG, "startScreenRecord needn't begin " + mIsRecording + "mIsRecording " + mExternalStorageReady + "mExternalStorageReady");
            return false;
        }

	    mScreenRecordDir = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES),
    			SCREENRECORD_DIR_NAME);
	
        mHandler.post(mScreenRecordStartRunnable);
        mGetSDAvailableSizeThread = new GetSDAvailableSizeThread();
        mGetSDAvailableSizeThread.start();
             
        Log.d(TAG, "startScreenRecord");
        mIsRecording = true;
        notifyChanged();        
        return true;
    }
     
    public boolean stopScreenRecord() {
        if (!mIsRecording) {
        	Log.d(TAG, "stopScreenRecord do nothing because screen record not begin ");
            return false;
        }
        mHandler.post(mScreenRecordStopRunnable);
        if (mGetSDAvailableSizeThread != null) {
    	    mGetSDAvailableSizeThread.cancel();
        }
        Log.d(TAG, "stopScreenRecord");
        mIsRecording = false;
        notifyChanged();
        return true;
    }    
   
}
