package com.actions.fragment;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;

import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.actions.actduinotest.R;
import com.actions.jni.uart;
import com.actions.utils.Utilities;

/**
 * Description: this is for uart test
 ********************************************* 
 * ActionsCode(author:jiangjinzhang, new_code)
 * 
 * @version 1.0
 */
public class UartDetailFragment extends Fragment {

	public static final String ITEM_ID = "item_id";
	private static final boolean DEBUG = true;
	private static final String TAG = "UartDetailFragment";
	public static String send;
	public static StringBuffer receive = null;
	public EditText etInput;
	public TextView tvShowReceive;
	private ReadThread mReadThread;
	private Spinner sp_baudrate = null;
	private Spinner sp_data = null;
	private Spinner sp_check = null;
	private Spinner sp_stop = null;
	private boolean run_flag;
	private String lock =new String("lock");


	private FileDescriptor mFd;
	private FileInputStream mFileInputStream;
	private FileOutputStream mFileOutputStream;
	private String uartport = null;
	private MyReceiver mr = null;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		if (DEBUG)
			Log.d(TAG, "onCreate");
		uartport = Utilities.uart_port;
		run_flag = true;

	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState) {
		if (DEBUG)
			Log.d(TAG, "onCreateView()");
		View rootView = inflater.inflate(R.layout.fragment_uart_detail,
				container, false);
		Button btn_uart_test = (Button) rootView
				.findViewById(R.id.btn_uart_test);
		btn_uart_test.setText("Send");
		((TextView) rootView.findViewById(R.id.uart_title))
				.setText("UART TEST");
		((TextView) rootView.findViewById(R.id.uart_desc))
				.setText("pls push the button");
		etInput = (EditText) rootView.findViewById(R.id.input_parament_uart);
		tvShowReceive = (TextView) rootView
				.findViewById(R.id.tvShowReceiveuart);
		sp_baudrate = (Spinner) rootView.findViewById(R.id.sp_baudrate);
		sp_data = (Spinner) rootView.findViewById(R.id.sp_data);
		sp_check = (Spinner) rootView.findViewById(R.id.sp_check);
		sp_stop = (Spinner) rootView.findViewById(R.id.sp_stop);

		mr = new MyReceiver();
		IntentFilter intentf = new IntentFilter();
		intentf.addAction("com.actions.RECEIVE");
		getActivity().registerReceiver(mr, intentf);

		btn_uart_test.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View arg0) {
				// TODO Auto-generated method stub

				if (DEBUG)
					Log.d(TAG, "btn_uart_test setOnClickListener");
				if (DEBUG)
					Log.d(TAG,
							"Integer.parseInt(uartport.substring(uartport.length()-1, uartport.length())"
									+ Integer.parseInt(uartport.substring(
											uartport.length() - 1,
											uartport.length())));
				if (mFd != null) {
					if (mReadThread != null) {
						run_flag = false;
						// ReadThread.interrupted();
						mReadThread = null;
					}
					mFd = null;
					synchronized(lock){
					mFileInputStream = null;
					}
					uart.close11();

				}
				if (etInput.getText().toString().length() == 0) {
					Toast.makeText(getActivity(), R.string.warning_empty,
							Toast.LENGTH_LONG).show();
				} else {

					mFd = uart.openuart11(Integer.parseInt(uartport.substring(
							uartport.length() - 1, uartport.length())),
							Integer.parseInt(sp_baudrate.getSelectedItem()
									.toString()), Integer.parseInt(sp_data
									.getSelectedItem().toString()), sp_check
									.getSelectedItem().toString().charAt(0),
							Integer.parseInt(sp_stop.getSelectedItem()
									.toString()));
					if (mFd == null) {
						Log.e(TAG, "native open returns null");
						Toast.makeText(getActivity(),
								R.string.pls_check_ttySx_exsit,
								Toast.LENGTH_LONG).show();

					} else {
						run_flag = true;
						mFileInputStream = new FileInputStream(mFd);
						mReadThread = new ReadThread();
						mReadThread.start();
						send = etInput.getText().toString();
						mFileOutputStream = new FileOutputStream(mFd);
						try {
							mFileOutputStream.write(send.getBytes());
							mFileOutputStream.write('\n');

							mFileOutputStream.close();
						} catch (IOException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
					}
				}
			}
		});
		receive = new StringBuffer();

		return rootView;
	}

	public class MyReceiver extends BroadcastReceiver {

		@Override
		public void onReceive(Context arg0, Intent arg1) {
			// TODO Auto-generated method stub
			if (DEBUG)
				Log.d(TAG, " uart  myreceiver start ");
			Bundle bundle = new Bundle();
			bundle = arg1.getExtras();
			receive.append(bundle.getChar("key_Result_uart"));
			tvShowReceive.setText(receive);
		}
	}

	@Override
	public void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		if (DEBUG)
			Log.d(TAG, "------------onDestroy-----------");
		if (mReadThread != null) {
			run_flag = false;
			ReadThread.interrupted();
		}
		if (mReadThread == null)
			Log.d(TAG, "mReadThread==null");
		if (mFileOutputStream != null)
			try {
				mFileOutputStream.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		if (mFileInputStream != null)
			try {
				mFileInputStream.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		getActivity().unregisterReceiver(mr);
	}

	private class ReadThread extends Thread {

		@Override
		public void run() {
			super.run();
			if (DEBUG)
				Log.d(TAG, "enter  thread"+Thread.currentThread().getId());
			while (run_flag) {
				int size;
				try {
					byte[] buffer = new byte[64];
					synchronized(lock){
					if (mFileInputStream == null) {
						Log.d(TAG,
								"mFileInputStream == null  thread Interrupted and return");
						return;
					}
					size = mFileInputStream.read(buffer);
					}
					if (size > 0) {
						Intent intent = new Intent("com.actions.RECEIVE");
						if (DEBUG)
							Log.d(TAG, "receive string :" + buffer);
						Bundle bundle = new Bundle();
						bundle.putChar("key_Result_uart",
								(char) buffer[size - 1]);
						intent.putExtras(bundle);
						getActivity().sendBroadcast(intent);
					}
				} catch (IOException e) {
					e.printStackTrace();
					Log.d(TAG, " IOException  thread Interrupted and return");
					return;
				}
			}
			if (DEBUG)
				Log.d(TAG, "thread Interrupted and return");

			return;
		}
	}
}
