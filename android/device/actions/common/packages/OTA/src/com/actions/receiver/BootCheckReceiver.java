package com.actions.receiver;

import java.util.Calendar;
import android.content.SharedPreferences.Editor;
import android.content.SharedPreferences;

import com.actions.utils.Utilities;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import java.util.Date;
import java.text.SimpleDateFormat;



public class BootCheckReceiver extends BroadcastReceiver {

	private static boolean rebootShallCheck = false;
	private long CHECK_REPEATE_TIME = 86400000;	
	
	SharedPreferences preferences ;
	SharedPreferences.Editor editor ;
	Context mContext = null ;
	
	@Override
	public void onReceive(Context context, Intent intent) {		
		String mAction = intent.getAction();
		mContext = context ;
		SaveUpdateTime() ;
		if(mAction.equals(Utilities.BootReceiver)){
			rebootShallCheck = true;
			dailyCheck(context);
		}
		
		if((mAction.equals(Utilities.ConnectivityReceiver)) 
				&& (Utilities.checkConnectivity(context)) 
				&& (rebootShallCheck)){
			rebootShallCheck = false;
			Intent mCheckIntent = new Intent(Utilities.UpdateCheckReceiver);
			mCheckIntent.putExtra("TYPE", "BOOT");
			context.sendBroadcast(mCheckIntent);
		}
	}
	
	public PendingIntent getPendingIntent(Context mContext){
		Intent pendingIntent = new Intent(Utilities.UpdateCheckReceiver);
		pendingIntent.putExtra("TYPE", "DAILY");
		return PendingIntent.getBroadcast(mContext, 0, pendingIntent, 0);
	}
	
	public void dailyCheck(Context mContext){
		AlarmManager mAlarmManager = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);
		
		// initialize the Calendar and set calendar time
		Calendar mCalendar = Calendar.getInstance();
		mCalendar.setTimeInMillis(System.currentTimeMillis());
		
		PendingIntent mPendingIntent = getPendingIntent(mContext);
		mAlarmManager.setRepeating(AlarmManager.RTC, 
				mCalendar.getTimeInMillis(), this.CHECK_REPEATE_TIME, 
				mPendingIntent);
	}

	void SaveUpdateTime(){
		preferences = mContext.getApplicationContext().getSharedPreferences("time", mContext.MODE_PRIVATE) ;
		editor = preferences.edit() ;
		String time = preferences.getString("time", null) ;
		if(time == null){
			SimpleDateFormat sdf = new SimpleDateFormat("yyyy.MM.dd "+ "HH:mm:ss") ;
			editor.putString("time", sdf.format(new Date())) ;
			editor.commit() ;
		}
	}
}
