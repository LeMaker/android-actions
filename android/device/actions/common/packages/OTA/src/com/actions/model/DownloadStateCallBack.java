package com.actions.model;

public interface DownloadStateCallBack {
    /*
     * public static final int STATUS_PENDING = 1 << 0;
        public static final int STATUS_RUNNING = 1 << 1;
        public static final int STATUS_PAUSED = 1 << 2;
        public static final int STATUS_FINISHED = 1 << 3;
        public static final int STATUS_FAILED = 1 << 4;
        public static final int STATUS_RESUMED = 1 << 5;
        public static final int STATUS_MD5_CHECKING = 1 << 6;
        public static final int STATUS_MD5_FINISHED = 1 << 7;
     */
    public void onDownloadPended(DownloadStatus us);
    public void onDownloadPause(DownloadStatus us);
    public void onDownloadStart();
    public void onDownloadStop();
    public void onDownloadResume(DownloadStatus us);
    public void onDownloadFinish(DownloadStatus us);
    public void onDownloadRuning(DownloadStatus us);
    public void onDownloadFailed(DownloadStatus us);
    public void onCheckMD5Start();
    public void onCheckMD5Finish(boolean passed);
    public void onDeleteOldStart();
    public void onDeleteOldFinished();
    public void showInstallDialog();
}
