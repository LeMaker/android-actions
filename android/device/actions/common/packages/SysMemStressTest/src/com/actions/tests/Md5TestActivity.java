package com.actions.tests;

import java.io.File;
import java.io.FileInputStream;
import java.io.DataOutputStream;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.Map;


import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import java.lang.Process;

public class Md5TestActivity extends Activity {
	private Button mSimpleReadBtn = null;
	private EditText mFilePathView;
	private String mTestPath;
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        switchToRoot();
        mSimpleReadBtn = (Button)findViewById(R.id.SimpleReadTest);
        mFilePathView  = (EditText)findViewById(R.id.filepath);
        
        mSimpleReadBtn.setOnClickListener(simpleReadTestListener);
        mFilePathView.setText("/system");
        updateViewEnable(true);
        //new Thread(	new simpleFileTest("/mnt/sdcard/test_2M", true)).start();
    }
    
    private void switchToRoot(){
    	Log.d("Md5", "request su");
    	try{
	    	Process process = Runtime.getRuntime().exec("su");
	    	Log.d("Md5", "request  fullfilled");
	     	//DataOutputStream os = new DataOutputStream(process.getOutputStream());
	     	//os.writeBytes("exit\n");
	     	//os.flush();
	    }catch(Exception e){
	    	Log.d("Md5", "su failed " +e );
	    }

    }
    public void updateViewEnable(boolean enable){
    	mFilePathView.setEnabled(enable);
    	mSimpleReadBtn.setEnabled(enable);
    	//mSimpleReadBtn.setVisibility(View.INVISIBLE);
    }
    
    protected void onDestroy(){
    	super.onDestroy();
    	System.exit(0);
    }
    
    private OnClickListener simpleReadTestListener = new OnClickListener(){

		public void onClick(View v) {
			updateViewEnable(false);
			// TODO Auto-generated method stub
			String path = mFilePathView.getText().toString().trim();
			new Thread(	new simpleFileTest(path, true)).start();
		}
    };
}


class simpleFileTest implements Runnable {
	private Map<String, String> mMd5Maps = null;
	String mDir = null;
	boolean mListChild = false;
	simpleFileTest(String fileName, boolean listChild){
		mDir = fileName;
		mListChild = listChild;
	}
	public void run() {
		int i = 0;
		while(true){
			File tmpFile = new File(mDir);
			// Generate md5
			if(mMd5Maps == null){
				mMd5Maps = Md5Class.getDirMD5(tmpFile, mListChild);		
			}
			
			// check md5
			File tmpFile2 = new File(mDir);
			if(Md5Class.checkDirMd5Valid(tmpFile2, mMd5Maps, mListChild)){
				//Log.v("Md5", "Test Pass: round="+i++);
			} else {
				Log.e("Md5", "Failed test, exit...");
				break;
			}
		}
	}	
	
	
}

