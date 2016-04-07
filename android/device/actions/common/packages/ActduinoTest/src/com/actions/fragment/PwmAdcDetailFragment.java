package com.actions.fragment;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Fragment;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import com.actions.actduinotest.R;
import com.actions.jni.adc;
import com.actions.jni.pwm;
import com.actions.utils.Utilities;

/**
 * Description: this is for adc test 1.request adc channle 2.get value
 ********************************************* 
 * ActionsCode(author:jiangjinzhang, new_code)
 * 
 * @version 1.0
 */
@TargetApi(Build.VERSION_CODES.HONEYCOMB)
@SuppressLint("NewApi")
public class PwmAdcDetailFragment extends Fragment {

	public static boolean flag_isOK = false;
	public static int adc_val = 0;
	public static int adc_val_ideal = 0;
	public TextView tvResult;
	public TextView tvResult_real;
	public TextView tvResult_ideal;
	public EditText et_duty;
	public EditText et_period;
	private Button btn_pwmadc_test;
	public int channle = -1;
	public int pwm_port = -1;

	enum State {
		NORMAL, TESTING, TESTED
	};

	private State mState = State.NORMAL;
	private static adc mADC = null;
	private static pwm mPWM = null;
	private static final boolean DEBUG = true;
	private static final String TAG = "PwmAdcDetailFragment";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (DEBUG)
			Log.d(TAG, "onCreate");
		Log.d(TAG, "Utilities.adc_port" + Utilities.adc_port);
		// get channle num
		if (Utilities.adc_port.equals("REMCON")) {
			channle = 10;// REMCON
		} else {
			Log.d(TAG, Utilities.adc_port.substring(2,
					Utilities.adc_port.length()));
			channle = Integer.parseInt(Utilities.adc_port.substring(3,
					Utilities.adc_port.length()));// AUX0-3
		}
		if (Utilities.pwm_port.startsWith("PWM")) {
			Log.d(TAG, Utilities.pwm_port.substring(2,
					Utilities.adc_port.length()));
			pwm_port = Integer.parseInt(Utilities.adc_port.substring(3,
					Utilities.adc_port.length()));// PWM0-5
		} else {
			pwm_port = 8;
		}
		try {
			mADC = adc.newInstance();
			mPWM = pwm.newInstance();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {

		if (DEBUG)
			Log.d(TAG, "onCreateView()");
		View rootView = inflater.inflate(R.layout.fragment_pwm_adc_detail,
				container, false);
		btn_pwmadc_test = (Button) rootView.findViewById(R.id.btn_pwmadc_test);
		((TextView) rootView.findViewById(R.id.pwmadc_title))
				.setText("PWM&&ADC TEST");
		tvResult = (TextView) rootView.findViewById(R.id.pwmadc_desc);
		tvResult_real = (TextView) rootView.findViewById(R.id.result_real);
		tvResult_ideal = (TextView) rootView.findViewById(R.id.result_ideal);
		et_duty = (EditText) rootView.findViewById(R.id.et_input_duty);
		et_period = (EditText) rootView.findViewById(R.id.et_input_period);
		if (mState == State.TESTING) {
			tvResult.setText((getString(R.string.pls_wait)));
			btn_pwmadc_test.setClickable(false);
		} else if (mState == State.TESTED) {
			tvResult_real.setText(getString(R.string.value_real)
					+ String.valueOf(adc_val));
			setResult();
		} else {
			tvResult.setText(getString(R.string.pls_push_the_button));
		}
		btn_pwmadc_test.setOnClickListener(new OnClickListener() {

			@SuppressLint("NewApi")
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				if (DEBUG)
					Log.d(TAG, "btn_pwmadc_test setOnClickListener");
				if (et_duty.getText().toString().length() == 0
						| et_period.toString().length() == 0) {
					Toast.makeText(getActivity(), getString(R.string.warning_empty),
							Toast.LENGTH_LONG).show();
				}else if (( ! (Integer.parseInt(et_period.getText().toString())>=20 && Integer.parseInt(et_period.getText().toString())<512&& Integer.parseInt(et_duty.getText().toString())>0)))  {
					
					Toast.makeText(getActivity(), getString(R.string.warning_format),
							Toast.LENGTH_LONG).show();
					
				}else{

					tvResult.setText(getString(R.string.pls_wait));


					updateState(State.TESTING);
					try {
						mPWM.sendcmdpwm(pwm_port, cal(Integer.parseInt(et_period.getText().toString()), Integer.parseInt(et_duty.getText().toString())));
						Thread.sleep(100);
						mADC.configrequest(channle, 0);// request channle

						Thread.sleep(100);
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					adc_val = mADC.getvalue(12, 0);// read channle
					Log.d(TAG, "adc_val=" + adc_val);
					if (adc_val >=0) {
						flag_isOK = true;
					}
					tvResult_real.setText(getString(R.string.value_real)
							+ String.valueOf(adc_val));
					setResult();
					updateState(State.TESTED);
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
			btn_pwmadc_test.setClickable(false);
			tvResult.setText(getString(R.string.pls_wait));
		} else if (newstate == State.TESTED) {
			btn_pwmadc_test.setClickable(true);
			setResult();
		} else {
			btn_pwmadc_test.setClickable(true);
			tvResult.setText(getString(R.string.pls_push_the_button));
		}
	}

	protected void setResult() {
		// TODO Auto-generated method stub
		if (flag_isOK) {
			tvResult.setText(getString(R.string.result_sucess));
			tvResult.setTextColor(Color.GREEN);
		} else {
			tvResult.setText(getString(R.string.result_fail));
			tvResult.setTextColor(Color.RED);

		}

	}
	private int cal(int period,int duty){
		
		int high = period *duty /100;
		int low=period-high;
		if (DEBUG)
			Log.d(TAG, "cal res="+((high<<8) +low));
		return (high<<8) +low;
		
	}
}
