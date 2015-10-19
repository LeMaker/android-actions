package com.android.settings.tvout;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.File;
import java.io.IOException;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.LinkedHashMap;
import java.util.Set;
import java.util.Collection;
import java.util.ArrayList;
import java.util.Iterator;
import android.util.Log;
import android.content.Context;
import com.android.settings.R;
import android.os.SystemProperties;
import android.provider.Settings;
import com.actions.hardware.DisplayManager;

public class TvoutUtils {
    static final String TAG = "TvoutUtils";
    private final String FILENAME_HDMI_MODE_SAVE = "/data/data/com.android.settings/hdmi_mode";
    private final String FILENAME_HDMI_WIDTH_SAVE = "/data/data/com.android.settings/hdmi_width";
    private final String FILENAME_HDMI_HEIGHT_SAVE = "/data/data/com.android.settings/hdmi_height";
    static DisplayManager mDisplayManager;
    // Context mContext;
    List<String> mSupportedModesList;
    String mSelectMode;
    String mTvoutTypeName;
    private static TvoutUtils sTvoutInstance = new TvoutUtils();
    private static CvbsUtils sCvbsInstance;
    private static HdmiUtils sHdmiInstance;

    public static final String TVOUT_CVBS = "cvbs";
    public static final String TVOUT_HDMI = "hdmi";
    static final String TVOUT_DISCONNECT = "Disconnect";

    public TvoutUtils() {

    }

    public static TvoutUtils getInstanceByName(String tvoutTypeName) {
        return sTvoutInstance.safeGetInstanceByName(tvoutTypeName);
    }

    TvoutUtils safeGetInstanceByName(String tvoutTypeName) {
        if (mDisplayManager == null) {
            mDisplayManager = new DisplayManager();
        }
        Log.d(TAG, "safeGetInstanceByName: " + tvoutTypeName);
        if (tvoutTypeName.equals(TVOUT_CVBS)) {
            if (sCvbsInstance == null) {
                sCvbsInstance = new CvbsUtils();
            }
            return sCvbsInstance;
        } else if (tvoutTypeName.equals(TVOUT_HDMI)) {
            if (sHdmiInstance == null) {
                sHdmiInstance = new HdmiUtils();
            }
            return sHdmiInstance;
        }

        return this;
    }


    public String getLastSelectModeValue() {
        return mSelectMode;
    }

    public int[] getTvDisplayScale() {
        int[] scales = new int[2];
        return scales;
    }

    public boolean setTvDisplayScale(int xscale, int yscale) {
        return true;
    }

    public String[] getSupportedModesList() {
        return null;
    }

    public boolean isCablePlugIn() {
            return  mDisplayManager.getHdmiCableState();
    }


    public class CvbsUtils extends TvoutUtils {
        public CvbsUtils() {
            mTvoutTypeName = TVOUT_CVBS;
        }

        public String[] getSupportedModesList() {
            return null;
        }

        private void setTheCvbsFormat(String format) {
            int mode = 1;
            if (format.equals("pal")) {
                mode = 0;
            } else {
                format = "ntsc";
            }
            Log.d(TAG, "mode=" + mode);
            // mDisplayManager.setFormat(format);
        }

        public void switchToSelectModeByModeValue(String modeValue) {
            Log.d(TAG, "the cvbs select mode value=" + modeValue);
            String mode = "";
            if (mTvoutTypeName.equals(TVOUT_CVBS)) {
                if (modeValue.equals("PAL")) {
                    mode = "PAL";
                } else if (modeValue.equals("TVOUT_DISCONNECT")) {
                    mode = TVOUT_DISCONNECT;
                } else {
                    mode = "NTSC";
                }
            }

        }

    }

    public class HdmiUtils extends TvoutUtils {
        public static final int HDMI_DEFAULT_VID = 19;

        public HdmiUtils() {
            mTvoutTypeName = TVOUT_HDMI;
        }

        public String getMode() {
	        return mDisplayManager.getHdmiMode();
        }

        public void setMode(String value) {
	         mDisplayManager.setHdmiMode(value);
        }

        public String[] getSupportedModesList() {
            return mDisplayManager.getHdmiSupportedModesList();
        }    

        public void setHdmiEnable(boolean flag) {
                mDisplayManager.setHdmiEnable(flag);
        }
		
        public boolean getHdmiEnable() {
             return mDisplayManager.getHdmiEnable();
        }
        
        public void switchToSelectModeByModeValue(String mode) {
            Log.d(TAG, "the hdmi select mode vid=" + mode);
            if(!mode.equals(mSelectMode)){
            	mDisplayManager.setHdmiMode(mode);
                mSelectMode = mode;
            }
        }

        public boolean setHdmiFitScreen(int value){
            return mDisplayManager.setHdmiFitScreen(value);
        }

        public int getHdmiFitScreen(){
            return mDisplayManager.getHdmiFitScreen();
        }
        
        public int[] getTvDisplayScale() {
	        int[] scales = new int[2];
	        mDisplayManager.getHdmiViewFrameSize(scales);
	        return scales;
	    }

		public boolean setTvDisplayScale(int xscale, int yscale) {
			 mDisplayManager.setHdmiViewFrameSize(xscale,yscale);
			 return true;
		}

        
        
        
    }
    
}
