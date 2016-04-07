package com.actions.fragment;


import com.actions.actduinotest.R;
import com.actions.jni.spi;
import com.actions.utils.Utilities;

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
 * this is for spi test
 *1.write spi  2.read spi
 *********************************************   
 *ActionsCode(author:jiangjinzhang, new_code)
 * @version 1.0
 */
public class SpiDetailFragment extends Fragment {

	public static boolean flag_isOK = false;
	public TextView tvResult;
	public TextView tvShowRead;
	public EditText etInput;
	private Button btn_spi_test;
	enum State {
		NORMAL, TESTING, TESTED
	};

	private State mState = State.NORMAL;
	private static spi mSPI = null;
	public static String input, result;
	private String spiport = null;
	private static final boolean DEBUG = true;
	private static final String TAG = "SpiDetailFragment";

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (DEBUG)
			Log.d(TAG, "onCreate");
		spiport=Utilities.spi_port;
		if (DEBUG)
			Log.d(TAG, "spiport="+spiport);
		try {
			mSPI = spi.newInstance(Integer.parseInt(spiport.substring(spiport.length()-1, spiport.length())));
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
		View rootView = inflater.inflate(R.layout.fragment_spi_detail,
				container, false);
		 btn_spi_test = (Button) rootView.findViewById(R.id.btn_spi_test);
		btn_spi_test.setText("Send");
		((TextView) rootView.findViewById(R.id.spi_title)).setText("SPI TEST");
		tvResult = (TextView) rootView.findViewById(R.id.spi_desc);
		tvShowRead = (TextView) rootView.findViewById(R.id.tvShowRead);
		etInput = (EditText) rootView.findViewById(R.id.input_parament);
		
		if (mState == State.TESTING) {
			tvResult.setText(R.string.pls_wait);
			btn_spi_test.setClickable(false);
		} else if (mState == State.TESTED) {
			setResult();
		} else {
			tvResult.setText(R.string.pls_push_the_button);
		}		
		btn_spi_test.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
				if (DEBUG)
					Log.d(TAG, "btn_spi_test setOnClickListener");
				
				if (etInput.getText().toString().length() == 0) {
					Toast.makeText(getActivity(), R.string.warning_empty,
							Toast.LENGTH_LONG).show();
				} else {
					updateState(State.TESTING);
						input = etInput.getText().toString();
						int ret = 0;
						mSPI.setmode();
						ret = mSPI.writespi(input, input.length());
						if (DEBUG)
							Log.d(TAG, "字符串长队度为：" + input.length() + "  已写入"
									+ ret);
						String raw_result = mSPI.readspi(ret>input.length() ? ret:input.length());
						if (raw_result.length()>=input.length()) {
							result=raw_result.substring(0, input.length());//string that from jni may need dispose
							Log.d(TAG, "input:"+input);
							Log.d(TAG, "result:"+result);
							Log.d(TAG, "raw_result:"+raw_result);
						flag_isOK = compare(input, result);
						}else if (raw_result.length()==0) {
							flag_isOK =false;
							Toast.makeText(getActivity(), R.string.check_device_was_installed,
									Toast.LENGTH_LONG).show();
					} else {
							flag_isOK =false;
							result=raw_result;
							Toast.makeText(getActivity(), R.string.warning_mixed_encoding,
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
			btn_spi_test.setClickable(false);
			tvResult.setText(R.string.pls_wait);
		} else if (newstate == State.TESTED) {
			btn_spi_test.setClickable(true);
			tvShowRead.setText(result);
			setResult();
		} else {
			btn_spi_test.setClickable(true);
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

	private boolean compare(String input, String result) {
		// TODO Auto-generated method stub
		if (input.equals(result)) {
			return true;
		}
		return false;
	}

}
