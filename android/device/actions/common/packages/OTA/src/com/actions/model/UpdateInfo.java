package com.actions.model;

import com.actions.utils.Utilities;
import com.actions.utils.VersionUtils;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

public class UpdateInfo implements Serializable{
	
	private String mOldVersion;
	private String mNewVersion;
	private String mDownloadUrl;
	private String mFileSize;
	private String mMd5;
	private List<String> mUpdateMessage;
	private String mUpdateType;
	private String mSystemVersion;
	/*
	 * we do have to provide some comparable datas to make caller understand more easier.
	 */
	private long mOldVersionComp;
	private long mNewVersionComp;
	private long mFileSizeComp;
	private long mSystemVersionComp;
	
	public UpdateInfo(){
		mUpdateMessage = new ArrayList<String>();
		mSystemVersion = Utilities.mCurrentSystemVersion;
		mSystemVersionComp = VersionUtils.StringtoIntVersion(mSystemVersion);
	}
	
	public String getOldVersion() {
		return mOldVersion;
	}
	public String getNewVersion() {
		return mNewVersion;
	}	
	public void setOldVersion(String mVersion) {
		this.mOldVersion = mVersion;
		mOldVersionComp = VersionUtils.StringtoIntVersion(mVersion);
	}
	public void setNewVersion(String mVersion) {
		this.mNewVersion = mVersion;
		mNewVersionComp = VersionUtils.StringtoIntVersion(mVersion);
	}
	
	public String getMd5() {
		return mMd5;
	}
	public void setMd5(String mMd5) {
		this.mMd5 = mMd5;
	}
	
	public String getDownloadUrl() {
		return mDownloadUrl;
	}
	public void setDownloadUrl(String mDownloadUrl) {
		this.mDownloadUrl = mDownloadUrl;
	}
	
	public String getFileSize() {
		return mFileSize;
	}
	public void setFileSize(String mFileSize) {
		this.mFileSize = mFileSize;
		if(mFileSize == null || mFileSize.equals("")) {
		    mFileSizeComp = 0L;
		    return;
		} 
		try {
		    mFileSizeComp =  Long.valueOf(mFileSize).longValue();
		} catch (Exception e){
		    mFileSizeComp = 0L;
		}
	}
	
	public List<String> getUpdateMessage() {
		return mUpdateMessage;
	}
	public void setUpdateMessage(List<String> mUpdateMessage) {
		this.mUpdateMessage = mUpdateMessage;
	}
	
	public void addUpdateMessage(String msg){
		this.mUpdateMessage.add(msg);
	}
	
	public String getUpdateType() {
		return mUpdateType;
	}
	public void setUpdateType(String Type) {
		this.mUpdateType = Type;
	}

    public long getOldVersionComp() {
        return mOldVersionComp;
    }

    public void setOldVersionComp(long mOldVersionComp) {
        this.mOldVersionComp = mOldVersionComp;
    }

    public long getNewVersionComp() {
        return mNewVersionComp;
    }

    public void setNewVersionComp(long mNewVersionComp) {
        this.mNewVersionComp = mNewVersionComp;
    }

    public long getFileSizeComp() {
        return mFileSizeComp;
    }

    public void setFileSizeComp(long mFileSizeComp) {
        this.mFileSizeComp = mFileSizeComp;
    }

    public String getSystemVersion() {
        return mSystemVersion;
    }

    public void setSystemVersion(String mSystemVersion) {
        this.mSystemVersion = mSystemVersion;
    }

    public long getSystemVersionComp() {
        return mSystemVersionComp;
    }

    private void setSystemVersionComp(long mSystemVersionComp) {
        this.mSystemVersionComp = mSystemVersionComp;
    }
	
    public boolean isAvailability() {
	    
	    if ((mNewVersion == null) || (mNewVersion.equals(""))){
	        return false;
	    }
	    if ((mFileSize == null) || (mFileSize.equals(""))){
	        return false;
	    }
	    if ((mDownloadUrl == null) || (mDownloadUrl.equals(""))){
	        return false;
	    }
	    if ((mMd5 == null) || (mMd5.equals(""))){
	        return false;
	    }
	    return true;
	}
	
}
