package com.actions.download;

import android.app.DownloadManager;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.webkit.MimeTypeMap;

import com.actions.model.CheckResult;
import com.actions.model.DownloadStatus;
import com.actions.model.UpdateInfo;
import com.actions.ota.R;
import com.actions.utils.Debug;
import com.actions.utils.Utilities;
import com.actions.utils.VersionUtils;

import java.net.URL;
import android.util.Log;
import java.io.File;

public class HttpDownload implements DownloadHelper {

    private static final String TAG = "ota.HttpDownload";
    private static final boolean DEBUG = true;
    private Context mContext;
    private DownloadManager mDownloadManager;
    private VersionUtils mVersionUtils;
    private static long mDownloadQueue = -1L;
    
    private long totalSize = -1L;
    private long downloadSize = -1L;
    
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
        if(mDownloadManager == null)
            mDownloadManager = (DownloadManager)mContext.getSystemService(mContext.DOWNLOAD_SERVICE);
        
        if(DEBUG) Log.d(TAG,"download uri:"+uri);
        cancelOld();
        //start to download
        Uri resource = Uri.parse(uri);
    
        DownloadManager.Request request = new DownloadManager.Request(resource);
        request.setAllowedOverRoaming(false);
        // set the file type
        MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
        String mimeString = mimeTypeMap.getMimeTypeFromExtension(MimeTypeMap.getFileExtensionFromUrl(uri));
        request.setMimeType(mimeString);
        // show in systemUI?
        request.setShowRunningNotification(false);
        request.setVisibleInDownloadsUi(true);
        // set the download path        
        request.setDestinationInExternalPublicDir("/", Utilities.mRecoveryFileName);
        request.setTitle(mContext.getString(R.string.start_download));
        mDownloadQueue = mDownloadManager.enqueue(request); 
    }

    public void stop() {
        // TODO Auto-generated method stub
        if(DEBUG) Log.d(TAG,"stop mDownloadQueue="+mDownloadQueue);
        if(mDownloadQueue > 0) {
            //DONT CALL remove() here, it will delete the file just has been downloaded.
            //mDownloadManager.remove(new long[]{mDownloadQueue});
            //mDownloadQueue = -1L;
        }
    }
    
    
    @Override
    public void pause() {
        // TODO Auto-generated method stub
        
    }

    @Override
    public void resume() {
        // TODO Auto-generated method stub
        
    }

    private HttpDownload() {
        
    }
    public HttpDownload(Context context) {
        mContext = context;
        mVersionUtils = new VersionUtils(context);
    }

    int mStatusErrCount = 0;
    public DownloadStatus getDownloadStatus() {
        // TODO Auto-generated method stub
        if(mDownloadManager == null)
            return null;
        
        DownloadManager.Query query = new DownloadManager.Query();
        query.setFilterById(mDownloadQueue);
        Cursor c = mDownloadManager.query(query);
        DownloadStatus dlStatus = new DownloadStatus();
        if(c != null){
            if(c.moveToFirst()){
                
                int status = c.getInt(c.getColumnIndex(DownloadManager.COLUMN_STATUS));
                if(DEBUG) Log.d(TAG,"download status:"+status);
                /*
                 * make sure every extra you've added should be cleared.
                 */
                switch (status) {
                    case DownloadManager.STATUS_PAUSED:
                        mStatusErrCount++;
                        dlStatus.setStatus(DownloadStatus.STATUS_PAUSED);
                        if (mStatusErrCount > 3) {
                            dlStatus.setErr(errorFormat(c.getInt(c.getColumnIndex(DownloadManager.COLUMN_REASON))));
                        }
                        break;
                    case DownloadManager.STATUS_PENDING:
                        mStatusErrCount++;
                        dlStatus.setStatus(DownloadStatus.STATUS_PENDING);
                        if (mStatusErrCount > 3) {
                            dlStatus.setErr(errorFormat(c.getInt(c.getColumnIndex(DownloadManager.COLUMN_REASON))));
                        }
                        break;
                    case DownloadManager.STATUS_FAILED:
                        mStatusErrCount++;
                        dlStatus.setStatus(DownloadStatus.STATUS_FAILED);
                        if (mStatusErrCount > 3) {
                            dlStatus.setErr(errorFormat(c.getInt(c.getColumnIndex(DownloadManager.COLUMN_REASON))));
                            if (mDownloadQueue > 0) {
                                mDownloadManager.remove(new long[]{mDownloadQueue});
                                mDownloadQueue = -1;
                            }
                        }
                        break;
                    case DownloadManager.STATUS_RUNNING:
                        long sDownloaded = c.getInt(c.getColumnIndex(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR));
                        long sTotal = c.getInt(c.getColumnIndex(DownloadManager.COLUMN_TOTAL_SIZE_BYTES));
                        int percent = 0;
                        if(sTotal == 0) {
                            percent = 1;
                        } else {
                            percent = (int) (sDownloaded * 100 / sTotal);
                            if(percent <= 1) 
                                percent = 1;
                        }
                        mStatusErrCount = 0;
                        downloadSize = sDownloaded;
                        totalSize = sTotal;
                        if(DEBUG) Log.d(TAG,"Downloaded: " + sDownloaded + ", " + percent + "%");
                        dlStatus.setStatus(DownloadStatus.STATUS_RUNNING);
                        dlStatus.setProgress(percent);
                        break;
                    case DownloadManager.STATUS_SUCCESSFUL:
                        dlStatus.setStatus(DownloadStatus.STATUS_FINISHED);
                        break;
                    default:
                        break;
                }
            }
        }
        if(c != null){
            c.close();
        }
        
        return dlStatus;
    }

    public void cancelOld() {
        // TODO Auto-generated method stub
        
        File f = new File(Utilities.SdCardRoot + File.separator + Utilities.mRecoveryFileName);
        if (f != null){
            f.delete();
        }
                        
        if(mDownloadManager == null)
            return;
        
        //DownloadManager.Request request = new DownloadManager.Request(resource);
        DownloadManager.Query query = new DownloadManager.Query();
        Cursor cursor = null;
        String local_file;
        try {
            cursor = mDownloadManager.query(query);
            if (cursor != null ) {
                for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                    local_file = cursor.getString(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_FILENAME));
if(DEBUG) Log.d(TAG,"cancelOld local fiel="+local_file);
                    if ((local_file != null) && (local_file.equalsIgnoreCase(Utilities.RecoveryPathName) 
                        || local_file.equalsIgnoreCase(Utilities.RecoveryStoragePathName))) {
                        int id = cursor.getInt(cursor.getColumnIndexOrThrow(DownloadManager.COLUMN_ID));
                        int num = mDownloadManager.remove(new long[]{id});
                        
                        if(DEBUG) Log.d(TAG,"cancelOld id="+id + " num=" + num );
                    }
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        
        if(mDownloadQueue > 0) {
            //if(DEBUG) Log.d(TAG,"cancelOld mDownloadQueue="+mDownloadQueue);
            //mDownloadManager.remove(new long[]{mDownloadQueue});
            mDownloadQueue = -1L;
        }
    }

    /**
     * To get a xml stream seems not that diffcult.
     * so process all cases here.
     */
    public CheckResult check() {
        // TODO Auto-generated method stub
        CheckResult cr = new CheckResult();
        UpdateInfo ui = mVersionUtils.CheckVersion(mVersionUtils.getXmlByHttp());
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

    private int errorFormat(int err) {
        /*
         *  public static final int ERROR_CONNECTIVITY = 1 << 0;
        public static final int ERROR_NO_SPACE = 1 << 1;
        public static final int ERROR_UNKNOWN = 1 << 2;
         */
        int ret = DownloadStatus.ERROR_UNKNOWN;
        switch (err) {
            case DownloadManager.ERROR_CANNOT_RESUME:
            case DownloadManager.ERROR_DEVICE_NOT_FOUND:
            case DownloadManager.ERROR_FILE_ALREADY_EXISTS:
            case DownloadManager.ERROR_FILE_ERROR:
            case DownloadManager.ERROR_HTTP_DATA_ERROR:
            case DownloadManager.ERROR_TOO_MANY_REDIRECTS:
            case DownloadManager.ERROR_UNHANDLED_HTTP_CODE:
            case DownloadManager.PAUSED_QUEUED_FOR_WIFI:
            case DownloadManager.PAUSED_WAITING_FOR_NETWORK:
            case DownloadManager.PAUSED_WAITING_TO_RETRY:
                ret = DownloadStatus.ERROR_CONNECTIVITY;
                break;
                
            case DownloadManager.ERROR_INSUFFICIENT_SPACE:
                ret = DownloadStatus.ERROR_NO_SPACE;
                break;
                
            case DownloadManager.ERROR_UNKNOWN:
            case DownloadManager.PAUSED_UNKNOWN:
            default:
                ret = DownloadStatus.ERROR_UNKNOWN; 
                break;
        }
        return ret;
    }
}
