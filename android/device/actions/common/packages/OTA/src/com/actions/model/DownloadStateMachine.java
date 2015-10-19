package com.actions.model;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.AsyncTask;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.provider.Settings;

import com.actions.download.DownloadHelper;
import com.actions.service.DownloadService;
import com.actions.utils.Debug;
import com.actions.utils.MD5;
import com.actions.utils.Utilities;
import com.actions.ota.R;
import com.actions.ota.UpdateApplication;
import android.util.Log;
import java.io.File;
import java.io.Serializable;
import android.content.DialogInterface;
import android.app.AlertDialog;

/**
 * The DownloadStateMachine plays a Controller role in MVC model.
 * It can handle any user actoins (passed from View) and update View by simple and specific intent.
 * @author caichsh
 *
 */
public class DownloadStateMachine {
    /**
     * It might confused that we have two states, STATE_XXXX and DownloadStatus.STATUS_XXX
     * If you want to change this machine, you should be totally understood that:
     * STATE_XXXs are used to clarify the machine's state TO the View (Activities),and
     * DownloadStatus.STATUS_XXXs are used to perform and shuttle the download thread To the machine(this class).
     */
    public static final int STATE_DOWNLOADING = 1 << 0;
    public static final int STATE_PAUSED = 1 << 1;
    public static final int STATE_STOPED = 1 << 2;
    public static final int STATE_FINISHED = 1 << 3;
    public static final int STATE_CHECKING = 1 << 4;
    public static final int STATE_CHECKED = 1 << 5;
    public static final int STATE_INITED = 1 << 5;
    public static final int STATE_UNKNOWN = 0;
    
    private int mState = STATE_INITED;
    private static final String TAG = "ota.DownloadStateMachine";
    private static final boolean DEBUG = true;
    
    /**
     * mTeller is used to update Views.
     * but now we use callback interface instead, so just ignore this.
     */
    public Handler mTeller;
    
    private Context mContext;
    
    private UpdateApplication mApp;
    
    private DownloadHelper mDownloadHelper;
    
    private DownloadStateCallBack mCallBack;
    
    private DownloadStatus mUpdateStatus;
    
    private boolean paused = false;
    
    /**
     * Controller API used to start a download service.
     */
    public void start() {
        if(DEBUG) Log.d(TAG,"start");
        UpdateApplication.instance().workOnDownloading(true);
        if(mState == STATE_STOPED || mState == STATE_UNKNOWN || mState == STATE_INITED) {
            mState = STATE_DOWNLOADING;
            mCallBack.onDownloadStart();
            doStart();
        }
    }
    
    /**
     * Controller API used to stop a download service.
     * Use it Only if you know definitely it is time to stop a download service, such as a CANCEL actions performed by users.
     */
    public void stop() {
        if(DEBUG) Log.d(TAG,"stop");
        UpdateApplication.instance().workOnDownloading(false);
        if(mState == STATE_UNKNOWN || mState == STATE_DOWNLOADING || mState == STATE_PAUSED) {
            mCallBack.onDownloadStop();
            doStop();
            mState = STATE_STOPED;
        }
    }
    
    /**
     * Controller API used to pause a download service.
     * When download in HTTP protocol we cannot pause it acturally. But download manager will tell UI thread when it paused itself.
     * 
     */
    public void pause() {
        if(DEBUG) Log.d(TAG,"pause");
        if(mState == STATE_DOWNLOADING) {
            mState = STATE_PAUSED;
            if(mService != null)
                mService.pause();
        }
    }
    
    /**
     * Contoller API used to resume from paused state.
     * Likewise, when download in HTTP protocol, we can be only told by download manager.
     */
    
    public void resume() {
        if(DEBUG) Log.d(TAG,"resume");
        if(mState == STATE_PAUSED) {
            mState = STATE_DOWNLOADING;
            if(mService != null) {
                mService.resume();
            }
        }
    }
    
    public int getState() {
        return mState;
    }
    
    public int setState(int state) {
        return mState = state;
    }
    
    /**
     * mListener is used to listen download procedure. 
     */
    public Handler mListener = new Handler() {

        public void handleMessage(Message msg) {
            // TODO Auto-generated method stub
            /**
             * if we have stoped the thread by call stop(), we hope it will perform immediately, and we don't care what next happened.
             */
            if(mState == STATE_STOPED || mState == STATE_CHECKED || mState == STATE_CHECKING)
                return;
           
            switch (msg.what) {   
                case DownloadService.UPDATE_DOWNLOAD_STATUS: 
                {
                    Bundle b = msg.getData();
                    mUpdateStatus.copyFrom((DownloadStatus)b.getSerializable(UpdateApplication.US_KEY));
                    switch (mUpdateStatus.getStatus()) {
                        case DownloadStatus.STATUS_PENDING:
                            mState = STATE_PAUSED;
                            mCallBack.onDownloadPended(mUpdateStatus);
                            break;
                        case DownloadStatus.STATUS_RUNNING:
                            mState = STATE_DOWNLOADING;
                            if(paused) {
                                mCallBack.onDownloadResume(mUpdateStatus);
                                paused = false;
                            } else {
                                mCallBack.onDownloadRuning(mUpdateStatus);
                            }
                            break;
                        case DownloadStatus.STATUS_PAUSED:
                            paused = true;
                            mState = STATE_PAUSED;
                            mCallBack.onDownloadPause(mUpdateStatus);
                            break;
                        case DownloadStatus.STATUS_FINISHED:
                            /*
                             * We use STATE_FINISHED only if we have finished checking it's MD5.
                             * So we stop it and going to check md5
                             */
                            mState = STATE_CHECKING;
                            mCallBack.onDownloadFinish(mUpdateStatus);
                            (new CheckMD5Task()).execute();
                            break;
                        case DownloadStatus.STATUS_FAILED:
                            if(DEBUG) Log.d(TAG,"STATUS_FAILED");
                            mState = STATE_UNKNOWN;
                            mCallBack.onDownloadFailed(mUpdateStatus);
                            break;
                        case DownloadStatus.STATUS_RESUMED:
                            mState = STATE_DOWNLOADING;
                            mCallBack.onDownloadResume(mUpdateStatus);
                            break;
                        case DownloadStatus.STATUS_WAIT_TO_LOGIN:
                            mState = STATE_DOWNLOADING;
                            //dont tell anyone
                            //mCallBack.onDownloadResume(mUpdateStatus);
                            break;
                        default:
                            mState = STATE_UNKNOWN;
                            if(DEBUG) Log.d(TAG,"default status error");
                            mCallBack.onDownloadFailed(mUpdateStatus);
                            break;
                    }
                    break;
                }
                case DownloadService.DOWNLOAD_ERROR: 
                    if(DEBUG) Log.d(TAG,"DOWNLOAD_ERROR");
                    mState = STATE_UNKNOWN;
                    mCallBack.onDownloadFailed(mUpdateStatus);
                    break;
                default:
                    mState = STATE_UNKNOWN;
                    if(DEBUG) Log.d(TAG,"default error");
                    mCallBack.onDownloadFailed(mUpdateStatus);
                    break;
            }
        }

    };

    private void doStart() {
        if(DEBUG) Log.d(TAG,"doStart");
        (new CheckLocalTask()).execute();
        
        if(Utilities.checkWifiState(mContext)){
            setWifiDormancy();
        }
        
        /*
         * We have to tell whether we need to start a download service or not.
         */
    }

    private void doStop() {
        if(DEBUG) Log.d(TAG,"doStop");
        if(Utilities.checkWifiState(mContext)){
            restoreWifiDormancy();
        }
        mContext.unbindService(mServiceConnection);
        //mContext.stopService(new Intent(mContext, DownloadService.class));
    }

    private void doPause() {
        if(DEBUG) Log.d(TAG,"doPause");
    }
    
    private void doResume() {
        if(DEBUG) Log.d(TAG,"doResume");
    }
    
    public DownloadStatus getUpdateStatus() {
        if(mUpdateStatus == null) {
            mUpdateStatus = new DownloadStatus(DownloadStatus.STATUS_UNKNOWN);
        }
        if(DEBUG) Log.d(TAG,"getUpdateStatus DownloadStatus=" + mUpdateStatus);
        return mUpdateStatus;
    }

    private void restoreWifiDormancy(){
        SharedPreferences prefs = mContext.getSharedPreferences(Utilities.mSharedPreferencesMark, 
                Context.MODE_PRIVATE);
        if(DEBUG) Log.d(TAG,"restoreWifiDormancy");
            int defaultPolicy = prefs.getInt(mContext.getString(R.string.wifi_sleep_policy_default), 
                    Settings.Global.WIFI_SLEEP_POLICY_DEFAULT);
            Settings.Global.putInt(mContext.getContentResolver(), 
                    Settings.System.WIFI_SLEEP_POLICY, 
                    defaultPolicy);
    }
    
    private void setWifiDormancy(){       
        if(DEBUG) Log.d(TAG,"setWifiDormancy");  
        int value = Settings.Global.getInt(mContext.getContentResolver(), 
                    Settings.Global.WIFI_SLEEP_POLICY,  
                    Settings.Global.WIFI_SLEEP_POLICY_DEFAULT);     
        SharedPreferences prefs = mContext.getSharedPreferences(Utilities.mSharedPreferencesMark, 
                Context.MODE_PRIVATE);
        Editor editor = prefs.edit();
        editor.putInt(mContext.getString(R.string.wifi_sleep_policy_default), value); 
        editor.commit();
            if(Settings.Global.WIFI_SLEEP_POLICY_NEVER != value){
                Settings.Global.putInt(mContext.getContentResolver(), 
                        Settings.Global.WIFI_SLEEP_POLICY, 
                        Settings.Global.WIFI_SLEEP_POLICY_NEVER);
            }
    }
    
    /**
     * Make sure whether have we already finished downloading and there has nothing to update
     * @author caichsh
     *
     */
    private class CheckLocalTask extends AsyncTask<Void, Void, Boolean> {

        @Override
        protected Boolean doInBackground(Void... params) {
            if(DEBUG) Log.d(TAG,"CheckLocalTask.doInBackground");
            // TODO Auto-generated method stub
            File sFile = new File(Utilities.SdCardRoot + File.separator + Utilities.mRecoveryFileName);
            if(sFile.exists()) {
                if(DEBUG) Log.d(TAG,"CheckLocalTask.doInBackground file exists");
                if(DEBUG) Log.d(TAG,"getFileSizeComp=" + UpdateApplication.instance().mUpdateInfo.getFileSizeComp() + " length=" + sFile.length());
                if(UpdateApplication.instance().mUpdateInfo.getFileSizeComp() == sFile.length()) {
                    if(MD5.checkMD5(UpdateApplication.instance().mUpdateInfo.getMd5(), Utilities.SdCardRoot + File.separator + Utilities.mRecoveryFileName)) {
                        return true;
                    }
                }
                sFile.delete();
            }
            return false;
        }

        @Override
        protected void onPreExecute() {
            if(DEBUG) Log.d(TAG,"CheckLocalTask.onPreExecute");
            // TODO Auto-generated method stub
            mCallBack.onDeleteOldStart();
            mState = STATE_CHECKING;
            super.onPreExecute();
        }

        @Override
        protected void onPostExecute(Boolean result) {
            if(DEBUG) Log.d(TAG,"CheckLocalTask.onPostExecute");
            // TODO Auto-generated method stub
            //we start download anyway
            mCallBack.onDeleteOldFinished();
            if(!result.booleanValue()) {
                mState = STATE_UNKNOWN;
                if(DEBUG) Log.d(TAG,"Check local file before starting download failed, so download it");
                mContext.bindService(new Intent(mContext, DownloadService.class), mServiceConnection, Context.BIND_AUTO_CREATE);
                //mContext.startService(new Intent(mContext, DownloadService.class));
            } else {
                mCallBack.showInstallDialog();
                //if(Utilities.checkWifiState(mContext)){
                //    restoreWifiDormancy();
                //}
                if(DEBUG) Log.d(TAG,"Check local file before starting download successful!");
                //mState = STATE_CHECKED;
                //mCallBack.onCheckMD5Finish(true);
            }
            super.onPostExecute(result);
        }
        
        
        
    }
    
    /**
     * In our state machine, we only used checkmd5task after we have finished download from the Internet.
     * @author caichsh
     *
     */
    class CheckMD5Task extends AsyncTask<Void, Void, Boolean> {

        @Override
        protected Boolean doInBackground(Void... params) {
            if(DEBUG) Log.d(TAG,"CheckMD5Task.doInBackground");
            // TODO Auto-generated method stub
            if(MD5.checkMD5(UpdateApplication.instance().mUpdateInfo.getMd5(), (Utilities.SdCardRoot + Utilities.mRecoveryFileName)))
                return true;
            return false;
        }

        @Override
        protected void onPreExecute() {
            if(DEBUG) Log.d(TAG,"CheckMD5Task.onPreExecute");
            // TODO Auto-generated method stub
            mCallBack.onCheckMD5Start();
            super.onPreExecute();
        }

        @Override
        protected void onPostExecute(Boolean result) {
            if(DEBUG) Log.d(TAG,"CheckMD5Task.onPostExecute");
            // TODO Auto-generated method stub
            doStop();
            mState = STATE_CHECKED;
            mCallBack.onCheckMD5Finish(result.booleanValue());
        }
        
    }
    
    public DownloadStateMachine(Context sContext, DownloadStateCallBack sCallBack) {
        this(sContext, null, sCallBack);
    }
    
    public DownloadStateMachine(Context sContext, Handler sHanlder, DownloadStateCallBack sCallBack) {
        // TODO Auto-generated constructor stub
        mContext = sContext;
        mTeller = sHanlder;
        if(mContext != null) {
            mApp = UpdateApplication.instance();
        }
        
        mDownloadHelper = mApp.mDownloadHelper;
        
        mApp.mHandler = mListener;
        
        mCallBack = sCallBack;
        
        mUpdateStatus = new DownloadStatus(DownloadStatus.STATUS_RUNNING, null);
        
    }
    
    public void CancleDownload() {
        mDownloadHelper.cancelOld();
    }
    
    public ServiceConnection getServiceConnection() {
        return mServiceConnection;
    }
    
    private DownloadService mService = null;
    private ServiceConnection mServiceConnection = new ServiceConnection() {
        
        @Override
        public void onServiceDisconnected(ComponentName name) {
            // TODO Auto-generated method stub
        }
        
        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            // TODO Auto-generated method stub
            DownloadService.PBinder iBinder = (DownloadService.PBinder)service;
            mService = iBinder.getService();
        }
    };
}
