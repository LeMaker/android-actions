package com.actions.jni;

import android.util.Log;

/**
 * Description:
* deal with native pwm test
*********************************************   
*ActionsCode(author:jiangjinzhang, new_code)
* @version 1.0
*/
public class pwm {
	
private static final String TAG = "ActduinoTest.PWM jni";
	
	public native int openpwm();
	public native int sendcmdpwm(int cmd, int arg);
	public native int close();
	
	public static pwm mPWM;
	
	static{
		System.loadLibrary("actduino_test");
		Log.d(TAG,"loadLibrary!");
	}
	
	public pwm() throws Exception{
		Log.d(TAG,"open():"+openpwm());
	}	

	public static pwm newInstance() throws Exception{
		if(mPWM == null){
			mPWM = new pwm();
		}
		return mPWM;
	}
public int configrequest(int cmd, int arg) {
		
		int res=sendcmdpwm(cmd, arg);
		return res;
	}
	public int getvalue(int cmd, int arg) {
		
		int res=sendcmdpwm(cmd, arg);
		Log.d(TAG,"getvalue():res="+res);
		return res;
	}

}
