package com.android.settings.tvout;

import android.util.Log;
import com.actions.hardware.DisplayManager;

public class TvoutUtils {

    public static TvoutUtils getInstance() {
        if (null == mTvoutUtils) {
            mTvoutUtils = new TvoutUtils();
        }
        return mTvoutUtils;
    }

    public boolean getHdmiState() {
        return mHdmiState;
    }

    public void setHdmiEnabled(boolean state) {
        if (mHdmiState != state) {
            mHdmiState = state;
            mDisplayManager.setHdmiEnable(state);
        }
    }

    public boolean getCvbsState() {
        return mCvbsState;
    }

    public void setCvbsEnabled(boolean state) {
        if (mCvbsState != state) {
            mCvbsState = state;
            mDisplayManager.setCvbsEnable(state);
        }
    }

    public void setCvbsMode(int mode) {
        if (mCvbsMode != mode) {
            mCvbsMode = mode;
            mDisplayManager.setCvbsMode(getModeStrFromInt(mode));
        }
    }

    public int getCvbsMode() {
        return mCvbsMode;
    }

    public int[] getScreenScaleSize() {
        int[] scales = new int[2];
        mDisplayManager.getHdmiViewFrameSize(scales);
        return scales;
    }

    public void setScreenScaleSize(int x, int y) {
        mDisplayManager.setHdmiViewFrameSize(x, y);
    }


    private TvoutUtils() {
        mDisplayManager = new DisplayManager();
        mHdmiState = mDisplayManager.getHdmiEnable();
        mCvbsState = mDisplayManager.getCvbsEnable();
        mCvbsMode = getModeIntFromStr(mDisplayManager.getCvbsMode());
    }

    private String getModeStrFromInt(int modeInt) {
        String modeStr = null;
        if (CVBS_MODE_P_INT == modeInt) {
            modeStr = CVBS_MODE_P_STR;
        } else if (CVBS_MODE_N_INT == modeInt) {
            modeStr = CVBS_MODE_N_STR;
        }
        return modeStr;
    }

    private int getModeIntFromStr(String modeStr) {
        int modeInt = CVBS_MODE_ERR;
        if (modeStr != null) {
            if (modeStr.equals(CVBS_MODE_P_STR)) {
                modeInt = CVBS_MODE_P_INT;
            } else if (modeStr.equals(CVBS_MODE_N_STR)) {
                modeInt = CVBS_MODE_N_INT;
            }
        }
        return modeInt;
    }

    private static TvoutUtils mTvoutUtils = null;

    private boolean mHdmiState = false;
    private boolean mCvbsState = false;
    private int mCvbsMode = CVBS_MODE_ERR;
    private DisplayManager mDisplayManager = null;

    private static final int CVBS_MODE_ERR = -1;
    private static final int CVBS_MODE_P_INT = 0;
    private static final int CVBS_MODE_N_INT = 1;
    private static final String CVBS_MODE_P_STR = "PAL";
    private static final String CVBS_MODE_N_STR = "NTSC";
}
