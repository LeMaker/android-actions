/*
 * Copyright (c) 2012 Actions-semi co. All Rights Reserved.
 * Author Actions
 * Version V1.0  
 * Date:  2012-9-12
 * Comment:this class lets you access the Display Engine,cvbs and hdmi.
 */

package com.actions.hardware;
 
import java.util.StringTokenizer;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import com.actions.hardware.IDisplayService;
import com.actions.hardware.ICableStatusListener;

/**
 * This class lets you access the Display Engine,cvbs and hdmi.
 * <p>
 * You can set which displayer output with {@link #setOutputDisplayer}.Also,you<br>
 * can set cvbs mode and hdmi using {@link #setDisplayPara} and
 * {@link #setHdmiVid} <br>
 * respectively etc..
 * 
 * @author Actions
 * @version V2.0
 */
public class DisplayManager {
	
    private static final String TAG = "DisplayManager";
    
    private static final int MAX_DISPLAY_NUMBER = 4;
    
    private DisplayDeviceInfo[] mDisplay = new DisplayDeviceInfo[MAX_DISPLAY_NUMBER];

    private IDisplayService mDisplayService;

    /** Constructor,complete initialiazation, get Bp of DisplayService */
    public DisplayManager() {

        mDisplayService = IDisplayService.Stub.asInterface(ServiceManager.getService("actions.display"));
        
        if (mDisplayService != null) {        	
           
        } 
        Log.d(TAG, "enter DisplayManager ~~~~~!");
    }
    public int getDisplayerCount() {
		Log.d(TAG, "enter getDisplayerCount ~~~~~!");
        return 2;
    }

    public boolean getHdmiEnable() { // Alicia add 2013.07.31    	
        try {
        	Log.d(TAG, "getHdmiEnable " + mDisplayService.getHdmiEnable()); 
            return mDisplayService.getHdmiEnable();
        } catch (Exception e) {
            Log.e(TAG, "Display service getHdmiEnable failed", e);
        }
        return false;
    }

    public void setHdmiEnable(boolean enable) {
    	Log.d(TAG, "setHdmiEnable " + enable); 
        try {
            mDisplayService.setHdmiEnable(enable);
        } catch (Exception e) {
            Log.e(TAG, "Display service enableHdmi failed", e);
        }
    }

    public boolean setHdmiMode(String mode) {
    	Log.d(TAG, "setHdmiMode " + mode); 
        try {
            mDisplayService.setHdmiMode(mode);
        } catch (Exception e) {
            Log.e(TAG, "Display service setMode failed", e);
        }

        return true;
    }

    public String getHdmiMode() {    	
        try {
        	Log.d(TAG, "getHdmiMode " + mDisplayService.getHdmiMode()); 
            return mDisplayService.getHdmiMode();
        } catch (Exception e) {
            Log.e(TAG, "Display service getMode failed", e);
        }
        return null;
    }    
    //CVBS Setting
    public void setCvbsEnable(boolean enable) {
    	Log.d(TAG, "setCvbsEnable " + enable); 
        try {
            mDisplayService.setCvbsEnable(enable);
        } catch (Exception e) {
            Log.e(TAG, "Display service setCvbsEnable failed", e);
        }
    }
     public boolean getCvbsEnable() {  	
        try {
        	Log.d(TAG, "getCvbsEnable " + mDisplayService.getCvbsEnable()); 
            return mDisplayService.getCvbsEnable();
        } catch (Exception e) {
            Log.e(TAG, "Display service getCvbsEnable failed", e);
        }
        return false;
    }
     public String getCvbsMode() {    	
        try {
        	Log.d(TAG, "getCvbsMode " + mDisplayService.getCvbsMode()); 
            return mDisplayService.getCvbsMode();
        } catch (Exception e) {
            Log.e(TAG, "Display service get cvbs Mode failed", e);
        }
        return null;
    }
    
      public boolean setCvbsMode(String mode) {
    	Log.d(TAG, "setCvbsMode " + mode); 
        try {
            mDisplayService.setCvbsMode(mode);
        } catch (Exception e) {
            Log.e(TAG, "Display service setMode failed", e);
        }

        return true;
    }

    public String[] getHdmiSupportedModesList(){
    	Log.d(TAG, "getHdmiSupportedModesList "); 
        try {
            return mDisplayService.getHdmiSupportedModesList();
        }catch (Exception e) {
            Log.e(TAG, "Display service getSupportedModesList failed", e);
        }

        return null;
    }

    public void setHdmiViewFrameSize(int dx, int dy) {
    	Log.d(TAG, "setHdmiViewFrameSize dx" + dx + " dy " + dy); 
        try {
            mDisplayService.setHdmiViewFrameSize(dx, dy);
        } catch (Exception e) {
            Log.e(TAG, "Display service resize failed", e);
        }
    }
    
    public void getHdmiViewFrameSize(int[] dx_dy) {
    	
        try {
            mDisplayService.getHdmiViewFrameSize(dx_dy);
        } catch (Exception e) {
            Log.e(TAG, "Display service resize failed", e);
        }
        Log.d(TAG, "getHdmiViewFrameSize dx" + dx_dy[0] + " dy " + dx_dy[1]); 
    }

    public boolean getHdmiCableState() {    	
        try {
        	Log.d(TAG, "getHdmiCableState CableState" + mDisplayService.getHdmiCableState()); 
            return mDisplayService.getHdmiCableState();
        } catch (Exception e) {
            Log.e(TAG, "Display service getCableState failed", e);
        }
        return false;
    }

    public boolean setHdmiFitScreen(int value){
    	Log.d(TAG, "setHdmiFitScreen value " + value); 
        try {
            return mDisplayService.setHdmiFitScreen(value);
        } catch (Exception e) {
            Log.e(TAG, "Display service setDisFitScreen failed",e);
        }
        return false;
    }

    public int getHdmiFitScreen(){
    	Log.d(TAG, "getHdmiFitScreen "); 
        try {
            return mDisplayService.getHdmiFitScreen();
        } catch (Exception e) {
            Log.e(TAG, "Display service getDisFitScreen failed", e);
        }

        return 3;
    }
    
    
    private DisplayDeviceInfo findDisplayInfo(int disp_id){
    	DisplayDeviceInfo info = null;
    	Log.d(TAG, "findDisplayInfo " + disp_id); 
    	for(int i = 0 ; i < MAX_DISPLAY_NUMBER ; i++){
    		if(mDisplay[i] != null && mDisplay[i].mType == disp_id){
    			info = mDisplay[i];
    		}
    	}
    	if(info == null){
    		Log.d(TAG, "not found display in list  " + disp_id + " we create a new one "); 
	    	for(int i = 0 ; i < MAX_DISPLAY_NUMBER ; i++){
	    		if(mDisplay[i] == null ){
	    			info = new DisplayDeviceInfo();
	    			info.mType = disp_id;
	    			mDisplay[i] = info;
	    		}
	    	}
    	}
    	return info;
    }
    
    private DisplayDeviceInfo getDisplayInfo(int disp_id){
    	int result = 0;
    	Log.d(TAG, "getDisplayInfo " + disp_id);
    	DisplayDeviceInfo info = findDisplayInfo(disp_id);
    	if(info != null){
	    	int info_data[] = new int[DisplayDeviceInfo.MAX_DISPINFO_SIZE];
	    	try {
	            result = mDisplayService.getDisplayInfo(disp_id,info_data);
	        } catch (Exception e) {
				Log.e(TAG, "disp sevice get display info error "); 
	        }    	
			info.getFromIntArray(info_data,1);
		}
    	return info;
    }
    
    private boolean setDisplayInfo(int disp_id){
    	int result = 0;
    	Log.d(TAG, "setDisplayInfo " + disp_id);
    	DisplayDeviceInfo info = findDisplayInfo(disp_id);
    	if(info != null){
	    	int info_data[] = new int[DisplayDeviceInfo.MAX_DISPINFO_SIZE];
	    	if(info.toIntArray(info_data,1) == 0){
		    	try {
		            result = mDisplayService.setDisplayInfo(disp_id,info_data);
		            if(result == 0){
		            	return true;
		            }
		        } catch (Exception e) {
					Log.e(TAG, "disp sevice set display info error "); 
		        }  
	    	}
    	}    	
    	return false;
    }

    public int getIcType() {  
    	Log.d(TAG, "getIcType Enter");        
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){
 			return info.mIcType;   		
 		}else{
 			Log.e(TAG, "error! can not get the ic type service!");
 		} 		 
    	return -1;  
    }
    public int getLcdType() { 
    	Log.d(TAG, "getLcdType Enter"); 
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){
 			return info.mPrivateDate[DisplayDeviceInfo.PD_LCD_TYPE_OFF];	 						
 		}else{
 			Log.e(TAG, "error! can not get the lcd display service!");
 		} 		 
    	return -1;      
    }    
    public int setLcdLighteness(int value) {  
    	Log.d(TAG, "setLcdLighteness Enter value is " + value);
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){
			info.mCmdMode = DisplayDeviceInfo.SET_LIGHTENESS;
			info.mPrivateDate[DisplayDeviceInfo.PD_LCD_LIGHTENESS_OFF] = value;	
			setDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 			return 0;   			
 		}else{
 			Log.e(TAG, "error! can not get the lcd display service!");
 		} 		 
    	return -1;      
    }
    public int getLcdLighteness() {    
    	Log.d(TAG, "getLcdLighteness Enter");
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){
			return info.mPrivateDate[DisplayDeviceInfo.PD_LCD_LIGHTENESS_OFF];  			
 		}else{
 			Log.e(TAG, "error! can not get the lcd display service!");
 		} 		 
    	return -1;     
    }
    public int setLcdSaturation(int value) {    
    	Log.d(TAG, "setLcdSaturation Enter value is " + value); 
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){
 			info.mCmdMode = DisplayDeviceInfo.SET_SATURATION;
			info.mPrivateDate[DisplayDeviceInfo.PD_LCD_SATURATION_OFF] = value;	
			setDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 			return 0;   			
 		}else{
 			Log.e(TAG, "error! can not get the lcd display service!");
 		} 		 
    	return -1;  
    }
    public int getLcdSaturation() { 
    	Log.d(TAG, "getLcdSaturation Enter");    
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){
			return info.mPrivateDate[DisplayDeviceInfo.PD_LCD_SATURATION_OFF];			
 		}else{
 			Log.e(TAG, "error! can not get the lcd display service!");
 		} 		 
    	return -1;    
    }
    public int setLcdContrast(int value) {  
    	Log.d(TAG, "setLcdContrast Enter value is " + value); 
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){
 			info.mCmdMode = DisplayDeviceInfo.SET_CONSTRAST;
			info.mPrivateDate[DisplayDeviceInfo.PD_LCD_CONSTRAST_OFF] = value;	
			setDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 			return 0;   			
 		}else{
 			Log.e(TAG, "error! can not get the lcd display service!");
 		} 		 
    	return -1;       
    }
    public int getLcdContrast() {  
    	Log.d(TAG, "getLcdContrast Enter");      
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){			
 			return info.mPrivateDate[DisplayDeviceInfo.PD_LCD_CONSTRAST_OFF];   			
 		}else{
 			Log.e(TAG, "error! can not get the lcd display service!");
 		} 		 
    	return -1;  
    }
	public int setDefault() {		
		Log.d(TAG, "setDefault Enter");  
    	DisplayDeviceInfo info = getDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);
 		if(info != null){
 			info.mCmdMode = DisplayDeviceInfo.SET_DEFAULT;			
			setDisplayInfo(DisplayDeviceInfo.TYPE_LCD_DISPLAY);		
			return 0;  	
 		}else{
 			Log.e(TAG, "error! can not get the lcd display service!");
 		} 		 
    	return -1;     
    }
}
