package com.android.settings.tvout;

import java.lang.reflect.Method;

import com.android.settings.R;
import android.os.Bundle;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences.Editor;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.os.SystemProperties;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import com.android.settings.tvout.TvoutUtils;
import android.util.DisplayMetrics;
import android.widget.RelativeLayout;

public class TvoutScreenResizeActivity extends Activity  {
    public static String HDMI_SCALES = "hdmi_scales";
    private Button mConfirmButton;
    private Button mCancelButton;
    private SeekBar mWidthSeekBar;
    private SeekBar mHeightSeekBar;
    private int orgScales[];
    private int curScales[];
    static final String LOG_TAG = "TvoutScreenResizeActivity";

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().requestFeature(Window.FEATURE_ACTION_BAR_OVERLAY);
        // setNavVisibility(false);
        setContentView(R.layout.tvout_screen_resize);

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);

        mWidthSeekBar = (SeekBar) findViewById(R.id.seek_width);
        mWidthSeekBar.setOnSeekBarChangeListener(mWidthListener);
        mHeightSeekBar = (SeekBar) findViewById(R.id.seek_height);
        mHeightSeekBar.setOnSeekBarChangeListener(mHeightListener);

        mConfirmButton = (Button) findViewById(R.id.confirm_button);
        mCancelButton = (Button) findViewById(R.id.cancel_button);

        curScales = TvoutUtils.getInstanceByName(TvoutUtils.TVOUT_HDMI).getTvDisplayScale();
        orgScales = curScales.clone();
        if (curScales != null) {
            mWidthSeekBar.setProgress(curScales[0]);
            mHeightSeekBar.setProgress(curScales[1]);
        }

        OnClickListener confirmListener = new OnClickListener() {
            public void onClick(View v) {
                TvoutUtils.getInstanceByName(TvoutUtils.TVOUT_HDMI).setTvDisplayScale(curScales[0], curScales[1]);
                finish();
            }
        };
        mConfirmButton.setOnClickListener(confirmListener);

        OnClickListener cancelListener = new OnClickListener() {
            public void onClick(View v) {
                if (orgScales != null) {
                    Log.d(LOG_TAG, "curScales[0]=" + curScales[0] + ",curScales[1]=" + curScales[1]);
                    TvoutUtils.getInstanceByName(TvoutUtils.TVOUT_HDMI).setTvDisplayScale(orgScales[0], orgScales[1]);
                }
                finish();
            }
        };
        mCancelButton.setOnClickListener(cancelListener);
    }

    void setNavVisibility(boolean visible) {
        int newVis = View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
        if (!visible) {
            newVis |= View.SYSTEM_UI_FLAG_LOW_PROFILE | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
        }

        getWindow().getDecorView().setSystemUiVisibility(newVis);
    }

    private SeekBar.OnSeekBarChangeListener mWidthListener = new SeekBar.OnSeekBarChangeListener() {
    	public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            if (curScales != null) {
                curScales[0] = progress;
                TvoutUtils.getInstanceByName(TvoutUtils.TVOUT_HDMI).setTvDisplayScale(curScales[0], curScales[1]);
                Log.d(LOG_TAG, "curScales[0]=" + curScales[0] + ",curScales[1]=" + curScales[1]);
            }
        }

        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        public void onStopTrackingTouch(SeekBar seekBar) {

        }
    };
    
    private SeekBar.OnSeekBarChangeListener mHeightListener = new SeekBar.OnSeekBarChangeListener() {
    	public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            if (curScales != null) {
                curScales[1] = progress;
                TvoutUtils.getInstanceByName(TvoutUtils.TVOUT_HDMI).setTvDisplayScale(curScales[0], curScales[1]);
                Log.d(LOG_TAG, "curScales[0]=" + curScales[0] + ",curScales[1]=" + curScales[1]);
            }
        }

        public void onStartTrackingTouch(SeekBar seekBar) {

        }

        public void onStopTrackingTouch(SeekBar seekBar) {

        }
    };
}
