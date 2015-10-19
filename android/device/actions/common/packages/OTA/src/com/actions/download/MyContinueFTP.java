package com.actions.download;

import com.actions.model.DownloadStatus;
import com.actions.utils.Debug;
import com.actions.utils.FileUtils;
import com.actions.utils.Utilities;

import org.apache.commons.net.ftp.FTP;
import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPFile;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * A package that :
 * 1)hide apache ftpclient interface from users and only provide easy-understand apis to users
 * 2)support continue-downloading
 * 
 * @author caichsh
 *
 */
public class MyContinueFTP {
	public static final int FTP_STATUS_OK = 1 << 0;
	public static final int FTP_STATUS_ERROR_LOGIN = 1 << 0;
	public static final int FTP_STATUS_ERROR_URL = 1;
	public static final int FTP_STATUS_ERROR_DOWNLOAD = 1;
	public static final int FTP_STATUS_ERROR = 1;
	public static final int FTP_STATUS_UNKNOWN = 0;
	
	
	public static final int FTP_STATE_UNINITIAL = 1 << 0;
	public static final int FTP_STATE_UNCONNECT = 1 << 1;
	public static final int FTP_STATE_CONNECTING = 1 << 2;
	public static final int FTP_STATE_CONNECTED = 1 << 3;
	public static final int FTP_STATE_DISCONNECTED = 1 << 4;
	public static final int FTP_STATE_UNLOGIN = 1 << 5;
	public static final int FTP_STATE_LOGINING = 1 << 6;
	public static final int FTP_STATE_LOGINED = 1 << 7;
	public static final int FTP_STATE_DOWNLOADING = 1 << 8;
	public static final int FTP_STATE_DOWNLOAD_PAUSED = 1 << 9;
	public static final int FTP_STATE_DOWNLOAD_RESUME = 1 << 10;
	public static final int FTP_STATE_DOWNLOAD_FINISHED = 1 << 11;
	public static final int FTP_STATE_ERROR = 1 << 12;
	public static final int FTP_STATE_UNKNOWN = 1 << 13;
	
	private FTPClient mFtpClient = null;
	private int mStatus = FTP_STATUS_UNKNOWN;
	private int mState = FTP_STATE_UNINITIAL;
	
	private long totalSize = 0L;
    private long downloadSize = 0L;
	
	public MyContinueFTP() {
	    mFtpClient = new FTPClient();
	    totalSize = 0L;
	    downloadSize = 0L;
	    mStatus = FTP_STATUS_UNKNOWN;
	    mState = FTP_STATE_UNINITIAL;
	}
	
	public boolean connect(String host, int port, String username, String password) {
	    try {
    	    Debug.v("on ftp connect");
    	    mState = FTP_STATE_UNCONNECT;
    		mFtpClient.connect(host, port);
    		mState = FTP_STATE_CONNECTED;
    		mFtpClient.setControlEncoding("GBK");
    		if((mFtpClient.login(username, password))){
    		    mState = FTP_STATE_LOGINED;
    		    mStatus = FTP_STATUS_OK;
    			return true;
    		} else{
    			disconnect();
    			mState = FTP_STATE_DISCONNECTED;
    			mStatus = FTP_STATUS_ERROR_LOGIN;
    			return false;
    		}
	    } catch (IOException e) {
	        e.printStackTrace();
	        mState = FTP_STATE_DISCONNECTED;
	        mStatus = FTP_STATUS_ERROR_LOGIN;
	    }
	    return false;
	}
	
	public void disconnect() throws IOException{
	    Debug.v("on ftp disconnect");
		if(mFtpClient.isConnected()){
			mFtpClient.disconnect();
			mState = FTP_STATE_DISCONNECTED;
		}
	}
	
	public void download(String uri, boolean continued) {
	    
	    if(mState == FTP_STATE_LOGINED) {
	        //download
	        try {
    	        mFtpClient.enterLocalPassiveMode();
                mFtpClient.setFileType(FTP.BINARY_FILE_TYPE);
                mFtpClient.setConnectTimeout(10000);
                
                FTPFile[] files = mFtpClient.listFiles(uri);
                if(files != null && files.length >= 1) {
                    int i = (int) files[0].getSize();
                    totalSize = i;
                    Debug.i("the files should be " + i + " bytes");
                    if(totalSize <= 1) {
                        //it may cause some fatal problems. so we should catch it and do something
                        throw new IOException("Get total Size return 0");
                    }
                    File f = new File(Utilities.SdCardRoot + Utilities.mRecoveryFileName);
                    if(f.exists()) {
                        if(continued) {
                            downloadSize = f.length();
                            if(downloadSize >= totalSize) {
                                f.delete();
                                downloadSize = 0;
                            }
                        } else {
                            f.delete();
                        }
                    }
                    if(downloadSize > 0) {
                        Debug.d("continue to download, begin at " + downloadSize);
                        mFtpClient.setRestartOffset(downloadSize);
                    }
                    mFtpClient.setBufferSize((int)(totalSize - downloadSize));
                    InputStream in = mFtpClient.retrieveFileStream(uri);
                    FileOutputStream out = new FileOutputStream(f,true);
                    
                    /*
                     * Now, we can tell state machine that we are downloading.
                     * Notice: don't tell state machine before we get the totalSize, or it will cause a divide zero error.
                     */
                    if(continued)
                        mState = FTP_STATE_DOWNLOAD_RESUME;
                    else
                        mState = FTP_STATE_DOWNLOADING;
                    
                    
                    byte[] bytes = new byte[1024 * 1024];
                    int c;
                    while((c = in.read(bytes))!= -1){
                        out.write(bytes, 0, c);
                        downloadSize += c;
                    }
                    out.flush();
                    in.close();
                    out.close();
                    Debug.d("ftp: download finished");
                    mState = FTP_STATE_DOWNLOAD_FINISHED;
                    mStatus = FTP_STATUS_OK;
                } else {
                    //uri error
                    mStatus = FTP_STATUS_ERROR_URL;
                    mState = FTP_STATE_ERROR;
                }
	        } catch (IOException e) {
                e.printStackTrace();
                if (downloadSize > 0 && downloadSize < totalSize) {
                    Debug.d("we have not finished downloading yet, we can restart any other times");
                    mState = FTP_STATE_DOWNLOAD_PAUSED;
                    mStatus = FTP_STATUS_OK;
                } else {
                    mState = FTP_STATE_ERROR;
                    mStatus = FTP_STATUS_ERROR_DOWNLOAD;
                }
            }
        }
	}

	public void pause() {
	    mState = MyContinueFTP.FTP_STATE_DOWNLOAD_PAUSED;
        mStatus = MyContinueFTP.FTP_STATUS_OK;
	}
	
	public  void resume() {
	    
	    
	}
    public int getStatus() {
        return mStatus;
    }

    public int getState() {
        return mState;
    }

    public DownloadStatus getDownloadStatus() {
        DownloadStatus dls = new DownloadStatus();
        int percent = 0;
        switch (mState) {
            case FTP_STATE_UNINITIAL:
            case FTP_STATE_UNCONNECT:
            case FTP_STATE_CONNECTING:
            case FTP_STATE_CONNECTED:
            case FTP_STATE_UNLOGIN:
            case FTP_STATE_LOGINING:
            case FTP_STATE_LOGINED:
            case FTP_STATE_UNKNOWN:
                dls.setStatus(DownloadStatus.STATUS_WAIT_TO_LOGIN);
                break;
            case FTP_STATE_DOWNLOAD_PAUSED:
                percent = (int) (downloadSize * 100 / totalSize);
                if (percent <= 1) {
                    percent = 1;
                }
                dls.setProgress(percent);
                dls.setStatus(DownloadStatus.STATUS_PAUSED);
                break;
            case FTP_STATE_DOWNLOAD_RESUME:
                /*
                 * don't tell ui any more
                 */
                mState = FTP_STATE_DOWNLOADING;
                
                percent = (int) (downloadSize * 100 / totalSize);
                if (percent <= 1) {
                    percent = 1;
                }
                dls.setProgress(percent);
                dls.setStatus(DownloadStatus.STATUS_RESUMED);
                break;
            case FTP_STATE_DOWNLOADING:
                percent = (int) (downloadSize * 100 / totalSize);
                if (percent <= 1) {
                    percent = 1;
                }
                dls.setProgress(percent);
                dls.setStatus(DownloadStatus.STATUS_RUNNING);
                break;
            case FTP_STATE_DISCONNECTED:
                dls.setStatus(DownloadStatus.STATUS_FAILED);
                
                dls.setErr(DownloadStatus.ERROR_CONNECTIVITY);
                break;
            case FTP_STATE_DOWNLOAD_FINISHED:
                dls.setStatus(DownloadStatus.STATUS_FINISHED);
                break;
            case FTP_STATE_ERROR:
                dls.setStatus(DownloadStatus.STATUS_FAILED);
                if(mStatus == FTP_STATUS_ERROR_URL)
                    dls.setErr(DownloadStatus.ERROR_FILE_NOT_FOUND);
                else
                    dls.setErr(DownloadStatus.ERROR_CONNECTIVITY);
                break;
            default:
                break;
        }
        Debug.v("ftp download state:" + mState + ", " + downloadSize + "/" + totalSize + " = "+ percent + "%");
        return dls;
    }
	
}
