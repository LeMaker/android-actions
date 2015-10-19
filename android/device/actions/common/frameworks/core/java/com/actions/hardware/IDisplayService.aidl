package com.actions.hardware;

import com.actions.hardware.ICableStatusListener;

interface IDisplayService
{
     
    void setHdmiEnable(boolean enable);
    
    boolean setHdmiMode(String mode);
	
	boolean getHdmiEnable();
    
    String getHdmiMode();
    
    String[] getHdmiSupportedModesList();
    
    void setHdmiViewFrameSize(int dx, int dy);
    
    void getHdmiViewFrameSize(out int[] dx_dy);
    
    boolean getHdmiCableState();
    
    boolean setHdmiFitScreen(int value);

    int getHdmiFitScreen();
    
    int setDisplayInfo(int display,in int[] info);	
	
    int getDisplayInfo(int display,out int[] info);
}