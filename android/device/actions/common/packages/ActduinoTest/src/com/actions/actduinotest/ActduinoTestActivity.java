package com.actions.actduinotest;

import com.actions.fragment.GpioDetailFragment;
import com.actions.fragment.I2cDetailFragment;
import com.actions.fragment.PwmAdcDetailFragment;
import com.actions.fragment.SpiDetailFragment;
import com.actions.fragment.HeaderListFragment;
import com.actions.fragment.UartDetailFragment;
import com.actions.jni.adc;
import com.actions.jni.gpio;
import com.actions.jni.i2c;
import com.actions.jni.spi;
import com.actions.utils.Utilities;

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;

/**
 * Description: this is the main activity
 ********************************************* 
 * ActionsCode(author:jiangjinzhang, new_code)
 * 
 * @version 1.0
 */

public class ActduinoTestActivity extends Activity implements
		HeaderListFragment.Callbacks {
	private static final String TAG = "ActduinoTest.SelectTestActivity";
	private static final boolean DEBUG = true;
	private Context mCotext = this;

	private final int GPIO_CASE = 1;
	private final int UART_CASE = 2;
	private final int I2C_CASE = 3;
	private final int SPI_CASE = 4;
	private final int PWM_ADC_CASE = 5;
	private static GpioDetailFragment mGpioDetailFragment;
	private static I2cDetailFragment mI2cDetailFragment;
	private static PwmAdcDetailFragment mPwmAdcDetailFragment;
	private static SpiDetailFragment mSpiDetailFragment;
	private static UartDetailFragment mUartDetailFragment;
	private static Fragment fragment;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (DEBUG) Log.d(TAG, "onCreate");
		setContentView(R.layout.activity_test_twopane);
		mGpioDetailFragment = null;
		mI2cDetailFragment = null;
		mPwmAdcDetailFragment = null;
		mSpiDetailFragment = null;
		mUartDetailFragment = null;
		fragment = null;
		
		//get test configs
		new Utilities(mCotext).readConfigXml();
		
		//insmod drivers and change device file properties
		new Utilities(mCotext).chmoddevices();

		fragment = new GpioDetailFragment();
		getFragmentManager().beginTransaction()
				.replace(R.id.test_detail_container, fragment).commit();

	}

	@Override
	public void onItemSelected(Integer id) {
		if (DEBUG) {
			Log.d(TAG, "fragment " + fragment + " id " + id);
		}

		switch (id) {
		case GPIO_CASE:
			if (null == mGpioDetailFragment) {
				mGpioDetailFragment = new GpioDetailFragment();
				Log.d(TAG, "null==mGpioDetailFragment");
			}
			fragment = mGpioDetailFragment;
			break;
		case I2C_CASE:
			if (null == mI2cDetailFragment) {
				mI2cDetailFragment = new I2cDetailFragment();
				Log.d(TAG, "null==mI2cDetailFragment");
			}
			fragment = mI2cDetailFragment;
			break;
		case SPI_CASE:
			if (null == mSpiDetailFragment) {
				mSpiDetailFragment = new SpiDetailFragment();
			}
			fragment = mSpiDetailFragment;
			break;
		case UART_CASE:
			if (null == mUartDetailFragment) {
				mUartDetailFragment = new UartDetailFragment();
				Log.d(TAG, "null==mUartDetailFragment");
			}
			fragment = mUartDetailFragment;
			break;
		case PWM_ADC_CASE:
			if (null == mPwmAdcDetailFragment) {
				mPwmAdcDetailFragment = new PwmAdcDetailFragment();
			}
			fragment = mPwmAdcDetailFragment;
			break;

		default:
			Log.w(TAG, "UNDEFINED CASE");
			break;
		}

		getFragmentManager().beginTransaction()
				.replace(R.id.test_detail_container, fragment).commit();
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		 mGpioDetailFragment = null;
		mI2cDetailFragment = null;
		mPwmAdcDetailFragment = null;
		mSpiDetailFragment = null;
		mUartDetailFragment = null;
		fragment = null;
		Log.d(TAG," ALL==null");


		if (gpio.mGPIO != null) {
			gpio.mGPIO.close();
			gpio.mGPIO = null;
		}
		if (adc.mADC != null) {
			adc.mADC.close();
			adc.mADC = null;
		}
		if (spi.mSPI != null) {
			spi.mSPI.close();
			spi.mSPI = null;
		}
		if (i2c.mI2C != null) {
			i2c.mI2C.close();
			i2c.mI2C = null;
		}

		

	}
}