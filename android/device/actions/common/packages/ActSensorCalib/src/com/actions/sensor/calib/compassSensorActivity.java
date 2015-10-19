package com.actions.sensor.calib;

import android.app.Activity;
import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.Button;
import android.widget.TextView;

public class compassSensorActivity extends Activity  implements SensorEventListener{
	private final String TAG = "compassSensorActivity";
	
	private TextView mTextView_x;
	private TextView mTextView_y;
	private TextView mTextView_z;
	private TextView mTextView_accuracy;
	private Button mButtonCalib;
	private Button mButtonReset;
	
	private SensorManager sm = null;
	private SensorControl sc = null;
	private boolean mCalibMode = false;
	private Handler mHandler;
	
	private void findViews() {
		mTextView_x = (TextView) findViewById(R.id.mTextView_magn_x);
		mTextView_y = (TextView) findViewById(R.id.mTextView_magn_y);
		mTextView_z = (TextView) findViewById(R.id.mTextView_magn_z);
		mTextView_accuracy = (TextView) findViewById(R.id.mTextView_magn_accuracy);
		if (mTextView_accuracy != null) {
			String txt = String.format("Accuracy: %d", 0);
			mTextView_accuracy.setText(txt);
			mTextView_accuracy.setTextColor(-16711681);
		}
		//mViewGuide = ()
	}

	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        Log.i(TAG,"onCreate...");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.compasssensor);
        findViews();
        sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
    }
    
    @Override
    public void onStart(){
    	Log.i(TAG,"onStart...");
    	super.onStart();
    }
    
    @Override
    public void onPause(){
    	Log.i(TAG,"onPause...");
    	super.onPause();
    	sm.unregisterListener(this);
    }
    
    @Override
    public void onResume(){
    	Log.i(TAG,"onResume...");
    	super.onResume();
    	sm.registerListener(this,
				sm.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD),
				SensorManager.SENSOR_DELAY_NORMAL);
    }
    
    @Override
    public void onStop(){
    	
    	Log.i(TAG,"onStop...");
    	super.onStop();	
    }
    
    @Override
    public void onDestroy(){
    	
    	Log.i(TAG,"onDestroy...");
    	super.onDestroy();
    	mHandler = null;
		sm = null;
		sc = null;
    }
    
    public void onAccuracyChanged(Sensor paramSensor, int paramInt) {
    	if (mTextView_accuracy != null) {
			String txt = String.format("Accuracy: %d", paramInt);
			mTextView_accuracy.setText(txt);
			mTextView_accuracy.setTextColor(-16711681);
		}
    
	}

	public void onSensorChanged(SensorEvent e) {
		if ((e != null) && (e.values.length == 3)) {
			//filter event in calib mode
			if(mCalibMode)
				return;

			if (mTextView_x != null) {
				String txt = String.format(" Magn_X : %.5f", e.values[0]);
				mTextView_x.setText(txt);
				mTextView_x.setTextColor(-16711681);
			}
			
			if (mTextView_y != null) {
				String txt = String.format(" Magn_Y : %.5f", e.values[1]);
				mTextView_y.setText(txt);
				mTextView_y.setTextColor(-16711681);
			}
			
			if (mTextView_z != null) {
				String txt = String.format(" Magn_Z : %.5f", e.values[2]);
				mTextView_z.setText(txt);
				mTextView_z.setTextColor(-16711681);
			}
			//if (mSensorHost != null)
			//	mSensorHost.onSensorChanged(e);
		}
	}
	

}