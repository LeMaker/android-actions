package com.actions.jni;

import java.io.FileDescriptor;

import android.util.Log;

/**
 * Description:
* deal with native uart test
*********************************************   
*ActionsCode(author:jiangjinzhang, new_code)
* @version 1.0
*/
public class uart {

	private static final String TAG = "ActduinoTest.UART jni";


	
	public native int openuart(int key);
	public native static FileDescriptor openuart11 (int key, int nSpeed, int nBits, char nEvent, int nStop);
	public native int closeuart();
	public native static void close();
	public native static void close11();
	public native int writeuart(String res,int size);
	public native String readuart(int size);
	public native static int setmodeuart(int nSpeed, int nBits, char nEvent, int nStop);
	
	private static uart mUART;
	
	
	static{
		System.loadLibrary("actduino_test");
		Log.d(TAG,"loadLibrary!");
	}
	
	public uart() throws Exception{
		Log.d(TAG,"openuart():"+openuart(0));
	}	

	public static uart newInstance() throws Exception{
		if(mUART == null){
			mUART = new uart();
		}
		return mUART;
	}
}