package com.actions.explore;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.Reader;
import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.format.Time;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.EditText;
import android.widget.Toast;
import android.util.Log;

/*@author lizihao@actions-semi.com*/

public class CreateFile extends Activity {

	private static final int MENU_SAVE = 0x00;
	private static final int MENU_CLOSE = 0x01;
	
	private static final int LOAD_SUCCESS = 1;
	private static final int LOAD_FAIL = 0;
	
	private ProgressDialog ScanWaitDialog;
	
	private EditText mEditText;
	
	public String mTmpString;
	
	final static String FOLDER = "/note/";
	final static String SUFFIX = ".txt";
	
	public static long mTxtSize = 0;
	public String mReadFileName;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.create_file);
		ScanWaitDialog = new ProgressDialog(this);
		mEditText = (EditText) findViewById(R.id.create_file);
		mEditText.clearFocus();
		if(getIntent().getExtra("type").equals("read")){
			mReadFileName = getFileName((String)getIntent().getExtra("path"));
			setTitle(mReadFileName + SUFFIX);
	        Show_ProgressDialog();
			ReadTxt mReadTxt= new ReadTxt((String)getIntent().getExtra("path"));
			mReadTxt.start();
		}
	}
	
	// wait progress
    private void Show_ProgressDialog() {
        ScanWaitDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        ScanWaitDialog.setTitle(getResources().getString(R.string.please_wait));
        ScanWaitDialog.setMessage(getResources().getString(R.string.loading));
        ScanWaitDialog.setCancelable(true);
        ScanWaitDialog.setOnCancelListener(new OnCancelListener(){
   	        public void onCancel(DialogInterface arg0) {
   	        	finish();
   	     }});
        ScanWaitDialog.show();
    }
	
	private Handler handler = new Handler() {

		@Override
		public void handleMessage(Message msg) {
			// TODO Auto-generated method stub
			if(msg.arg1 == LOAD_SUCCESS){
				mEditText.setText(mTmpString);
				mTxtSize = msg.arg2;
				ScanWaitDialog.dismiss();
			} else if(msg.arg1 == LOAD_FAIL){
				ScanWaitDialog.dismiss();
				finish();
			}
		}
		
	};
	
	public String getFileName(String mPath){
		return mPath.substring(mPath.lastIndexOf("/")+1, mPath.lastIndexOf("."));
	}
	
	public String getFilePath(){
		if(getIntent().getExtra("type").equals("read")){
			String mPath = (String)getIntent().getExtra("path");
			mPath = mPath.substring(0, mPath.lastIndexOf("/")+1);
			return mPath;
		} else if(getIntent().getExtra("type").equals("create")){
			String mPath = (String)getIntent().getExtra("path");
			return mPath;
		}
		return "";
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// TODO Auto-generated method stub
		menu.add(0, MENU_SAVE, 0, getResources().getString(R.string.save));
		menu.add(0, MENU_CLOSE, 0, getResources().getString(R.string.close));
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// TODO Auto-generated method stub
		switch (item.getItemId()) {
		case MENU_SAVE:
			if((getIntent().getExtra("type").equals("create")) && (mEditText.getText().length() != 0)){
				saveFileToDir("create");
			} else if((getIntent().getExtra("type").equals("read")) && (mTxtSize != mEditText.getText().length())){
				saveFileToDir("read");
			} else {
				mTxtSize = 0;
				Toast.makeText(this, getResources().getString(R.string.no_input), Toast.LENGTH_SHORT).show();
				finish();
			}
			break;
		case MENU_CLOSE:
			if((getIntent().getExtra("type").equals("create")) && (mEditText.getText().length() != 0)){
				showDialogNoticeNotSave();
			} else if((getIntent().getExtra("type").equals("read")) && (mTxtSize != mEditText.getText().length())){
				showDialogNoticeNotSave();
			} else {
				mTxtSize = 0;
				Toast.makeText(this, getResources().getString(R.string.no_input), Toast.LENGTH_SHORT).show();
				finish();
			}
			break;
		}
		return false;
	}
	
	@Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		// TODO Auto-generated method stub
		MenuItem mSaveItem = menu.findItem(MENU_SAVE);
		if((getIntent().getExtra("type").equals("create")) && (mEditText.getText().length() != 0)){
			mSaveItem.setEnabled(true);
		} else if((getIntent().getExtra("type").equals("read")) && (mTxtSize != mEditText.getText().length())){
			mSaveItem.setEnabled(true);
		} else {
			mSaveItem.setEnabled(false);
		}
		return super.onPrepareOptionsMenu(menu);
	}

	public void showDialogNoticeNotSave(){
		AlertDialog ad = new AlertDialog.Builder(this).create();
		ad.setTitle(getResources().getString(R.string.not_save_file_title));
		ad.setMessage(getResources().getString(R.string.not_save_file_msg));
		ad.setButton(getResources().getString(R.string.sure), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
        		finish();
            }});
		ad.setButton2(getResources().getString(R.string.Cancel), new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
            	// nothing to do
            }});
		ad.setCancelable(false);
		ad.show();
	}
	
	public void saveFileToDir(String type){
		if(type.equals("create")){
			writeFile(mEditText.getText().toString(), getFileName(), getFilePath() + "/");
			Toast.makeText(this, getResources().getString(R.string.save_to) + " " + getFilePath()/* + "/note"*/, Toast.LENGTH_SHORT).show();
			finish();
			RefreshMedia mRefresh = new RefreshMedia(this);
			mRefresh.notifyMediaAdd(getFilePath() + "/"+ getFileName()+SUFFIX);
		} else if(type.equals("read")){
			writeFile(mEditText.getText().toString(), mReadFileName, getFilePath());
			Toast.makeText(this, getResources().getString(R.string.save_to) + " " + getFilePath()/* + "/note"*/, Toast.LENGTH_SHORT).show();
			finish();
		}
		
	}

	@Override
	public void onBackPressed() {
		// TODO Auto-generated method stub
		if((getIntent().getExtra("type").equals("create")) && (mEditText.getText().length() != 0)){
			showDialogNoticeNotSave();
		} else if((getIntent().getExtra("type").equals("read")) && (mTxtSize != mEditText.getText().length())){
			showDialogNoticeNotSave();
		} else {
			super.onBackPressed();
		}
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		System.gc();
		super.onDestroy();
	}

	public String getFileName(){
		// set current time to use file name
	/*	Time t=new Time();
		t.setToNow();
		int year = t.year;
		int month = t.month;
		int date = t.monthDay;
		int hour = t.hour;
		int minute = t.minute;
		int second = t.second;
		String s= year + "-" + month + "-" + date + "-" + hour + "-" + minute + "-" + second;
		*/
		Date date = new Date(System.currentTimeMillis());
		SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd-kk-mm-ss");
		String filename = format.format(date);
		return filename;
	}
	
	public void writeFile(String sb, String filename, String path) {
	    //String foldername = Environment.getExternalStorageDirectory().getPath()
	    //                        + FOLDER;
		String foldername = path/* + FOLDER*/;
		
	    File folder = new File(foldername);
	    if (folder != null && !folder.exists()) {
	        if (!folder.mkdir() && !folder.isDirectory())
	        {
	           // Log.d(TAG, "Error: make dir failed!");
	            return;
	        }
	    }
	  
	    //String stringToWrite = sb.toString();
	    String targetPath = foldername + filename + SUFFIX;
	    File targetFile = new File(targetPath);
	    if (targetFile != null) {
	        if (targetFile.exists()) {
	            targetFile.delete();
	        }

	        OutputStreamWriter osw;
	        try{
	            osw = new OutputStreamWriter(
	                        new FileOutputStream(targetFile),"gbk");
	            try {
	                    osw.write(sb);
	                    osw.flush();
	                    osw.close();
	                } catch (IOException e) {
	                    // TODO Auto-generated catch block
	                    e.printStackTrace();
	                }
	        } catch (UnsupportedEncodingException e1) {
	                // TODO Auto-generated catch block
	            e1.printStackTrace();
	        } catch (FileNotFoundException e1) {
	            // TODO Auto-generated catch block
	            e1.printStackTrace();
	        }
	    }
	}
	
	public String readFile(String filepath) {
	    String path = filepath;
	    if (null == path) {
	    	System.out.println("Error: Invalid file name!");
	        return null;
	    }
	    
	    String filecontent = null;
	    File f = new File(path);
	    if (f != null && f.exists())
	    {
	    	try {
	    		//getFileCode(f);
	    		StringBuffer buffer = new StringBuffer();
				FileInputStream fis = new FileInputStream(f);
				String code = getFileCode(f);
				//System.out.println("--->code is: " + code);
				InputStreamReader isr = new InputStreamReader(fis, code);
				Reader in = new BufferedReader(isr);
				int ch;
				while ((ch = in.read()) > -1) {
					buffer.append((char)ch);
				}
				in.close();
				filecontent = buffer.toString();
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (UnsupportedEncodingException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	    	
	    }
	    return filecontent;
	}
	
	public String getFileCode(File mFile){
		String code = "";
		int length = 0;
		if(mFile.length() == 0) {
			code = "gb2312";
			return code;
		}
		try {
			InputStream is = new FileInputStream(mFile);
			byte[] codehead = new byte[4];
			byte[] head = InputStreamToByte(is);
			length = head.length < 4? head.length : 4;
			System.arraycopy(head, 0, codehead, 0, length);
			
		    if(head[0] == -1 && head[1] == -2) {
		        code = "UTF-16";
		    }
		    else if(head[0] == -2 && head[1] == -1) {
		        code = "Unicode";
		    }
		    else if(head[0] == -17 && head[1] == -69 && head[2] == -65)
		        code = "UTF-8";
		    else {
		        code = "gb2312";
		    }
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			code = "gb2312";
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			code = "gb2312";
		}
		
		return code;
	}
	
	public static byte[] InputStreamToByte(InputStream is) throws IOException {  
    	ByteArrayOutputStream bytestream = new ByteArrayOutputStream();
        int ch;
        while ((ch = is.read()) != -1) {
        	bytestream.write(ch);
        }
        byte imgdata[] = bytestream.toByteArray();
        bytestream.close();
        return imgdata;
   }
	
	class ReadTxt extends Thread {
		private String mFilePath;
		Message msg = new Message();

		public ReadTxt(String mFilePath) {
			super();
			this.mFilePath = mFilePath;
			mTmpString = "";
		}

		@Override
		public void run() {
			// TODO Auto-generated method stub
			mTmpString = readFile(this.mFilePath);
			if(mTmpString == null){
				msg.arg1 = LOAD_FAIL;
			} else {
				msg.arg1 = LOAD_SUCCESS;
				msg.arg2 = mTmpString.length();
			}
			handler.sendMessage(msg);
		}
		
	}
}
