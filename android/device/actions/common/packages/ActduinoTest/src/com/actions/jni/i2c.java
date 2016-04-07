package com.actions.jni;


import android.util.Log;

/**
 * Description:
* deal with native i2c test
*********************************************   
*ActionsCode(author:jiangjinzhang, new_code)
* @version 1.0
*/
public class i2c {
	private static final String TAG = "ActduinoTest.I2C jni";
	
	//when the I2C_CNT = 4

	
	public native int openi2c(int key);
	public native int close();
	public native int writei2c(String res,int size);
	public native String readi2c(int size);
	public native int setmodei2c();
	public native int test();
	public native String stringFromJNI();
	
	public static i2c mI2C;
	
	static{
		System.loadLibrary("actduino_test");
		Log.d(TAG,"loadLibrary!");
	}
	
	public i2c(int key) throws Exception{
		Log.d(TAG,"openi2c():"+openi2c(key));
	}	

	public static i2c newInstance(int key) throws Exception{
		if(mI2C == null){
			mI2C = new i2c(key);
		}
		return mI2C;
	}
}
