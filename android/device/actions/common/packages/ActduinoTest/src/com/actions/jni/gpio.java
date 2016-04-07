package com.actions.jni;

import android.util.Log;

/**
  * Description:
 * deal with native gpio test
 *********************************************   
 *ActionsCode(author:jiangjinzhang, new_code)
 * @version 1.0
 */


public class gpio {
	private static final String TAG = "ActduinoTest.GPIO jni";
	private static final int CMD_SET_GPIOA_OUT = 0;
	private static final int CMD_SET_GPIOB_OUT = 1;
	private static final int CMD_SET_GPIOC_OUT = 20;
	private static final int CMD_SET_GPIOD_OUT = 3;
	private static final int CMD_SET_GPIOE_OUT = 4;


	private static final int CMD_SET_GPIOA_IN = 5;
	private static final int CMD_SET_GPIOB_IN = 6;
	private static final int CMD_SET_GPIOC_IN = 7;
	private static final int CMD_SET_GPIOD_IN = 8;
	private static final int CMD_SET_GPIOE_IN = 9;

	private static final int CMD_INIT = 10;
	private static final int CMD_RELEASE = 11;	
	private static final int CMD_START_TEST = 12;
	private static final int CMD_SET_ON_OFF  =13;
	private static final int CMD_READ_ON_OFF  =14;
	
	public native int open();
	public native int sendcmd(int cmd, int arg);
	public native int close();
	
	public static gpio mGPIO;
	
	static{
		System.loadLibrary("actduino_test");
		Log.d(TAG,"loadLibrary!");
	}
	
	public gpio() throws Exception{
		Log.d(TAG,"open():"+open());
	}	

	public static gpio newInstance() throws Exception{
		if(mGPIO == null){
			mGPIO = new gpio();
		}
		return mGPIO;
	}
	
	public void setGPIO(String s1,String s2){
		Log.d(TAG,"setGPIO "+s1 + " "+s2);
		
		s1 = s1.substring(4);//GPIOD18
		int arg = Integer.parseInt(s1.substring(1));		
		Log.d(TAG,"s1 "+s1+" :"+arg);
		
		if(s1.startsWith("A")){
			sendcmd(CMD_SET_GPIOA_OUT, arg);
		}else if(s1.startsWith("B")){
			sendcmd(CMD_SET_GPIOB_OUT, arg);
		}else if(s1.startsWith("C")){
			sendcmd(CMD_SET_GPIOC_OUT, arg);
		}else if(s1.startsWith("D")){
			sendcmd(CMD_SET_GPIOD_OUT, arg);
		}else if(s1.startsWith("E")){
			sendcmd(CMD_SET_GPIOE_OUT, arg);
		}		
		
		s2 = s2.substring(4);//GPIOD29
		arg = Integer.parseInt(s2.substring(1));		
		Log.d(TAG,"s2 "+s2+" :"+arg);
		
		if(s2.startsWith("A")){
			sendcmd(CMD_SET_GPIOA_IN, arg);
		}else if(s2.startsWith("B")){
			sendcmd(CMD_SET_GPIOB_IN, arg);
		}else if(s2.startsWith("C")){
			sendcmd(CMD_SET_GPIOC_IN, arg);
		}else if(s2.startsWith("D")){
			sendcmd(CMD_SET_GPIOD_IN, arg);
		}else if(s2.startsWith("E")){
			sendcmd(CMD_SET_GPIOE_IN, arg);
		}
	}
	public int request_GPIO(){
		int ret = mGPIO.sendcmd(CMD_INIT, 1);
		return ret;
	}
	
	public int set_on_GPIO(){
		int ret = mGPIO.sendcmd(CMD_SET_ON_OFF, 1);
		return ret;
	}
	public int set_off_GPIO(){
		int ret = mGPIO.sendcmd(CMD_SET_ON_OFF, 0);
		return ret;
	}
	public int read_on_off_GPIO(){
		int ret = mGPIO.sendcmd(CMD_READ_ON_OFF, -1);
		return ret;
	}
//	public int testGPIO(){
//		Log.d(TAG,"testGPIO");		
//		sendcmd(CMD_INIT, -1);
//		int ret = mGPIO.sendcmd(CMD_START_TEST, -1);
//		sendcmd(CMD_RELEASE, -1);
//		return ret;
//	}
	}
	
	
	

