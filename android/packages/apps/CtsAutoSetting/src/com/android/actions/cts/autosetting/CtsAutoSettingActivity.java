package com.android.actions.cts.autosetting;


import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import android.provider.*;
import android.app.KeyguardManager;
import android.app.admin.DevicePolicyManager;
import com.android.internal.widget.LockPatternUtils;
import android.view.WindowManager.LayoutParams;
import android.content.pm.PackageManager;
import android.os.Bundle;

import com.android.actions.cts.autosetting.WifiConnect;
import com.android.actions.cts.autosetting.GoogleSettingsContract;


public class CtsAutoSettingActivity extends Activity {
	
	private final String TAG = "ctsAutoSetting";
	
	//String SSID="ACTS_IPV6_2.4G"; 
	//String Password="ipv68888";
	String SSID="ACTZH-Pad-ToHK"; 
	String Password="HK99999999";
	
	/*
	 * wifi connect strategy:indentify a default AP and password in the code.
	 * If they are right,wifi will connected.If they are wrong,wifi will not connect,but It doesn't matter.
	 * When we try to connect WiFi,it will generate wpa_supplicant.conf in /data/misc/wifi.
	 * And then the autoTest script can pull supplicant.conf out(we must pull out,because it contain the model/manufacturer and USB serial,it is deferent in deferent device)
	 * and then the autoTest script modify wpa_supplicant.conf,add the correct AP and password.
	 * network={
	     ssid="ACTZH-Pad-ToHK"
	     psk="HK99999999"
	     key_mgmt=WPA-PSK
	     priority=2   
       }
     Note:priority must >1,because priority=1 when the first time wifi connect.
	 * WiFi will connected after reboot.
	 */
	
	//int screen_timeout = -1;
	
	private Button mButtonAutoSet;
	private Button mButtonDebug;
	
	/** Broadcast intent action when the location mode is about to change. */
    //private static final String MODE_CHANGING_ACTION ="com.android.settings.location.MODE_CHANGING";
    //private static final String CURRENT_MODE_KEY = "CURRENT_MODE";
    //private static final String NEW_MODE_KEY = "NEW_MODE";
    
    //private ChooseLockSettingsHelper mChooseLockSettingsHelper;
    
    private LockPatternUtils mLockPatternUtils;
    
    DevicePolicyManager mDPM;
	
	private void setpProvision(){
		Log.i(TAG, "setpProvision..." );
	    // Add a persistent setting to allow other apps to know the device has been provisioned.
        Settings.Global.putInt(getContentResolver(), Settings.Global.DEVICE_PROVISIONED, 1);
        Settings.Secure.putInt(getContentResolver(), Settings.Secure.USER_SETUP_COMPLETE, 1);
        Log.i(TAG, "setpProvision...finished" );
	}
	
	private void connectWifi() {
		Log.i(TAG, "connectWifi..." );
		final WifiManager wifiManager =
            (WifiManager) getSystemService(Context.WIFI_SERVICE);
        if (wifiManager == null) {
            Log.d(TAG, "No wifiManager.");
            return;
        }
		WifiConnect mWifiConnect = new WifiConnect(wifiManager);
		
		
		boolean ret = mWifiConnect.Connect(SSID, Password, WifiConnect.WifiCipherType.WIFICIPHER_WPA);
		
		// show info
		//Toast.makeText(CtsAutoSettingActivity.this, "connectWifi...finished", Toast.LENGTH_LONG).show();
		
		Log.i(TAG, "connectWifi...finished"+ret );
	}
	
	private void setDisplaySleep() {
		Log.i(TAG, "setDisplaySleep..." );
		int time_out_value = 2147483647;
		if(Settings.System.getInt(getContentResolver(),Settings.System.SCREEN_OFF_TIMEOUT, 0) != time_out_value)
		{
	
			boolean setRet = Settings.System.putInt(getContentResolver(),Settings.System.SCREEN_OFF_TIMEOUT, time_out_value);
		} 
		int getRet = Settings.System.getInt(getContentResolver(),Settings.System.SCREEN_OFF_TIMEOUT, 0);
		Log.i(TAG, "SCREEN_OFF_TIMEOUT:"+getRet );
		// show info
		//Toast.makeText(CtsAutoSettingActivity.this, "setDisplaySleep:"+getRet, Toast.LENGTH_LONG).show();
		
		Log.i(TAG, "setDisplaySleep finished " );
	
	}
	private void setScreenLock1() {
		//*****************this method:device will locked after reboot**************
		KeyguardManager keyguardManager=(KeyguardManager )getSystemService(Context.KEYGUARD_SERVICE);
		String lockTag=this.getLocalClassName();
		KeyguardManager.OnKeyguardExitResult keyguardExitResultListener =new KeyguardManager.OnKeyguardExitResult()
		{
			@Override
			public void onKeyguardExitResult(boolean success) {
			// TODO Auto-generated method stub
			if(success)
				Log.i(TAG,"successfull to do Keyguard exit");
			else
				Log.i(TAG,"fail to do Keyguard exit");
			}
		};
		keyguardManager.exitKeyguardSecurely(keyguardExitResultListener);
		KeyguardManager.KeyguardLock keyguardLock=keyguardManager.newKeyguardLock(lockTag);
		for(int i=10;i>0;i--)
		{
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			Log.i(TAG,"We will unlock the screen in "+i+" seconds");
		}
		keyguardLock.disableKeyguard();
		/*
		for(int i=20;i>0;i--)
		{
		try {
		Thread.sleep(1000);
		} catch (InterruptedException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
		}
		Log.i(TAG,"We will roll back the state of Keyguard in "+i+" seconds");
		}
		long t=System.currentTimeMillis();
		keyguardLock.reenableKeyguard();
		Log.i(TAG,"time:"+(System.currentTimeMillis()-t));
		*/
		Log.i(TAG, "setScreenLock finished" );
	
	}
	private void setScreenLock() {
		Log.i(TAG, "setScreenLock..." );
		
		
		
		mLockPatternUtils.clearLock(false);
		mLockPatternUtils.setLockScreenDisabled(true);
		
		/*
		 //*************this is a bug in android 4.3 and below,4.4 has fixed it****** 
		Intent intent = new Intent();
        intent.setComponent(new ComponentName("com.android.settings", "com.android.settings.ChooseLockGeneric")); //start ChooseLockGeneric"
        intent.putExtra("confirm_credentials", false);  //confirm_credentials is false,mPasswordConfirmed is true
        intent.putExtra("lockscreen.password_type",0);  //lockscreen.password_type=0:ASSWORD_QUALITY_UNSPECIFIED
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
        */
		Log.i(TAG, "setScreenLock finished " );
	}
	
	/*
	private void setLauguage() {
		Log.i(TAG, "setLauguage..." );
		
		boolean setRet = Settings.Secure.putInt(getContentResolver(),Settings.Secure.LOCK_PATTERN_ENABLED, 0);
		Log.i(TAG, "setLauguage setRet= "+ setRet);
		
		int getRet = Settings.Secure.getInt(getContentResolver(),Settings.Secure.LOCK_PATTERN_ENABLED, 0);
		Log.i(TAG, "LOCK_PATTERN_ENABLED:"+getRet );
		// show info
		//Toast.makeText(CtsAutoSettingActivity.this, "setLauguage:"+getRet, Toast.LENGTH_LONG).show();
		Log.i(TAG, "setLauguage finished " );
	}
	*/
	
	
	private void enableDeveloperOptions() {
		Log.i(TAG, "enableDeveloperOptions..." );
		int state = 1;
		if(Settings.Global.getInt(getContentResolver(),Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 0) != state)
		{
	
			boolean setRet = Settings.Global.putInt(getContentResolver(),Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, state);
		} 
		int getRet = Settings.Global.getInt(getContentResolver(),Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, 0);
		Log.i(TAG, "DEVELOPMENT_SETTINGS_ENABLED:"+getRet );
		// show info
		//Toast.makeText(CtsAutoSettingActivity.this, "setDeveloperOptions:"+getRet, Toast.LENGTH_LONG).show();
		
		Log.i(TAG, "setDeveloperOptions finished " );
	}
	private void enableADBDebugging() {
		Log.i(TAG, "enableADBDebugging..." );
		int state = 1;
		if(Settings.Global.getInt(getContentResolver(),Settings.Global.ADB_ENABLED, 0) != state)
		{
	
			boolean setRet = Settings.Global.putInt(getContentResolver(),Settings.Global.ADB_ENABLED, state);
		} 
		int getRet = Settings.Global.getInt(getContentResolver(),Settings.Global.ADB_ENABLED, 0);
		Log.i(TAG, "ADB_ENABLED:"+getRet );
		// show info
		//Toast.makeText(CtsAutoSettingActivity.this, "enableADBDebugging:"+getRet, Toast.LENGTH_LONG).show();
		
		Log.i(TAG, "enableADBDebugging finished " );
	}
	
	private void setStayAwake() {
		Log.i(TAG, "setStayAwake..." );
		int state = 3;
		if(Settings.Global.getInt(getContentResolver(),Settings.Global.STAY_ON_WHILE_PLUGGED_IN, 0) != state)
		{
	
			boolean setRet = Settings.Global.putInt(getContentResolver(),Settings.Global.STAY_ON_WHILE_PLUGGED_IN, state);
		} 
		int getRet = Settings.Global.getInt(getContentResolver(),Settings.Global.STAY_ON_WHILE_PLUGGED_IN, 0);
		Log.i(TAG, "SCREEN_OFF_TIMEOUT:"+getRet );
		// show info
		//Toast.makeText(CtsAutoSettingActivity.this, "setStayAwake:"+getRet, Toast.LENGTH_LONG).show();
		
		Log.i(TAG, "setStayAwake finished " );
	}
	
	
	
	private void setAllowMockLocation() {
		Log.i(TAG, "setAllowMockLocation..." );
		if(Settings.Secure.getInt(getContentResolver(),Settings.Secure.ALLOW_MOCK_LOCATION, 0)==0)
		{
			boolean setRet = Settings.Secure.putInt(getContentResolver(),Settings.Secure.ALLOW_MOCK_LOCATION, 1);
		}
		else
		{
			//boolean setRet = Settings.Secure.putInt(getContentResolver(),Settings.Secure.ALLOW_MOCK_LOCATION, 0);
			
		}
		
		int getRet = Settings.Secure.getInt(getContentResolver(),Settings.Secure.ALLOW_MOCK_LOCATION, 0);
		Log.i(TAG, "ALLOW_MOCK_LOCATION:"+getRet );
		// show info
		//Toast.makeText(CtsAutoSettingActivity.this, "setAllowMockLocation:"+getRet, Toast.LENGTH_LONG).show();
		Log.i(TAG, "setAllowMockLocation finished " );
	}
	
	private void getAllowMockLocation() {
		Log.i(TAG, "setAllowMockLocation..." );
		
		int ret = Settings.Secure.getInt(getContentResolver(),Settings.Secure.ALLOW_MOCK_LOCATION, 0);
		Log.i(TAG, "ALLOW_MOCK_LOCATION:"+ret );
		// show info
		//Toast.makeText(CtsAutoSettingActivity.this, "getAllowMockLocation:"+ret, Toast.LENGTH_LONG).show();
		Log.i(TAG, "getAllowMockLocation finished " );
	}
	
	private void setLocationMode() {
		Log.i(TAG, "setLocationMode..." );
		
		GoogleSettingsContract.Partner.putString(getContentResolver(), "network_location_opt_in", "1");
		
        Settings.Global.putInt(getContentResolver(), "wifi_scan_always_enabled", 1);
        Settings.Secure.setLocationProviderEnabled(getContentResolver(), "network", true);
        
        //Toast.makeText(CtsAutoSettingActivity.this, "setLocationMode finished ...", Toast.LENGTH_LONG).show();
		Log.i(TAG, "setLocationMode finished ..." );
	}
	
	
	private void addDeviceAdmin() {
		Log.i(TAG, "addDeviceAdmin..." );
		ComponentName ComponentName1 = new ComponentName("android.deviceadmin.cts",
               "android.deviceadmin.cts.CtsDeviceAdminReceiver");
		Log.i(TAG, "ComponentName1:"+ComponentName1 );
		mDPM.setActiveAdmin(ComponentName1, false);
		
		ComponentName ComponentName2 = new ComponentName("android.deviceadmin.cts",
	               "android.deviceadmin.cts.CtsDeviceAdminReceiver2");
		Log.i(TAG, "ComponentName2:"+ComponentName2 );
		mDPM.setActiveAdmin(ComponentName2, false);
		
		Log.i(TAG, "addDeviceAdmin finished " );
	}
	
	
	
	private void autoSet() {
		Log.i(TAG, "autoSet..." );
		setpProvision();
		connectWifi();
		
		setDisplaySleep();
		setScreenLock();
		enableDeveloperOptions();
		enableADBDebugging();
		setStayAwake();
		setAllowMockLocation();
		setLocationMode();
		addDeviceAdmin();
		
		Log.i(TAG, "autoSet finished!" );
		
	}
	
	private View.OnClickListener mBtnAutoSetListener = new View.OnClickListener() {

		public void onClick(View v) {
			Log.i(TAG, "mBtnAutoSet -- onClick..." );
			mButtonAutoSet.setClickable(false);
			autoSet();
			mButtonAutoSet.setClickable(true);
		}
	};
	
	private View.OnClickListener mBtnDebugListener = new View.OnClickListener() {

		public void onClick(View v) {
			Log.i(TAG, "mBtnDebug -- onClick..." );
			mButtonDebug.setClickable(false);
			setAllowMockLocation();
			mButtonDebug.setClickable(true);
		}
	};
	
	
	private void findViews() {
		mButtonAutoSet = (Button) findViewById(R.id.buttonAutoSet);	
	
		mButtonDebug = (Button) findViewById(R.id.buttonDebug);	
		
	}
	
	
	private void setListensers() {
		mButtonAutoSet.setOnClickListener(mBtnAutoSetListener);	
		mButtonDebug.setOnClickListener(mBtnDebugListener);	
		
	}
	
	private void killMyself() {

	
    // remove this activity from the package manager.
    PackageManager pm = getPackageManager();
    ComponentName name = new ComponentName(this, CtsAutoSettingActivity.class);
    pm.setComponentEnabledSetting(name, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
            PackageManager.DONT_KILL_APP);

    // terminate the activity.
    finish(); 
    
	}
	
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
    	
        super.onCreate(savedInstanceState);
        Log.i(TAG, "onCreate..." );
        
        /*
         //display a window,it is suit for develop mode
        setContentView(R.layout.main);
        findViews();
        setListensers();
        */
        
        mLockPatternUtils = new LockPatternUtils(CtsAutoSettingActivity.this);
        mDPM = (DevicePolicyManager)getSystemService(Context.DEVICE_POLICY_SERVICE);
        
        //autoSet and then kill itself
        autoSet();
        killMyself();
        
    }
  
    
    protected void onDestroy() {
    	Log.i(TAG, "onDestroy..." );
		super.onDestroy();
	}

	protected void onPause() {
		Log.i(TAG, "onPause..." );
		super.onPause();
	}

	protected void onResume() {
		Log.i(TAG, "onResume..." );
		super.onResume();
	}

}