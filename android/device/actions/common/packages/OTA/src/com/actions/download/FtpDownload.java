package com.actions.download;

import android.content.Context;

import com.actions.model.CheckResult;
import com.actions.model.DownloadStatus;
import com.actions.model.UpdateInfo;
import com.actions.userconfig.UserConfig;
import com.actions.utils.FileUtils;
import com.actions.utils.Utilities;
import com.actions.utils.VersionUtils;

import java.io.IOException;

public class FtpDownload implements DownloadHelper {

    Context mContext;
    private String mUri = null;
    private MyContinueFTP myContinueFTPFile;
    private VersionUtils mVersionUtils;
    
    private long totalSize = -1L;
    private long downloadSize = -1L;
    
    private DownloadThread mDownloadThread = null;
    
    /*
     * Really bad code here!!!!!
     * we cannot use a global var to tell whether it is contiune download or not. we it seems the best way so far....
     */
    private boolean mContinued = false;
    
    public UpdateInfo getUpdateInfo() {
        // TODO Auto-generated method stub
        return null;
    }

    public long getDownloadSize() {
        // TODO Auto-generated method stub
        return downloadSize;
    }

    public long getTotalSize() {
        // TODO Auto-generated method stub
        return totalSize;
    }

    public void download(String uri) {
        // TODO Auto-generated method stub
        mUri = uri;
        
        mDownloadThread = new DownloadThread();
        if(myContinueFTPFile != null) {
            try {
                myContinueFTPFile.disconnect();
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        myContinueFTPFile = new MyContinueFTP();
        mDownloadThread.start();
    }

    public void stop() {
        // TODO Auto-generated method stub
        if(myContinueFTPFile != null) {
            try {
                myContinueFTPFile.disconnect();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        if(mDownloadThread != null) {
            mDownloadThread.interrupt();
            mDownloadThread = null;
        }
    }
    
    
    @Override
    public void pause() {
        // TODO Auto-generated method stub
        if (myContinueFTPFile != null
                && myContinueFTPFile.getState() == MyContinueFTP.FTP_STATE_DOWNLOADING) {
            if (mDownloadThread != null) {
                mDownloadThread.interrupt();
                mDownloadThread = null;
            }
            myContinueFTPFile.pause();
        }
    }

    @Override
    public void resume() {
        // TODO Auto-generated method stub
        if(myContinueFTPFile != null) {
            if(myContinueFTPFile.getState() == MyContinueFTP.FTP_STATE_DOWNLOAD_PAUSED) {
                mDownloadThread = new DownloadThread();
                if(myContinueFTPFile != null) {
                    try {
                        myContinueFTPFile.disconnect();
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                }
                mContinued = true;
                mDownloadThread.start();
            }
        }
    }


    class DownloadThread extends Thread {

        @Override
        public void run() {
            if(myContinueFTPFile.connect(UserConfig.ftpHost, UserConfig.ftpPort, UserConfig.ftpUserName, UserConfig.ftpPassword)){
                myContinueFTPFile.download(mUri, mContinued);
                mContinued = false;
            }
        }
    }
    
    public void deleteZip(){
        // whether had update.zip in /mnt/sdcard
        FileUtils mFileUtils = new FileUtils();
        // update.zip is exist
        if(mFileUtils.isFileExist(Utilities.mRecoveryFileName)){
            mFileUtils.deleteFile(Utilities.mRecoveryFileName);
        }
    }
    
    private FtpDownload() {
        
    }
    
    public FtpDownload(Context context) {
        mContext = context;
        mVersionUtils = new VersionUtils(context);
    }

    public DownloadStatus getDownloadStatus() {
        // TODO Auto-generated method stub
       return myContinueFTPFile.getDownloadStatus();
    }

    public void cancelOld() {
        // TODO Auto-generated method stub
        
    }

    public CheckResult check() {
        // TODO Auto-generated method stub
        CheckResult cr = new CheckResult();
        UpdateInfo ui = mVersionUtils.CheckVersion(mVersionUtils.getXmlByFTP());
        if(ui == null) {
            cr.setResult(CheckResult.RESULT_SERVER_NOT_FOUND);
            return cr;
        }
        cr.mUpdateInfo = ui;
        
        if(ui.getUpdateType() != null && ui.getUpdateType().equals("All-download-OTA")) {
            if(ui.getNewVersionComp() > ui.getSystemVersionComp()) {
                cr.setResult(CheckResult.RESULT_UPDATE_NEEDED);
            } else {
                cr.setResult(CheckResult.RESULT_UP_TO_DATE);
            }
        } else if(ui.getUpdateType() != null && ui.getUpdateType().equals("Recent-version-OTA")) {
            if(ui.getNewVersionComp() > ui.getSystemVersionComp()) {
                cr.setResult(CheckResult.RESULT_UPDATE_NEEDED);
            } else {
                cr.setResult(CheckResult.RESULT_UP_TO_DATE);
            }
        } else {
            cr.setResult(CheckResult.RESULT_UNKNOW);
        }
        
        return cr;
    }

}
