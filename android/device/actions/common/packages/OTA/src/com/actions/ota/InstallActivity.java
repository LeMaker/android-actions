package com.actions.ota;

import android.app.Activity;
import android.os.BatteryManager;
import android.os.Bundle;

import com.actions.model.UpdateInfo;
import com.actions.ota.R;

import android.util.Log;
import android.widget.Button;
import android.widget.TextView;
import android.content.Intent;
import android.content.IntentFilter;
import com.actions.utils.FileUtils;
import com.actions.utils.Utilities;
import android.widget.Toast;
import android.view.View;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.DialogInterface;
import android.app.AlertDialog;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;

public class InstallActivity extends Activity{	
	private static final String TAG = "com.actions.ota.InstallActivity";
	private Context mContext = this;
	
	private TextView RightPromptInfo  = null ;
	private TextView LeftPromptInfo = null ;
	private TextView Progresstext  = null ;
	private Button btnOperation = null ;	
	
	private int battery;
	private int battery_level;
	private int battery_scale;
	private int charging_state;
		
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		 super.onCreate(savedInstanceState);
	     setContentView(R.layout.activity_main);	     
	     getViewObjects();
	     
	     InstallInterface();
	}
	
	
	@Override
    protected void onStart() {
        // TODO Auto-generated method stub
		Log.e(TAG, "myprint onStart");
	    registerReceiver(batteryChangedReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        super.onStart();
    }


    @Override
	protected void onStop() {		
		// unregister battery receiver
		unregisterReceiver(batteryChangedReceiver);
		super.onStop();
	}
	
	private void getViewObjects() {
		RightPromptInfo = (TextView) findViewById(R.id.RightPromptInfo);
		LeftPromptInfo = (TextView) findViewById(R.id.LeftPromptInfo);
		Progresstext = (TextView) findViewById(R.id.progresstext);
		btnOperation = (Button) findViewById(R.id.btnOperation);
	}
	
	private void InstallInterface(){
		FileUtils mFileUtils = new FileUtils();
    	if(mFileUtils.isFileExist(Utilities.mRecoveryFileName)){
    		Utilities.mIslocalDownload = true ;
    		btnOperation.setText(getString(R.string.install));
			LeftPromptInfo.setText(getString(R.string.Will_upgrade));
			Utilities.setViewDisText(RightPromptInfo, getString(R.string.update_notice)) ;
	    	Progresstext.setBackground(getResources().getDrawable(R.drawable.ic_finish_pic)) ;
    	}else{
    		btnOperation.setText(getString(R.string.Quit));
			LeftPromptInfo.setText(getString(R.string.No_update_file)) ;
			Utilities.setViewDisText(RightPromptInfo, getString(R.string.No_update_file_Prompt)) ;
	    	Progresstext.setBackground(getResources().getDrawable(R.drawable.ic_error_pic)) ;
    	}
	}
	
	public void updateOnLine(View view){
	
		
    	if(btnOperation.getText().toString().equals(getString(R.string.install))){
    	if(battery <= 50 && BatteryManager.BATTERY_PLUGGED_AC != charging_state){
    	
    			show_battery_dialog();
    		}else{
    		    update();
    		}
    	}else if(btnOperation.getText().equals(getString(R.string.Quit))){
    		finish() ;
    	}
    }
	
	private void update(){
	    // whether had update.zip in /mnt/sdcard
        FileUtils mFileUtils = new FileUtils();
        if(mFileUtils.isFileExist(Utilities.mRecoveryFileName)){
        	// enter recovery to update
        	if(Utilities.getRecoveryMode() == 1){
        		Utilities.rebootToAutoRecovery(mContext);
        	} else{
        		Utilities.rebootToRecovery(mContext);
        	}
		} else {
			//TODO:can't be here
		}
	}
	 
	private BroadcastReceiver batteryChangedReceiver = new BroadcastReceiver(){
			@Override
			public void onReceive(Context context, Intent intent) {
				if(intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)){
					battery_level = intent.getIntExtra("level", 0);
					battery_scale = intent.getIntExtra("scale", 100);
					charging_state = intent.getIntExtra(BatteryManager.EXTRA_PLUGGED, -1);
					Log.e(TAG, "myprint charging_state" + charging_state);
					battery = (battery_level*100/battery_scale);					
				}
			}			
	};
	 
	private void show_battery_dialog(){
	    	btnOperation.setText(getString(R.string.Quit));
	    	Progresstext.setBackground(getResources().getDrawable(R.drawable.ic_error_pic)) ;
	    	LeftPromptInfo.setText(getString(R.string.battery_low)) ;
			Utilities.setViewDisText(RightPromptInfo, getString(R.string.battery_notify)) ;
	}
}
