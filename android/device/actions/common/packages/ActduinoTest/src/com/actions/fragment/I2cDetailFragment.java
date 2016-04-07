package com.actions.fragment;

import com.actions.actduinotest.R;
import com.actions.jni.i2c;
import com.actions.utils.Utilities;

import android.annotation.SuppressLint;
import android.app.Fragment;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Description:
 * this is for i2c test
 * 1.write i2c  2.read i2c
 *********************************************   
 *ActionsCode(author:jiangjinzhang, new_code)
 * @version 1.0
 */
public class I2cDetailFragment extends Fragment {

	public static boolean flag_isOK = false;// true:test pass;false:test fail
	public TextView tvResult;
	public TextView tvShowRead;
	public EditText etInput;
	private Button btn_i2c_test;

	enum State {
		NORMAL, TESTING, TESTED
	};

	private State mState = State.NORMAL;
	private static i2c mI2C = null;
	public static String input, result;
	private String i2cport = null;
	private static final boolean DEBUG = true;
	private static final String TAG = "I2cDetailFragment";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (DEBUG)
			Log.d(TAG, "onCreate");
		i2cport=Utilities.i2c_port;
		if (DEBUG)
			Log.d(TAG, "spiport="+i2cport);
		try {
			mI2C = i2c.newInstance(Integer.parseInt(i2cport.substring(i2cport.length()-1, i2cport.length())));
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
		View rootView = inflater.inflate(R.layout.fragment_i2c_detail,
				container, false);
		btn_i2c_test = (Button) rootView.findViewById(R.id.btn_i2c_test);
		btn_i2c_test.setText("Send");
		((TextView) rootView.findViewById(R.id.i2c_title)).setText("I2C TEST");
		tvResult = (TextView) rootView.findViewById(R.id.i2c_desc);
		tvShowRead = (TextView) rootView.findViewById(R.id.tvShowReadi2c);
		etInput = (EditText) rootView.findViewById(R.id.input_paramenti2c);

		if (mState == State.TESTING) {
			tvResult.setText(R.string.pls_wait);
			btn_i2c_test.setClickable(false);
		} else if (mState == State.TESTED) {
			setResult();
		} else {
			tvResult.setText(R.string.pls_push_the_button);
		}

		btn_i2c_test.setOnClickListener(new OnClickListener() {
			@SuppressLint("ShowToast")
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				if (DEBUG)
					Log.d(TAG, "btn_i2c_test setOnClickListener");
				if (etInput.getText().toString().length() == 0) {
					Toast.makeText(getActivity(), R.string.warning_empty,
							Toast.LENGTH_LONG).show();
				} else {
					updateState(State.TESTING);
					input = etInput.getText().toString();
					int ret = 0;
					mI2C.setmodei2c();
					ret = mI2C.writei2c(input, input.length());//write string to eprom by i2c
					if (DEBUG)
						Log.d(TAG, "字符串长队度为：" + input.length() + "  已写入" + ret);
					String raw_result = mI2C.readi2c(ret > input.length() ? ret
							: input.length());//read string from eprom by i2c
					if (raw_result.length() >= input.length()) {
						result = raw_result.substring(0, input.length());// string that from jni may need dispose
						Log.d(TAG, "input:" + input);
						Log.d(TAG, "result:" + result);
						Log.d(TAG, "raw_result:" + raw_result);
						flag_isOK = compare(input, result);
					} else if (raw_result.length() == 0) {
						flag_isOK = false;
						Toast.makeText(getActivity(),
								R.string.check_device_was_installed,
								Toast.LENGTH_LONG).show();
					} else {
						flag_isOK = false;
						result = raw_result;
						Toast.makeText(getActivity(),
								R.string.warning_mixed_encoding,
								Toast.LENGTH_LONG).show();
					}
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
			btn_i2c_test.setClickable(false);
			tvResult.setText(R.string.pls_wait);
		} else if (newstate == State.TESTED) {
			btn_i2c_test.setClickable(true);
			tvShowRead.setText(result);
			setResult();
		} else {
			btn_i2c_test.setClickable(true);
			tvResult.setText(R.string.pls_push_the_button);
		}
	}

	protected void setResult() {
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

	protected boolean compare(String input2, String result2) {
		// TODO Auto-generated method stub
		if (input2.equals(result2)) {
			return true;
		}
		return false;
	}

}
