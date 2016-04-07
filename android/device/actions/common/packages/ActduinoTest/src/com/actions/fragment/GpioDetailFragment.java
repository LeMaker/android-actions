package com.actions.fragment;

import android.annotation.SuppressLint;
import android.app.Fragment;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;
import com.actions.actduinotest.ActduinoTestActivity;
import com.actions.actduinotest.R;
import com.actions.jni.gpio;
import com.actions.utils.Utilities;


/**
 * Description:
 * this is for gpio test
 *  1.setGPIO 2.testGPIO
 *********************************************   
 *ActionsCode(author:jiangjinzhang, new_code)
 * @version 1.0
 */

@SuppressLint("ShowToast")
public class GpioDetailFragment extends Fragment {
	private static final boolean DEBUG = true;
	private static final String TAG = "ActduinoTest.GpioDetailFragment";
	public static TextView tvResult;
	private static String[] gpio_string;

	enum State {
		NORMAL, TESTING, TESTED
	};

	private State mState = State.NORMAL;

	private static boolean flag_isOK = true;// true:test pass;false:test fail
	private boolean[] flag_isOn ={false,false,false,false};//test result for each led 

	private Button btn_gpio_test;
	private EditText et_on_off;
	private ImageView[] iv_led = new ImageView[4];
	private static gpio mGPIO = null;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (DEBUG)
			Log.d(TAG, "onCreate");
		try {
			mGPIO = gpio.newInstance();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			Log.w(TAG, "new mGPIO fail");
		}
		gpio_string = (String[]) Utilities.gpio_pin
				.toArray(new String[Utilities.gpio_pin.size()]);
	
		mState = State.NORMAL;

	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {

		if (DEBUG)
			Log.d(TAG, "enter onCreateView");
		View rootView = inflater.inflate(R.layout.fragment_gpio_detail,
				container, false);
		btn_gpio_test = (Button) rootView.findViewById(R.id.btn_gpio_test);
		et_on_off = (EditText) rootView.findViewById(R.id.et_on_off);
		iv_led[0] = (ImageView) rootView.findViewById(R.id.iv_led0);
		iv_led[1] = (ImageView) rootView.findViewById(R.id.iv_led1);
		iv_led[2] = (ImageView) rootView.findViewById(R.id.iv_led2);
		iv_led[3] = (ImageView) rootView.findViewById(R.id.iv_led3);
		((TextView) rootView.findViewById(R.id.gpio_title))
				.setText(R.string.GPIO_TEST);
		tvResult = (TextView) rootView.findViewById(R.id.gpio_desc);
		btn_gpio_test.setClickable(true);

		if (mState == State.TESTING) {
			tvResult.setText(R.string.pls_wait);
			btn_gpio_test.setClickable(false);
		} else if (mState == State.TESTED) {
			setResult();
		} else {
			tvResult.setText(R.string.pls_push_the_button);
		}
		//make the image keep the form state when creates view
		for (int i = 0; i < gpio_string.length /2; i++) {
			if (flag_isOn[i]) {
				iv_led[i]
						.setImageResource(R.drawable.lightbulb_on);
			}else {
				iv_led[i]
						.setImageResource(R.drawable.lightbulb_off);
			}
		}
	
		btn_gpio_test.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub

				if (DEBUG)
					Log.d(TAG, "-------TESTING-----");
				int on_off=-1;
				if (et_on_off.getText().toString().matches("[0-1]")) {
					on_off= Integer.parseInt(et_on_off.getText().toString());
				}
				 
				if (on_off == 0 | on_off == 1) {

					updateState(State.TESTING);
					int ret = 0;
					for (int i = 0; i < gpio_string.length; i += 2) {
						mGPIO.setGPIO(gpio_string[i], gpio_string[i + 1]);
						mGPIO.request_GPIO();
						if (on_off == 0) {
							mGPIO.set_off_GPIO();//set off the led
						} else {
							mGPIO.set_on_GPIO();//set on the led
						}
						ret = mGPIO.read_on_off_GPIO();//get the voltage 
						Log.d(TAG, "read result:ret=" + ret);
						//according to the voltage  the image was set
						if (ret > 0) {
							iv_led[i / 2]
									.setImageResource(R.drawable.lightbulb_on);
							flag_isOn[i / 2]=true;

						} else {
							iv_led[i / 2]
									.setImageResource(R.drawable.lightbulb_off);
							flag_isOn[i / 2]=false;
						}
						
						flag_isOK=flag_isOK & (on_off==1 ?  ret>0:ret==0);
					}
					updateState(State.TESTED);
					if (DEBUG)
						Log.d(TAG, "----TESTED----");
					

				}else {
					Toast.makeText(getActivity(), R.string.pls_input_0_or_1, Toast.LENGTH_SHORT).show();
				}
			}
		});

		return rootView;
	}

	private void updateState(State newstate) {
		if (DEBUG)
			Log.d(TAG, "---updateState");
		mState = newstate;
		if (newstate == State.TESTING) {
			tvResult.setText(getString(R.string.pls_wait));
			btn_gpio_test.setClickable(false);
			if (DEBUG)
				Log.d(TAG, "-tvResult.setText(R.string.pls_wait)");

		} else if (newstate == State.TESTED) {
			btn_gpio_test.setClickable(true);
			setResult();
		} else {
			btn_gpio_test.setClickable(true);
			tvResult.setText(R.string.pls_push_the_button);
		}
	}

	private void setResult() {
		if (DEBUG)
			Log.d(TAG, "---setResult");
		if (flag_isOK) {
			tvResult.setText(R.string.result_sucess);
			tvResult.setTextColor(Color.GREEN);
		} else {
			tvResult.setText(R.string.result_fail);
			tvResult.setTextColor(Color.RED);
		}

	}

	

}
