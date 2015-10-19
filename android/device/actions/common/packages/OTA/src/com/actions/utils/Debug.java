package com.actions.utils;

import android.util.Log;

public class Debug {
    public static final String TAG = "ota";
    public static final int DEBUG_LEVEL = 0;
    public static final int level = 0;
    
    public static void v(String msg) {
        i(TAG, msg);
    }
    public static void v(String tag, String msg) {
        if(level <= DEBUG_LEVEL + 0)
            Log.i(tag, msg);
    }
    
    public static void i(String msg) {
        i(TAG, msg);
    }
    public static void i(String tag, String msg) {
        if(level <= DEBUG_LEVEL + 1)
            Log.i(tag, msg);
    }
    
    public static void d(String msg) {
        i(TAG, msg);
    }
    public static void d(String tag, String msg) {
        if(level <= DEBUG_LEVEL + 2)
            Log.d(tag, msg);
    }
    
    public static void w(String msg) {
        i(TAG, msg);
    }
    public static void w(String tag, String msg) {
        if(level <= DEBUG_LEVEL + 3)
            Log.w(tag, msg);
    }
    
    public static void e(String msg) {
        i(TAG, msg);
    }
    public static void e(String tag, String msg) {
        if(level <= DEBUG_LEVEL + 4)
            Log.i(tag, msg);
    }
    
}
