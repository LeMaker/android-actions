package com.actions.sensor.calib;


import java.util.List;

import android.app.Activity;
import android.content.Intent;
import android.hardware.Sensor;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;

public class SensorActivity extends Activity{
	private final String TAG = "SensorActivity";
	
	private Button mButtonGsensor;
	private Button mButtonGyrosensor;
	private Button mButtonCompasssensor;
	
	private ImageView iv_dist = null;
	private ImageView iv_gravity = null;
	private ImageView iv_gypro = null;
	private ImageView iv_light = null;
	private ImageView iv_magnt = null;
	private ImageView iv_orien = null;
	private ImageView iv_pressure = null;
	private ImageView iv_tempe = null;
	private ImageView iv_voice = null;
	
	private List<Sensor> l_s = null;
	private SensorManager sm = null;
	
	private void initOnClickList()
	  {
	    if (this.l_s.contains(this.sm.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)))
	    {
	      this.iv_gravity.setBackgroundResource(R.drawable.main_gravity);
	      this.iv_gravity.setOnClickListener(new IVOnClickListener());
	    }
	    if (this.l_s.contains(this.sm.getDefaultSensor(Sensor.TYPE_LIGHT)))
	    {
	      this.iv_light.setBackgroundResource(R.drawable.main_light);
	      this.iv_light.setOnClickListener(new IVOnClickListener());
	    }
	    if (this.l_s.contains(this.sm.getDefaultSensor(Sensor.TYPE_ORIENTATION)))
	    {
	      this.iv_orien.setBackgroundResource(R.drawable.main_orien);
	      this.iv_orien.setOnClickListener(new IVOnClickListener());
	    }
	    if (this.l_s.contains(this.sm.getDefaultSensor(Sensor.TYPE_PROXIMITY)))
	    {
	      this.iv_dist.setBackgroundResource(R.drawable.main_dist);
	      this.iv_dist.setOnClickListener(new IVOnClickListener());
	    }
	    if (this.l_s.contains(this.sm.getDefaultSensor(Sensor.TYPE_TEMPERATURE)))
	    {
	      this.iv_tempe.setBackgroundResource(R.drawable.main_tempe);
	      this.iv_tempe.setOnClickListener(new IVOnClickListener());
	    }
	    if (this.l_s.contains(this.sm.getDefaultSensor(Sensor.TYPE_GYROSCOPE)))
	    {
	      this.iv_gypro.setBackgroundResource(R.drawable.main_gyro);
	      this.iv_gypro.setOnClickListener(new IVOnClickListener());
	    }
	    if (this.l_s.contains(this.sm.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD)))
	    {
	      this.iv_magnt.setBackgroundResource(R.drawable.main_magn);
	      this.iv_magnt.setOnClickListener(new IVOnClickListener());
	    }
	    if (this.l_s.contains(this.sm.getDefaultSensor(Sensor.TYPE_PRESSURE)))
	    {
	      this.iv_pressure.setBackgroundResource(R.drawable.main_pressure);
	      this.iv_pressure.setOnClickListener(new IVOnClickListener());
	    }
	    this.iv_voice.setBackgroundResource(R.drawable.main_voice);
	    this.iv_voice.setOnClickListener(new IVOnClickListener());
	  }
	
	/** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        Log.i(TAG,"onCreate...");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        this.iv_gravity = ((ImageView)findViewById(R.id.iv_gravity));
        this.iv_light = ((ImageView)findViewById(R.id.iv_light));
        this.iv_orien = ((ImageView)findViewById(R.id.iv_orien));
        this.iv_dist = ((ImageView)findViewById(R.id.iv_dist));
        this.iv_tempe = ((ImageView)findViewById(R.id.iv_tempe));
        this.iv_gypro = ((ImageView)findViewById(R.id.iv_gyro));
        this.iv_voice = ((ImageView)findViewById(R.id.iv_voice));
        this.iv_magnt = ((ImageView)findViewById(R.id.iv_magn));
        this.iv_pressure = ((ImageView)findViewById(R.id.iv_pressure));
        
        this.sm = ((SensorManager)getSystemService("sensor"));
        this.l_s = this.sm.getSensorList(-1);
        
        initOnClickList();
        
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
    }
    
    @Override
    public void onResume(){
    	Log.i(TAG,"onResume...");
    	super.onResume();
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
    }
	

    private class IVOnClickListener implements View.OnClickListener
    {
        private IVOnClickListener()
        {
        }

        public void onClick(View paramView)
        {
          if (paramView.equals(SensorActivity.this.iv_gravity))
          {
            Intent intent = new Intent(SensorActivity.this, gSensorActivity.class);
            SensorActivity.this.startActivity(intent);
          }
          if (paramView.equals(SensorActivity.this.iv_gypro))
          {
            Intent intent = new Intent(SensorActivity.this, gyroSensorActivity.class);
            SensorActivity.this.startActivity(intent);
          }
          if (paramView.equals(SensorActivity.this.iv_magnt))
          {
            Intent intent = new Intent(SensorActivity.this, compassSensorActivity.class);
            SensorActivity.this.startActivity(intent);
          }
        }
 
    }      
}



