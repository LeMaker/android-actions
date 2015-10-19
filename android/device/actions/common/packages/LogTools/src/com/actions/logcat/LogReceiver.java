
package com.actions.logcat;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class LogReceiver extends BroadcastReceiver {
	
	protected static final String TAG = "LogReceiver";
	public static final String ACTIONS_START_LOG = "logcat.actions.START_LOG";
	public static final String ACTIONS_STOP_LOG = "logcat.actions.STOP_LOG";

	static final int START_LOG_SERVICE = 0x100;
	static final int STOP_LOG_SERVICE = 0x101;
	
	private boolean logStared = false;
	private int mode;
	
	LogModel mModel;
	
	private void startLog(Context context, int op){
		if(mode == LogModel.SERVICE_MODE) {
			if(op == START_LOG_SERVICE) {
				mModel.creatLogFile(mModel.getDefaultName());
				mModel.startLogcatService();
			}
			else {
				mModel.stopLogcatService();
			}
		}
		else {
			if(op == START_LOG_SERVICE){
			}
			else{
			}
		}
	}
	
	public void onReceive(Context context, Intent intent) {
		String action = intent.getAction();
		mModel = new LogModel(context);
		mode = mModel.getServiceMode();
		
		if(mode == LogModel.SERVICE_MODE)
			logStared = mModel.processIsExist(LogModel.logcatServiceBinLocation);
		else
			logStared = mModel.processIsExist(LogModel.logcatBinLocation);
		
		if(action.equals(ACTIONS_START_LOG)){
			if(logStared){
				Log.d(TAG, "logservice is running now");
				return;
			}
			Log.d(TAG, "ACTION_START_LOG");
			startLog(context, START_LOG_SERVICE);
		}
		else if(action.equals(ACTIONS_STOP_LOG)){
			if(logStared==false){
				Log.d(TAG, "logservice is stopped now");
				return;
			}
			Log.d(TAG, "ACTION_STOP_LOG");
			startLog(context, STOP_LOG_SERVICE);
		} 
	}
}