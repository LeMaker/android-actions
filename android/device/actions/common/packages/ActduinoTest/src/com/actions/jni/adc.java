package com.actions.jni;

import android.util.Log;

/**
 * Description:
* deal with native adc test
*********************************************   
*ActionsCode(author:jiangjinzhang, new_code)
* @version 1.0
*/
public class adc {
	
private static final String TAG = "ActduinoTest.ADC jni";
	
	public native int openadc();
	public native int sendcmdadc(int cmd, int arg);
	public native int close();
	public native int readadccase();
	
	public static adc mADC;
	
	static{
		System.loadLibrary("actduino_test");
		Log.d(TAG,"loadLibrary!");
	}
	
	public adc() throws Exception{
		Log.d(TAG,"open():"+openadc());
	}	

	public static adc newInstance() throws Exception{
		if(mADC == null){
			mADC = new adc();
		}
		return mADC;
	}
public int configrequest(int cmd, int arg) {
		
		int res=sendcmdadc(cmd, arg);
		return res;
	}
	public int getvalue(int cmd, int arg) {
		
		int res=sendcmdadc(cmd, arg);
		Log.d(TAG,"getvalue():res="+res);
		return res;
	}

}
