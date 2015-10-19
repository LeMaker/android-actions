package com.actions.ota;

import android.app.Application;
import android.os.Handler;

import com.actions.download.DownloadHelper;
import com.actions.download.FtpDownload;
import com.actions.download.HttpDownload;
import com.actions.model.UpdateInfo;
import com.actions.userconfig.UserConfig;
import com.actions.utils.Debug;
import com.actions.utils.Utilities;
import android.util.Log;

public class UpdateApplication extends Application {
    public static final int HTTP = 1 << 0;
    public static final int FTP = 1 << 1;
    public static final int UNKNOWN = 1 << 2;
    
    public static final int WORK_ON_UNKNOWN = 0;
    public static final int WORK_ON_CHECKING = 1 << 0;
    public static final int WORK_ON_DOWNLOAD = 1 << 1;
    
    private static final String TAG = "ota.UpdateApplication";
    private static final boolean DEBUG = true;
    
    /**
     * US_KEY is used as a key to pass DownloadStatus instance.
     */
    public static final String US_KEY = "actions";
    
    /**
     * mHandler is used to handle download status, 
     * but we don't feel like to handle download status in Activity directly,
     * so this instance is only used in DownloadStateMachine.
     */
    public Handler mHandler;
    
    /**
     * 
     */
    public UpdateInfo mUpdateInfo;
    
    /**
     * it means where this APP work on...
     */
    private int mWorkOn = WORK_ON_UNKNOWN;
    
    public DownloadHelper mDownloadHelper;
    
    private static UpdateApplication mInstance;
    
    /**
     * we support two downloading modes: http & ftp
     */
    public int mMode = UNKNOWN;
    
    public void onCreate(){
        // TODO Auto-generated method stub
        mInstance = this;
        
        String host = android.os.SystemProperties.get("ro.ota.server", "ftp://**.**.***/");
        String tmpHost = host.toLowerCase();

        if (DEBUG) Log.d(TAG,"onCreate host=" + host);
        if(tmpHost.startsWith("http://")) {
            mMode = HTTP;
            UserConfig.mServerIP = host;
            mDownloadHelper = new HttpDownload(this);
        } else if (tmpHost.startsWith("ftp://")){
            // ftp format: 'ftp://user:password@uri : port', 
            // for example : ftp://test:1234@58.254.217.101:21
            // for example : ftp://test:1234@58.254.217.101/gs705a/:21
            mMode = FTP;
            String dir = "";
            host = host.substring("ftp://".length());
            if (host.indexOf('@') < 0) {
                // no user name and password
                UserConfig.ftpUserName = "";
                UserConfig.ftpPassword = "";
            } else {
                String name = host.substring(0, host.indexOf('@'));
                if (name.indexOf(':') < 0) {
                    UserConfig.ftpUserName = name;
                    UserConfig.ftpPassword = "";
                } else {
                    UserConfig.ftpUserName = name.substring(0, name.indexOf(':'));
                    UserConfig.ftpPassword = name.substring((name.indexOf(':') + 1));
                }
                host = host.substring(host.indexOf('@') + 1);
            }
                
            if (host.indexOf(':') < 0) {
                // defaut ftp port
                UserConfig.ftpPort = 21;
            } else {
                UserConfig.ftpPort = Integer.parseInt(host.substring(host.indexOf(':') + 1));
                host = host.substring(0, host.indexOf(':') + 1);
            }
            
            if (host.indexOf('/') > 0) {
                dir = host.substring(host.indexOf('/'));
                host = host.substring(0, host.indexOf('/'));
            }
            UserConfig.ftpHost = host;
            UserConfig.ftpRootDir = dir;
            if ((!dir.equals("")) && (dir.charAt(dir.length() - 1) == '/')) {
                UserConfig.remoteXmlPath = dir + "UpdateInfo_" + Utilities.mDeviceModel + "_" + Utilities.mManufacturer + ".xml";
            } else {
                UserConfig.remoteXmlPath = dir + "/UpdateInfo_" + Utilities.mDeviceModel + "_" + Utilities.mManufacturer + ".xml";
            }
            
            if (DEBUG) Log.d(TAG,"onCreate ftpHost=" + UserConfig.ftpHost);
            if (DEBUG) Log.d(TAG,"onCreate ftpRootDir=" + UserConfig.ftpRootDir);
            if (DEBUG) Log.d(TAG,"onCreate ftpUserName=" + UserConfig.ftpUserName);
            if (DEBUG) Log.d(TAG,"onCreate ftpPassword=" + UserConfig.ftpPassword);
            if (DEBUG) Log.d(TAG,"onCreate remoteXmlPath=" + UserConfig.remoteXmlPath);
            if (DEBUG) Log.d(TAG,"onCreate ftpPort=" + UserConfig.ftpPort);
            
            mDownloadHelper = new FtpDownload(this);
        } else { 
            Log.e(TAG,"format error ro.ota.server=" + host);
            mMode = UNKNOWN;
        }
        super.onCreate();
    }
    
    public static UpdateApplication instance() {
        
        /**
         * I was wondered ...
         * will Application instance will create by Android OS?
         * any way, just make sure.
         */
        if(mInstance == null) {
            if(DEBUG) Log.d(TAG,"Android it didn't create Application instance");
            mInstance = new UpdateApplication();
        }
        return mInstance;
    }
    
    /**
     * @deprecated use this before we find a other way, use it the less the better
     * @return
     */
    public boolean isWorkOnCheckingOrDownloading() {
        if(mWorkOn == WORK_ON_CHECKING || mWorkOn == WORK_ON_DOWNLOAD)
            return true;
        return false;
    }
    
    /**
     * @deprecated
     * @param yes
     */
    public void workOnChecking(boolean yes) {
        if(yes)
            mWorkOn = WORK_ON_CHECKING;
        else
            mWorkOn = WORK_ON_UNKNOWN;
    }
    
    /**
     * @deprecated
     * @param yes
     */
    public void workOnDownloading(boolean yes) {
        if(yes)
            mWorkOn = WORK_ON_DOWNLOAD;
        else
            mWorkOn = WORK_ON_UNKNOWN;
    }
}
