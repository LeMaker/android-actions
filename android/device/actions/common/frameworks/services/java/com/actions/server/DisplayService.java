package com.actions.server;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import android.util.Log;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.IBinder;
import android.os.PowerManager;
import android.os.SystemProperties;
import android.os.RemoteException;
import android.provider.Settings;
import android.view.WindowManagerPolicy;
import com.actions.hardware.IDisplayService;
import com.actions.hardware.ICableStatusListener;

//import com.android.server.InputManager;
/**
 * The implementation of the display service.
 * <p>
 * This implementation focuses on manage the tvout and lcd display. Most methods
 * are synchronous to external calls.
 * 
 * @author Actions
 * @version V2.0
 */
public class DisplayService extends IDisplayService.Stub {

    static {
        System.loadLibrary("actions_runtime");
    }

    private static final String TAG = "DisplayService";
    
    /**
     * Binder context for this service
     */
    private Context mContext;
    
    private ContentResolver mContentResolver;
  
    
    /**
     * Constructs a new DisplayService instance.
     * 
     * @param context
     *            Binder context for this service
     */
    public DisplayService(Context context) {
        Log.d(TAG, "DisplayService()");
        mContext = context;
        mContentResolver = context.getContentResolver();
        _init();

    }
    
    public boolean getHdmiEnable() {
        return _getHdmiEnable();
     }
    
    public void setHdmiEnable(boolean enable) { // Alicia add 2013.08.01
        Log.d(TAG, "enableHdmi enable:" + enable);
        _setHdmiEnable(enable);
    }
    
    public boolean setHdmiMode(String mode) {
        Log.d(TAG, "setMode: " + mode);
        return _setHdmiMode(mode);
    }
    
    
    public String getHdmiMode() {        
        String mode = _getHdmiMode();
        Log.d(TAG, "getMode" + mode);
        return mode;
    }
    
    public String[] getHdmiSupportedModesList(){
        Log.d(TAG,"getSupportedModesList");
        return _getHdmiSupportedModesList();
    }
    
    
    public void setHdmiViewFrameSize(int dx, int dy) {
        Log.d(TAG, "setHdmiViewFrameSize dx: " + dx + " dy: " + dy);
        _setHdmiViewFrameSize(dx, dy);
    }
    
    public void getHdmiViewFrameSize(int [] dx_dy) {       
        _getHdmiViewFrameSize(dx_dy);
        Log.d(TAG, "getHdmiViewFrameSize dx: " + dx_dy[0] + " dy: " + dx_dy[1]);
    }

	
    /**
     * get hdmi and cvbs's cable state.
     * 
     * @return cable state value.
     */
    public boolean getHdmiCableState() {
    	Log.d(TAG, "getHdmiCableState ");
        synchronized (this) {
            return _getHdmiCableState();
        }
    }

    public boolean setHdmiFitScreen(int value){
        Log.d(TAG, "setDisFitScreen value: " + value);
        return _setHdmiFitScreen(value);
    }

    public int getHdmiFitScreen(){
        Log.d(TAG,"getDisFitScreen");
        return _getHdmiFitScreen();
    }
    
    public int setDisplayInfo(int display , int info[]){
    	return _setDisplayInfo(display,info,0);
    }
    
    public int getDisplayInfo(int display , int info[]){
    	return _getDisplayInfo(display,info,0);
    }
    
    private static native boolean _init();
    
    private native final void _setHdmiEnable(boolean enable);

    private native final boolean _getHdmiEnable();           
    
    private native final boolean _setHdmiMode(String mode);

    private native final String _getHdmiMode();
    
    private native final String[] _getHdmiSupportedModesList();
    
    private native final void _setHdmiViewFrameSize(int dx, int dy);
    
    private native final int _getHdmiViewFrameSize(int[] dx_dy);
    
    private native final boolean _getHdmiCableState();    

    private native final boolean _setHdmiFitScreen(int value);

    private native final int _getHdmiFitScreen();

    private native int _getDisplayInfo(int display , int info[],int offset);
    
    private native int _setDisplayInfo(int display , int info[],int offset);

}
