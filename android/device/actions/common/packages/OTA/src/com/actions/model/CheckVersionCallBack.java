package com.actions.model;

public interface CheckVersionCallBack {

    /*
     * public static final int RESULT_CONNECTIVITY_ERROR = 1 << 0;
    public static final int RESULT_SERVER_NOT_FOUND = 1 << 1;
    public static final int RESULT_CONTENT_NOT_FOUND = 1 << 2;
    public static final int RESULT_UP_TO_DATE = 1 << 3;
    public static final int RESULT_UNKNOW = 1 << 4;
    public static final int RESULT_UPDATE_NEEDED = 1 << 5;
    public static final int RESULT_CHECK_NOT_READY = 1 << 6;
     */
    public void onCVError(CheckResult cr);
    public void onCVServerNotFound(CheckResult cr);
    public void onCVContentNotFound(CheckResult cr);
    public void onCVUpToDate(CheckResult cr);
    public void onCVUpdateNeeded(CheckResult cr);
    public void onCVCheckNotReady(CheckResult cr);
    public void onCVUnknown(CheckResult cr);
    public void onCVStart();
}
