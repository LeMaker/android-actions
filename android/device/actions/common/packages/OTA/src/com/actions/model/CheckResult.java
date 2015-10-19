package com.actions.model;

public class CheckResult {
    public static final int RESULT_CONNECTIVITY_ERROR = 1 << 0;
    public static final int RESULT_SERVER_NOT_FOUND = 1 << 1;
    public static final int RESULT_CONTENT_NOT_FOUND = 1 << 2;
    public static final int RESULT_UP_TO_DATE = 1 << 3;
    public static final int RESULT_UNKNOW = 1 << 4;
    public static final int RESULT_UPDATE_NEEDED = 1 << 5;
    public static final int RESULT_CHECK_NOT_READY = 1 << 6;
    
    public UpdateInfo mUpdateInfo;
    private int result;
    public int getResult() {
        return result;
    }
    public void setResult(int result) {
        this.result = result;
    }
    
}
