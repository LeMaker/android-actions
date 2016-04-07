package com.actions.jni;

import android.util.Log;

/**
 * Description:
* deal with native spi test
*********************************************   
*ActionsCode(author:jiangjinzhang, new_code)
* @version 1.0
*/
public class spi {
	private static final String TAG = "ActduinoTest.SPI jni";
	public native int openspi(int key);
	public native int close();
	public native int sendcmdspi(int cmd, int arg);
	public native int writespi(String res,int size);
	public native String readspi(int size);
	public native int setmode();
	
	
public static spi mSPI;
	
	static{
		System.loadLibrary("actduino_test");
		Log.d(TAG,"loadLibrary!");
	}
	public spi(int key) throws Exception{
		Log.d(TAG,"open():"+openspi(key));
	}
	public static spi newInstance(int key) throws Exception{
		if(mSPI == null){
			mSPI = new spi(key);
		}
		return mSPI;
	}
	public int  configSPI(String s){
		Log.d(TAG,"configSPI");	
		int ret = setmode();

		return ret;
	}
	
	public String  testSPI(String s){
		Log.d(TAG,"testSPI");		
		int ret=writespi(s,s.length());
		if (ret<0) {
			Log.d(TAG,"writespi fail!");
		}
		String des=readspi(s.length());
		System.out.println(""+des);
		return des;
	}

}
