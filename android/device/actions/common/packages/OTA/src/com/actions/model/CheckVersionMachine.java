package com.actions.model;

import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.widget.Toast;

import com.actions.download.DownloadHelper;
import com.actions.ota.MainActivity;
import com.actions.ota.R;
import com.actions.ota.UpdateApplication;
import com.actions.utils.Debug;

public class CheckVersionMachine {
    Context mContext;
    CheckVersionCallBack mCallBack;
    DownloadHelper mDownloadHelper;
    CheckResult mCheckResult;
    private boolean isDoingCheck = false;
    
    public CheckVersionMachine(Context context, CheckVersionCallBack ccb) {
        mContext = context;
        mCallBack = ccb;
        mDownloadHelper = UpdateApplication.instance().mDownloadHelper;
    }
    
    public void pause() {
        
    }
    
    public void resume() {
        
    }
    
    public void stop() {
        UpdateApplication.instance().workOnChecking(false);
    }
    
    public void check() {
        UpdateApplication.instance().workOnChecking(true);
        if(!isReady()) {
            CheckResult cr = new CheckResult();
            cr.setResult(CheckResult.RESULT_CHECK_NOT_READY);
            mCallBack.onCVCheckNotReady(cr);
            return;
        }
        mCallBack.onCVStart();
        isDoingCheck = true;
        new CheckUpdateXml().start();
    }
    
    /**
     * Think about this case:
     * When the users perform a lot of click actions and then invoke a lot of thread to do check version stuff.
     * It should mess up all the state machines. 
     * So we do have to tell whether is it time to do check.
     * @return
     */
    private boolean isReady() {
        // TODO Auto-generated method stub
        return !isDoingCheck;
    }

    
    /**
     * This handler is only used to handle checkversion stuff
     */
    private Handler mCheckHanlder = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            // dismiss the progress bar
            switch (msg.what) {
                case CheckResult.RESULT_CONTENT_NOT_FOUND:
                    mCallBack.onCVContentNotFound(mCheckResult);
                    break;
                case CheckResult.RESULT_UPDATE_NEEDED:
                    mCallBack.onCVUpdateNeeded(mCheckResult);
                    break;
                case CheckResult.RESULT_UP_TO_DATE:
                    mCallBack.onCVUpToDate(mCheckResult);
                    break;
                case CheckResult.RESULT_CHECK_NOT_READY:
                    mCallBack.onCVCheckNotReady(mCheckResult);
                    break;
                case CheckResult.RESULT_CONNECTIVITY_ERROR:
                    mCallBack.onCVError(mCheckResult);
                    break;
                case CheckResult.RESULT_SERVER_NOT_FOUND:
                    mCallBack.onCVServerNotFound(mCheckResult);
                    break;
                case CheckResult.RESULT_UNKNOW:
                    mCallBack.onCVUnknown(mCheckResult);
                    break;
                default:
                    break;
            }
            
            isDoingCheck = false;
        }
    };
    
    /** 
    * @ClassName: CheckUpdateXml
    * @Description: Parse xml file thread
    * @author  
    * @date 2015-1-30 16:05:02 
    *  
    */
    class CheckUpdateXml extends Thread {
        Message msg = new Message();

        @Override
        public void run() {
            DownloadHelper dlh = UpdateApplication.instance().mDownloadHelper;
            if (dlh != null) {
                mCheckResult = dlh.check();
                UpdateApplication.instance().mUpdateInfo = mCheckResult.mUpdateInfo;
                msg.what = mCheckResult.getResult();
            
                //fix BUG00271034, we don't need to tell UI thread that much hurry
                mCheckHanlder.sendMessageAtTime(msg, SystemClock.uptimeMillis() + 2*1000);
            } else {
                msg.what = CheckResult.RESULT_CONNECTIVITY_ERROR;
                mCheckHanlder.sendMessage(msg);
            }
        }
        
        
    }// end CheckUpdateXml  
}
