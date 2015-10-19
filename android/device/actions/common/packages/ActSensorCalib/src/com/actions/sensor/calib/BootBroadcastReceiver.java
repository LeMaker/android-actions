package com.actions.sensor.calib;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class BootBroadcastReceiver extends BroadcastReceiver {

	private static final String TAG = "BootBroadcastReceiver";
	private static final String ACTION = "android.intent.action.BOOT_COMPLETED";
	
	@Override
	public void onReceive(Context context, Intent intent) {
		// TODO Auto-generated method stub
		if (intent.getAction().equals(ACTION)){
			int i = 0;
			
			if (gSensorActivity.checkIfMIR3DA() == null){
				Log.d(TAG, "it seems gsensor is not mir3da");
				return;
			}
			
			for (i = 0; i < gSensorActivity.cali_file_path.length; i++){
				if (gSensorActivity.fileIsExists(gSensorActivity.cali_file_path[i])){
					break;
				}
			}
			
			if (i == gSensorActivity.cali_file_path.length){
				//file not found
				Log.d(TAG, "Calibrate file: " + gSensorActivity.cali_file_path + " not exist");
				Intent nintent=new Intent(context,gSensorActivity.class);
				nintent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				context.startActivity(nintent);
			}else{
				Log.d(TAG, "Calibrate file: " + gSensorActivity.cali_file_path[i] + " already exist");
			}
			
			/*
			if (!gSensorActivity.fileIsExists(gSensorActivity.cali_file_path)){
				Log.d(TAG, "Calibrate file: " + gSensorActivity.cali_file_path + " not exist");
				Intent nintent=new Intent(context,gSensorActivity.class);
				nintent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
				context.startActivity(nintent);
			}else{
				Log.d(TAG, "Calibrate file: " + gSensorActivity.cali_file_path + " already exist");
			}
			*/
		}
	}	
}
