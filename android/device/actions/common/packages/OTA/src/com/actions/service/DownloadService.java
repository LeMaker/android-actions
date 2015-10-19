package com.actions.service;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;

import com.actions.download.DownloadHelper;
import com.actions.model.DownloadStatus;
import com.actions.model.UpdateInfo;
import com.actions.ota.UpdateApplication;

import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;
import android.util.Log;
/**
 * Download UpdateDownloadUrl to /mnt/sdcard/update.zip
 * and notice the progress to UpateOnlineActivity
 * input:  started by intent with UpdateDownloadUrl and UpdateFileSize
 * output: send mUpdateProgress or mFinishDownload
 * @author Laura Wan
 *
 */
public class DownloadService extends Service {
	private Timer mTick;
	private UpdateApplication mApp;
	private UpdateInfo mUpdateInfo;
	private DownloadHelper mDownloadHelper;
	private Handler mHanlder;
	
	private static final String TAG = "ota.DownloadService";
    private static final boolean DEBUG = true;
    
	private final PBinder pBinder = new PBinder();
	
	public static final int UPDATE_DOWNLOAD_STATUS = 17 ;
	public static final int DOWNLOAD_ERROR = 33;
	
	
	
	public class PBinder extends Binder {
	    public DownloadService getService() {
            return DownloadService.this;
        }
	}
	
	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
	    mApp = UpdateApplication.instance();
        mUpdateInfo = mApp.mUpdateInfo;
        mDownloadHelper = mApp.mDownloadHelper;
        mHanlder = mApp.mHandler;
        
        if(DEBUG) Log.d(TAG,"onBind");
        
//        mDownloadHelper.cancelOld();
        try {
            mDownloadHelper.download(mUpdateInfo.getDownloadUrl());
            mTick = new Timer();
            mTick.schedule(task, new Date(), 1000);
        }catch(Exception e) {
            if(DEBUG) Log.d(TAG,"onBind err");
	        Message msg =  mHanlder.obtainMessage();
	        msg.what = DOWNLOAD_ERROR; 
	        mHanlder.sendMessage(msg);
        }
        
		return pBinder;
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		// service onStartCommand
        return super.onStartCommand(intent, flags, startId);
	}
	
    @Override
    public void onCreate() {
        // TODO Auto-generated method stub
        super.onCreate();
    }

    public void onDestroy() {
        // TODO Auto-generated method stub
        super.onDestroy();
    }

    
    @Override
    public boolean onUnbind(Intent intent) {
        // TODO Auto-generated method stub
        if(mTick != null) {
            mTick.cancel();
        }
        if(mDownloadHelper != null)
            mDownloadHelper.stop();
        
        /*
         * make sure we return false, since we would like service called onBind instead of onReBind.
         */
        return false;
    }

    /**
     * This method seems like a abstract layer to filter any stuff passed by download service.
     * Actually, we can do more here to make the datas and actions, which will be passed to View, simpler and more specific 
     * 
     */
	private void updateZipSize(){
	    DownloadStatus us = mDownloadHelper.getDownloadStatus();
	    if (us != null) {
	        Bundle data = new Bundle();
	        data.putSerializable(UpdateApplication.US_KEY, us);
	        Message msg =  mHanlder.obtainMessage();
	        msg.setData(data);
	        msg.what = UPDATE_DOWNLOAD_STATUS; 
	        mHanlder.sendMessage(msg);
	    }
	}
	
	public void pause() {
	    if(mDownloadHelper != null)
            mDownloadHelper.pause();
	}
	
	public void resume() {
	    if(mDownloadHelper != null)
            mDownloadHelper.resume();
	}
	
    /**
	 * update the downloaded size of update.zip
	 */
	TimerTask task = new TimerTask() {		
		@Override
		public void run() {
		    updateZipSize();
		}
	};	
	
}
