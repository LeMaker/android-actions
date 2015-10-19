package com.actions.download;

import com.actions.model.CheckResult;
import com.actions.model.DownloadStatus;
import com.actions.model.UpdateInfo;

public interface DownloadHelper {
    abstract public UpdateInfo getUpdateInfo();
    abstract long getDownloadSize();
    abstract long getTotalSize();
    abstract DownloadStatus getDownloadStatus();
    abstract void download(String uri);
    abstract void cancelOld();
    abstract void stop();
    abstract void pause();
    abstract void resume();
    abstract CheckResult check();
    
}
    
