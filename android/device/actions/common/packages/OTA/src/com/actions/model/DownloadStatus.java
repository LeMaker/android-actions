
package com.actions.model;

import java.io.Serializable;

public class DownloadStatus implements Serializable {
    /**
     * 
     */
    private static final long serialVersionUID = -5634926679532639517L;

    public static final int STATUS_PENDING = 1 << 0;
    public static final int STATUS_RUNNING = 1 << 1;
    public static final int STATUS_PAUSED = 1 << 2;
    public static final int STATUS_FINISHED = 1 << 3;
    public static final int STATUS_FAILED = 1 << 4;
    public static final int STATUS_RESUMED = 1 << 5;
    public static final int STATUS_MD5_CHECKING = 1 << 6;
    public static final int STATUS_MD5_FINISHED = 1 << 7;
    public static final int STATUS_WAIT_TO_LOGIN = 1 << 8;
    public static final int STATUS_UNKNOWN = 1 << 9;
    
    public static final int ERROR_CONNECTIVITY = 1 << 0;
    public static final int ERROR_NO_SPACE = 1 << 1;
    public static final int ERROR_FILE_NOT_FOUND = 1 << 2;
    public static final int ERROR_UNKNOWN = 1 << 3;

    private int status;
    private int err;
    private String info;
    private int progress;

    public int getStatus() {
        return status;
    }

    public void setStatus(int status) {
        this.status = status;
    }

    public int getErr() {
        return err;
    }

    public void setErr(int err) {
        this.err = err;
    }

    public String getInfo() {
        return info;
    }

    public void setInfo(String info) {
        this.info = info;
    }

    public int getProgress() {
        return progress;
    }

    public void setProgress(int progress) {
        this.progress = progress;
    }

    public void copyFrom(DownloadStatus us) {
        this.setStatus(us.getStatus());
        this.setInfo(us.getInfo());
        this.setErr(us.getErr());
        this.setProgress(us.getProgress());
    }

    public DownloadStatus() {
        // TODO Auto-generated constructor stub
        this(STATUS_UNKNOWN);
    }

    public DownloadStatus(int status) {
        this(status, null);
    }

    public DownloadStatus(int status, String info) {
        this.status = status;
        this.info = info;
    }
}
