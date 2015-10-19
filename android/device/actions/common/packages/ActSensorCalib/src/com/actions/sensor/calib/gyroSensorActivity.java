package com.actions.sensor.calib;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

public class gyroSensorActivity extends Activity implements SensorEventListener{
	private final String TAG = "gyroSensorActivity";
	private final int mButtonDelay = 1000;
	
	
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
		mTextView_x = (TextView) findViewById(R.id.mTextView_gyro_x);
		mTextView_y = (TextView) findViewById(R.id.mTextView_gyro_y);
		mTextView_z = (TextView) findViewById(R.id.mTextView_gyro_z);
		mTextView_accuracy = (TextView) findViewById(R.id.mTextView_gyro_accuracy);
		if (mTextView_accuracy != null) {
			String txt = String.format("Accuracy: %d", 0);
			mTextView_accuracy.setText(txt);
			mTextView_accuracy.setTextColor(-16711681);
		}
		
		mButtonCalib = (Button) findViewById(R.id.gyro_buttonCalib);
		mButtonReset = (Button) findViewById(R.id.gyro_buttonReset);
		
	}
	
	
private Runnable mCalibRunnable = new Runnable() {
        
		public void run() {
			mButtonCalib.setClickable(true);
			mCalibMode = false;
            
			if(sc.calibration_type == sc.CALI_TYPE_INPUT)
			{
				Log.i(TAG, "calibration_type: CALI_TYPE_INPUT");
			// calibrate g-sensor
			sc.runCalib();

			// get new calib data
			String calib = sc.getCalibValue();
			Log.i(TAG, "Calib: " + calib);

			// write calibration file
			String file = calib + "\n";
			sc.writeCalibFile(file);
			}
			else if(sc.calibration_type == sc.CALI_TYPE_IIO)
			{
				Log.i(TAG, "calibration_type: CALI_TYPE_IIO");
			Log.i(TAG, "start inv self_test...");
			
			String args = "inv_self_test-shared -w";
            try
            {
                Process process = Runtime.getRuntime().exec(args);
				process.waitFor();
				
				//get the err line
                InputStream stderr = process.getErrorStream();
                InputStreamReader isrerr = new InputStreamReader(stderr);
                BufferedReader brerr = new BufferedReader(isrerr);

				//get the output line  
                InputStream outs = process.getInputStream();
                InputStreamReader isrout = new InputStreamReader(outs);
                BufferedReader brout = new BufferedReader(isrout);

                String errline = null;
                String result = "";

				// get the whole error message string  
                while ( (errline = brerr.readLine()) != null)
                {
                    result += errline;
                    result += "\n";
                } 

                if( result != "" )
                {
                    // put the result string on the screen
                	Log.i(TAG, result);

                }
                
                String outline = null;
                String out_result = "";
                 // get the whole standard output string
                 while ( (outline = brout.readLine()) != null)
                {
                	 out_result += outline;
                	 out_result += "\n";
                }
                if( out_result != "" )
                {
                    // put the result string on the screen
                	Log.i(TAG, out_result);

                }
               
				

             }catch(Throwable t)
             {
                 t.printStackTrace();
             }
			}

			// show info
			Toast.makeText(gyroSensorActivity.this, R.string.info_calib, 
					Toast.LENGTH_LONG).show();						
		}
	};
    
	private View.OnClickListener mBtnCalibListener = new View.OnClickListener() {

		public void onClick(View v) {
			mButtonCalib.setClickable(false);
			mCalibMode = true;
            
			if(sc.calibration_type == sc.CALI_TYPE_INPUT)
			{
			// reset calibration
			sc.resetCalib();
			}

			mHandler.postDelayed(mCalibRunnable, mButtonDelay);
		}
	};

    private Runnable mResetRunnable = new Runnable() {
        
		public void run() {
			mButtonReset.setClickable(true);
			mCalibMode = false;

			if(sc.calibration_type == sc.CALI_TYPE_INPUT)
			{
			// get new calib data
			String calib = sc.getCalibValue();
			Log.i(TAG, "Calib: " + calib);

			// write calibration file
			String file = calib + "\n";
			sc.writeCalibFile(file);
			}
			else if(sc.calibration_type == sc.CALI_TYPE_IIO)
			{
			
            Log.i(TAG, " clear the MLCAL_FILE...");
            String args = "inv_self_test-shared -c";
            try
            {
                Process process = Runtime.getRuntime().exec(args);
				process.waitFor();
				
				//get the err line
                InputStream stderr = process.getErrorStream();
                InputStreamReader isrerr = new InputStreamReader(stderr);
                BufferedReader brerr = new BufferedReader(isrerr);

				//get the output line  
                InputStream outs = process.getInputStream();
                InputStreamReader isrout = new InputStreamReader(outs);
                BufferedReader brout = new BufferedReader(isrout);

                String errline = null;
                String result = "";

				// get the whole error message string  
                while ( (errline = brerr.readLine()) != null)
                {
                    result += errline;
                    result += "/n";
                } 

                if( result != "" )
                {
                    // put the result string on the screen
                	Log.i(TAG, result);

                }
                
                String outline = null;
                String out_result = "";
                 // get the whole standard output string
                 while ( (outline = brout.readLine()) != null)
                {
                	 out_result += outline;
                	 out_result += "/n";
                }
                if( out_result != "" )
                {
                    // put the result string on the screen
                	Log.i(TAG, out_result);

                }
             }catch(Throwable t)
             {
                 t.printStackTrace();
             }
			}
			

			// show info
			Toast.makeText(gyroSensorActivity.this, R.string.info_reset, 
					Toast.LENGTH_LONG).show();
		}
	};
    
	private View.OnClickListener mBtnResetListener = new View.OnClickListener() {

		public void onClick(View v) {
			mButtonReset.setClickable(false);
			mCalibMode = true;

			if(sc.calibration_type == sc.CALI_TYPE_INPUT)
			{
			// reset calibration
			sc.resetCalib();
			}

			mHandler.postDelayed(mResetRunnable, mButtonDelay);
		}
	};
	
	private void setListensers() {
		mButtonCalib.setOnClickListener(mBtnCalibListener);
		mButtonReset.setOnClickListener(mBtnResetListener);		
	}

	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        Log.i(TAG,"onCreate...");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.gyrosensor);
        findViews();
        
        mHandler = new Handler();
        sm = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        sc = new SensorControl(this);
        setListensers();
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
    	mHandler.removeCallbacks(mCalibRunnable);
        mHandler.removeCallbacks(mResetRunnable);
    }
    
    @Override
    public void onResume(){
    	Log.i(TAG,"onResume...");
    	super.onResume();
    	sm.registerListener(this,
				sm.getDefaultSensor(Sensor.TYPE_GYROSCOPE),
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
				String txt = String.format(" Gyro_X : %.5f", e.values[0]);
				mTextView_x.setText(txt);
				mTextView_x.setTextColor(-16711681);
			}
			
			if (mTextView_y != null) {
				String txt = String.format(" Gyro_Y : %.5f", e.values[1]);
				mTextView_y.setText(txt);
				mTextView_y.setTextColor(-16711681);
			}
			
			if (mTextView_z != null) {
				String txt = String.format(" Gyro_Z : %.5f", e.values[2]);
				mTextView_z.setText(txt);
				mTextView_z.setTextColor(-16711681);
			}
			//if (mSensorHost != null)
			//	mSensorHost.onSensorChanged(e);
		}
	}
	

}
