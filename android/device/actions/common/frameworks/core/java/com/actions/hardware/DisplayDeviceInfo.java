/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.actions.hardware;


import libcore.util.Objects;

/**
 * Describes the characteristics of a physical display device.
 */
final class DisplayDeviceInfo{
    /**
     * Flag: Indicates that this display device should be considered the default display
     * device of the system.
     */
    private static final int MAX_PRIVATE_DATA_SIZE = 40;
    
    public static final int MAX_DISPINFO_SIZE = MAX_PRIVATE_DATA_SIZE + 10;
	
	public static final int TYPE_LCD_DISPLAY = 1 << 0;
	
	public static final int TYPE_HDMI_DISPLAY = 1 << 1;
	
	public static final int TYPE_CVBS_DISPLAY = 1 << 2;
	
	public static final int TYPE_YPBPR_DISPLAY = 1 << 3;
	
    /**max x scale rate*/
    public static final int MAX_SCALE_X = 75;
    
    /**max y scale rate*/
    public static final int MAX_SCALE_Y = 75;
    
    /*set cmd*/
    public static final int SET_LIGHTENESS = 0;
    public static final int SET_SATURATION = 1;
    public static final int SET_CONSTRAST = 2;
	public static final int SET_DEFAULT = 3;
    
    public static final int PD_LCD_TYPE_OFF = 0;    
    public static final int PD_LCD_LIGHTENESS_OFF = PD_LCD_TYPE_OFF + 1;
    public static final int PD_LCD_SATURATION_OFF = PD_LCD_LIGHTENESS_OFF + 1;
    public static final int PD_LCD_CONSTRAST_OFF = PD_LCD_SATURATION_OFF + 1;
    public static final int PD_LCD_DITHER_OFF = PD_LCD_CONSTRAST_OFF + 1;
    public static final int PD_LCD_GAMMA_OFF = PD_LCD_DITHER_OFF + 1;
    public static final int PD_LCD_GAMMA_RX = PD_LCD_GAMMA_OFF + 1;
    public static final int PD_LCD_GAMMA_GX = PD_LCD_GAMMA_RX + 1;
	public static final int PD_LCD_GAMMA_BX = PD_LCD_GAMMA_GX + 1;
	public static final int PD_LCD_GAMMA_RY = PD_LCD_GAMMA_BX + 1;
	public static final int PD_LCD_GAMMA_GY = PD_LCD_GAMMA_RY + 1;
	public static final int PD_LCD_GAMMA_BY = PD_LCD_GAMMA_GY + 1;
    //LCD INFO
	public static final int PD_LCD_PIXCLOCK = PD_LCD_GAMMA_BY + 1;
    public static final int PD_LCD_HSW = PD_LCD_PIXCLOCK + 1;
    public static final int PD_LCD_HFP = PD_LCD_HSW + 1;
    public static final int PD_LCD_HBP = PD_LCD_HFP + 1;
    public static final int PD_LCD_VSW = PD_LCD_HBP + 1;
    public static final int PD_LCD_VFP = PD_LCD_VSW + 1;
    public static final int PD_LCD_VBP = PD_LCD_VFP + 1;

    /**
    * Gets the name of the display device, which may be derived from
    * EDID or other sources.  The name may be displayed to the user.
    */
    public String mName;
    
    /**
    * Display type.
    */
    public int mType;
    
    public int mState;
    
   /**
    * disply Plug state.
    */
    public int mPluginState;

    /**
     * The width of the display in its natural orientation, in pixels.
     * This value is not affected by display rotation.
     */
    public int mWidth;

    /**
     * The height of the display in its natural orientation, in pixels.
     * This value is not affected by display rotation.
     */
    public int mHeight;

    /**
     * The refresh rate of the display.
     */
    public int mRefreshRate;    
    
    /**
    *The width scale rate of the display  
    */    
	public int mWidthScale;	
	
	/**
    *The height scale rate of the display  
    */    
	public int mHeightScale;
	
	/**
    *The cmdmode  
    */    
	public int mCmdMode;
	
	/**
	THE IC_TYPE
	*/
	public int mIcType;
    /**
    *The private data of the display  
    */    
	public int[] mPrivateDate = new int[MAX_PRIVATE_DATA_SIZE];
	
	public int toIntArray(int[] info,int offset) {
		//info[offset++] = this.mType;
		info[offset++] = this.mState;
        info[offset++] = this.mPluginState;
        info[offset++] = this.mWidth;
        info[offset++] = this.mHeight;
        info[offset++] = this.mRefreshRate;
        info[offset++] = this.mWidthScale;
        info[offset++] = this.mHeightScale;
        info[offset++] = this.mCmdMode;
        info[offset++] = this.mIcType;
        System.arraycopy(this.mPrivateDate, 0, info, offset, this.mPrivateDate.length);
        return 0;
    }
    
    public int getFromIntArray(int[] info , int offset) {
        //this.mType = info[offset++];
        this.mState = info[offset++];
        this.mPluginState = info[offset++];
        this.mWidth = info[offset++];
        this.mHeight = info[offset++];
        this.mRefreshRate = info[offset++];
        this.mWidthScale = info[offset++];
        this.mHeightScale = info[offset++];
        this.mCmdMode = info[offset++];
        this.mIcType = info[offset++];
        System.arraycopy(info, offset, this.mPrivateDate, 0, this.mPrivateDate.length);
        return 0;
    }

    // For debugging purposes
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("DisplayDeviceInfo{\"");
        sb.append(mName).append("\": ").append(mWidth).append(" x ").append(mHeight);
        sb.append(", ").append(mRefreshRate).append(" fps, ");
        sb.append(", ").append("width_scale ").append(mWidthScale);
		sb.append(", ").append("height_scale ").append(mHeightScale);
        sb.append("}");
        return sb.toString();
    }
}
