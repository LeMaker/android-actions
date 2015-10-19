/**
 * 
 * @author caichsh <caichsh@artekmicro.com>
 */
package com.actions.logcat;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Build;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import android.app.ActivityManager;

public class MainActivity extends Activity{
	protected static final String TAG = "MainActivity"; 
	public static final String SDCARD_PATH = "/mnt/sd-ext/";
	static final int STATE_STOPPED = -1;
	static final int STATE_PAUSED = 0;
	static final int STATE_STARTED = 1;

	static  boolean useService = true;
	
	InputStream in = null;
	File tempFile = null;
	String packagePath = null;
	File logPath = null;

	Button btStart = null;
	Button btHome = null;
	Button btSave = null;
	Button btClear = null;
	Button btCopy = null;
	EditText etPath = null;
	
	EditText etFilename ;
	EditText etFilemaxsize ;
	private TextView tview; 
	private TextView statusview; 
	int mState = STATE_STOPPED;

	FileCopyDialog mProgressDlg = null;
	Dialog appendDlg = null;

	//save dialog
	Dialog repeatStartDlg =null;
	Dialog sizedlg;
	//	String defaultName = null;
	boolean inLog = false;
	boolean dumpstateDone = false;
	boolean dumpstateStarted = false;
	boolean actionslogcatDone = false;
	boolean actionslogcatStarted = false;;
	
	boolean isSaveSuccess = false;
	public static boolean isSdcardMount = true;
	public static boolean isTFcardMount = true;
	public static ActivityManager am;
	public static long freesize;
	public static int aviablesize;
	public static int maxsize;
	public static int suggestMaxsize;
	public static long currentMaxSize;
	public String sdpath;
	public FileCopyThread copyThread = null;
	boolean bgWorking = false;	
	String lastname="";
	String fullname="";
	Process logProcess = null;
	Process suProcess = null;
	
	LogModel mModel;
	private int mode;
	
	public static final int MAX_PROGRESS = 100;

	public static final int MAX_ALARM_SIZE = 50; //Mbytes
	public static final int SAVE_LOG_OK = 1;
	public static final int SAVE_LOG_FAILED = -1;
	public static final int WHAT_COPY_PROGRESS = 1;
	public static final int WHAT_COPY_FILE_NAME =2;
	public static final int WHAT_COPY_SHOW_DLG = 101;
	public static final int WHAT_COPY_DISMISS_DLG = 102;
	public static final int WHAT_COPY_SHOW_TOAST = 201;
	public static final int WHAT_FILE_EXIST = 301;
	public static final int WHAT_STORAGE_FULL = 401;

	public static final int MENU_GROUP = 0;
	public static final int MENU_ITEM_ABOUT = 0;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		Log.d(TAG,"enter onCreate!!");
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		Log.d(TAG,"enter onCreate!!");
		
		mModel = new LogModel(this);
		mode = mModel.getServiceMode();
		
		useService = false;
		if(mode == LogModel.SERVICE_MODE){
			Log.d(TAG, "mode is Serivice!");
			useService = true;
			bgWorking = mModel.processIsExist(LogModel.logcatServiceBinLocation);
		}
		else{
			mModel.copyRawFile();
			bgWorking = mModel.processIsExist(LogModel.logcatBinLocation);
			Log.d(TAG, "mode is exe!");
		}

		SharedPreferences sp ;		
		sp = getSharedPreferences("actionlogcat", Context.MODE_PRIVATE);
		fullname = sp.getString("fullname", "");
		lastname = sp.getString("lastname", "");
		maxsize = sp.getInt("max_size", 0);
		
		am = (ActivityManager)this.getSystemService(Context.ACTIVITY_SERVICE);
		//setPersistent(true);
		
		suggestMaxsize = mModel.getSuggestSize();
		currentMaxSize = suggestMaxsize;
		freesize = mModel.getStorageAvailableSize();
		
		logPath = new File(mModel.getLogDir());	
		if (mModel.getLogDir()==null){
			Log.d(TAG,"create log dir failed");
			Toast.makeText(MainActivity.this, R.string.newDirFailed, Toast.LENGTH_SHORT).show();
			MainActivity.this.finish();
		} else {
			if(MyFileUtils.getTotalFileLength(logPath) > (suggestMaxsize<<20)) {
				Log.d(TAG,"log dir is full");
				Toast.makeText(MainActivity.this, R.string.alarmstoresize, Toast.LENGTH_LONG).show();
			}
		}
		
		etFilename= (EditText)findViewById(R.id.inputfilename);
		etFilemaxsize = (EditText)findViewById(R.id.inputmaxsize);
		etFilename.setTextColor(android.graphics.Color.BLACK);
		etFilemaxsize.setTextColor(android.graphics.Color.BLACK);
		btStart = (Button)findViewById(R.id.start);	
	
		if(maxsize==0 || bgWorking==false)
			etFilemaxsize.setHint(getString(R.string.sizehint)+suggestMaxsize);
		else
			etFilemaxsize.setText(Integer.toString(maxsize));
		
		long logsize = (MyFileUtils.getTotalFileLength(logPath))>>20;
		String text = getString(R.string.storagefree)+freesize+"  Mbytes\n"+getString(R.string.storageexist)+logsize+"  Mbytes\n"+getString(R.string.storagesuggest)+" < "+suggestMaxsize+"  Mbytes";
		tview = (TextView)findViewById(R.id.storagetext);
		tview.setText(text);
		
		statusview = (TextView)findViewById(R.id.status);
		text = getString(R.string.status)+"  \n";
		
		if(bgWorking)
		{
			Log.d(TAG, "backgroud is running now!");
			text += (getString(R.string.recording)+fullname+" \n");
			btStart.setText(R.string.stop);
			etFilename.setText(lastname);
			
			inLog = true;
			mState = STATE_STARTED;
		}
		else
		{
			text += (getString(R.string.stopped)+fullname+" \n");
		}
		statusview.setText(text);
			
		btStart.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {
				// TODO Auto-generated method stub
				logstart();
			}
		});

		btClear = (Button)findViewById(R.id.clear);
		btClear.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				if(!inLog) {
					new AlertDialog.Builder(MainActivity.this)
					.setTitle(R.string.sureclear)
					.setPositiveButton(R.string.OK, new DialogInterface.OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							// TODO Auto-generated method stub
							clearAllLogFile();
							String text;
							
							long logsize = (MyFileUtils.getTotalFileLength(logPath))>>20;
							int leftsize = suggestMaxsize;
						/*	if(leftsize>(logsize+1))
								leftsize-=(logsize+1);
							else
								leftsize = 0;*/
							
							text = getString(R.string.storagefree)+freesize+"  Mbytes\n"+getString(R.string.storageexist)+logsize+"  Mbytes\n"+getString(R.string.storagesuggest)+" < "+leftsize+"  Mbytes";
							tview.setText(text);

							if(etFilemaxsize.getText().toString().isEmpty())
							{
								etFilemaxsize.setHint(getString(R.string.sizehint)+leftsize);
							}
						}
					}).setNegativeButton(R.string.NO, new DialogInterface.OnClickListener() {

						@Override
						public void onClick(DialogInterface dialog, int which) {
							// TODO Auto-generated method stub

						}
					}).show();
				} else {
					Toast.makeText(MainActivity.this, R.string.stopforclear, Toast.LENGTH_SHORT).show();
				}
			}
		});
		
		btCopy = (Button)findViewById(R.id.copy);
		btCopy.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				// TODO Auto-generated method stub
				if(inLog) {
					Toast.makeText(MainActivity.this, R.string.stopforclear, Toast.LENGTH_SHORT).show();
					return;
				}
				try {
					copy2External();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		});
	}

	Handler saveHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			// TODO Auto-generated method stub
			final int action  = msg.what;
			switch (action) {
			case SAVE_LOG_OK:
				dismissAppendDlg();
				dismissKeyBackDlg();
				dismissRepeatStartDlg();
				break;
			case SAVE_LOG_FAILED:
				dismissAppendDlg();
				dismissKeyBackDlg();
				dismissRepeatStartDlg();
				break;
			default:
				break;
			}
			super.handleMessage(msg);
		}

	};

	Handler copyHandler = new Handler() {

		@Override
		public void handleMessage(Message msg) {
			// TODO Auto-generated method stub
			switch (msg.what) {
			case WHAT_COPY_PROGRESS:
				updateProgressDlg(msg.arg1);
				break;
			case WHAT_COPY_FILE_NAME:
				setCurrentCopyFileName((String)msg.obj);
				break;
			case WHAT_COPY_SHOW_DLG:
				showProgressDlg();
				break;
			case WHAT_COPY_DISMISS_DLG:
				dismissProgressDlg();
				break;
			case WHAT_COPY_SHOW_TOAST:
				Toast.makeText(MainActivity.this, getText(R.string.savesucess) + "\n" +getText(R.string.saveto) + msg.obj, 2000).show();
				break;
			case WHAT_FILE_EXIST:

				setAlarmMessage(msg.arg1, msg.arg2);
				break;
			case WHAT_STORAGE_FULL:
				dismissProgressDlg();
				Toast.makeText(MainActivity.this, R.string.exstoragefull, Toast.LENGTH_LONG).show();
				break;
			default:
				break;
			}
			super.handleMessage(msg);
		}

	};
	
	private final BroadcastReceiver mFileReceiver = new BroadcastReceiver() 
	{
		// need stop file rw operation when sd card is unmounted
		public void onReceive(Context context, Intent intent) 
		{
			String action = intent.getAction();
			if(action.equals(Intent.ACTION_MEDIA_UNMOUNTED)  || action.equals(Intent.ACTION_MEDIA_EJECT))
			{
				Log.v(TAG, "Intent.ACTION_MEDIA_UNMOUNTED");
				isTFcardMount = false;
			}
			else if(action.equals(Intent.ACTION_MEDIA_MOUNTED))
			{
				Log.v(TAG, "Intent.ACTION_MEDIA_MOUNTED");				
			//	Toast.makeText(MainActivity.this, R.string.sdcardable,Toast.LENGTH_LONG).show();
				isTFcardMount = true;
			} 
		}
	};
	
    /**
     * Reads a line from the specified file.
     * @param filename the file to read from
     * @return the first line, if any.
     * @throws IOException if the file couldn't be read
     */
    private static String readLine(String filename) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(filename), 256);
        try {
            return reader.readLine();
        } finally {
            reader.close();
        }
    }

    public static String getFormattedKernelVersion() {
        try {
            return formatKernelVersion(readLine("proc/version"));

        } catch (IOException e) {
            Log.e(TAG,
                "IO Exception when getting kernel version for Device Info screen",
                e);

            return "Unavailable";
        }
    }

    public static String formatKernelVersion(String rawKernelVersion) {
        // Example (see tests for more):
        // Linux version 3.0.31-g6fb96c9 (android-build@xxx.xxx.xxx.xxx.com) \
        //     (gcc version 4.6.x-xxx 20120106 (prerelease) (GCC) ) #1 SMP PREEMPT \
        //     Thu Jun 28 11:02:39 PDT 2012

        final String PROC_VERSION_REGEX =
            "Linux version (\\S+) " + /* group 1: "3.0.31-g6fb96c9" */
            "\\((\\S+?)\\) " +        /* group 2: "x@y.com" (kernel builder) */
            "(?:\\(gcc.+? \\)) " +    /* ignore: GCC version information */
            "(#\\d+) " +              /* group 3: "#1" */
            "(?:.*?)?" +              /* ignore: optional SMP, PREEMPT, and any CONFIG_FLAGS */
            "((Sun|Mon|Tue|Wed|Thu|Fri|Sat).+)"; /* group 4: "Thu Jun 28 11:02:39 PDT 2012" */

        Matcher m = Pattern.compile(PROC_VERSION_REGEX).matcher(rawKernelVersion);
        if (!m.matches()) {
            Log.e(TAG, "Regex did not match on /proc/version: " + rawKernelVersion);
            return "Unavailable";
        } else if (m.groupCount() < 4) {
            Log.e(TAG, "Regex match on /proc/version only returned " + m.groupCount()
                    + " groups");
            return "Unavailable";
        }
        return m.group(1) + "\n" +                 // 3.0.31-g6fb96c9
            m.group(2) + " " + m.group(3) + "\n" + // x@y.com #1
            m.group(4);                            // Thu Jun 28 11:02:39 PDT 2012
    }
	
	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		if(keyCode == KeyEvent.KEYCODE_BACK) {
		/*	if(inLog) {
				//new Dialog here
				if(keyBackDlg == null) {
					keyBackDlg = new Dialog(this);
					LayoutInflater inflater = getLayoutInflater();
					final View dlView =  inflater.inflate(R.layout.save, null);
					keyBackDlg.setContentView(dlView);
					keyBackDlg.setTitle(R.string.nosave);
					final Button save = (Button)dlView.findViewById(R.id.OK);
					final Button cancel = (Button)dlView.findViewById(R.id.cancel);
					final Button back = (Button)dlView.findViewById(R.id.back);
					final EditText fileName = (EditText)dlView.findViewById(R.id.dl_filename);
					cancel.setText(R.string. no);
					save.setOnClickListener(new OnClickListener() {

						@Override
						public void onClick(View v) {
							// TODO Auto-generated method stub
							String path = fileName.getText().toString();
							if(path == null || (path != null && path.trim().length() == 0)) {
								Toast.makeText(MainActivity.this, R.string.invaliableName, Toast.LENGTH_SHORT).show();
								return;
							}
							save(path);
						}
					});
					cancel.setOnClickListener(new OnClickListener() {

						@Override
						public void onClick(View v) {
							// TODO Auto-generated method stub
							if(tempFile != null && tempFile.exists()) {
								tempFile.delete();
							}
							inLog = false;
							killProcess();
							keyBackDlg.dismiss();
							finish();
						}
					});
					back.setOnClickListener(new OnClickListener() {

						@Override
						public void onClick(View v) {
							// TODO Auto-generated method stub
							keyBackDlg.dismiss();
						}
					});
					keyBackDlg.setOnShowListener(new OnShowListener() {

						@Override
						public void onShow(DialogInterface dialog) {
							// TODO Auto-generated method stub
							fileName.setText(getEditName());
						}
					});
				}
				keyBackDlg.show();
				return true;
			}*/
		}
		return super.onKeyDown(keyCode, event);
	}


	private boolean isFileExist (String filename)
	{
		return false;
	}
	
	/*
	 *
	 *public static String getKernelVersion() {
    String kernelVersion = "";
    InputStream inputStream = null;
    try {
        inputStream = new FileInputStream("/proc/version");
    } catch (FileNotFoundException e) {
        e.printStackTrace();
        return kernelVersion;
    }
    BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(inputStream), 8 * 1024);
    String info = "";
    String line = "";
    try {
        while ((line = bufferedReader.readLine()) != null) {
            info += line;
        }
    } catch (IOException e) {
        e.printStackTrace();
    } finally {
        try {
            bufferedReader.close();
            inputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
 
    try {
        if (info != "") {
            final String keyword = "version ";
            int index = info.indexOf(keyword);
            line = info.substring(index + keyword.length());
            index = line.indexOf(" ");
            kernelVersion = line.substring(0, index);
        }
    } catch (IndexOutOfBoundsException e) {
        e.printStackTrace();
    }
 
    return kernelVersion;
}
	 */
	public void logstart() {
		String sizestr = null;
		if(etFilename.getText().toString().isEmpty())
			setDefaultName();
		else
			isFileExist(etFilename.getText().toString());
		
		if(mState==STATE_STARTED)
		{
			Log.d(TAG, "stop log service");
			mModel.stopLogcatService();
			
			mState = STATE_STOPPED;
			btStart.setText(R.string.start);
			etFilename.setText("");
			etFilemaxsize.setText("");
			inLog = false;
			bgWorking = false;
			
			if(tempFile!=null)
				statusview.setText(getString(R.string.status)+"\n"+getString(R.string.stopped)+tempFile.getAbsolutePath());
			else if(!fullname.isEmpty())
				statusview.setText(getString(R.string.status)+"\n"+getString(R.string.stopped)+fullname);
			else
				statusview.setText(getString(R.string.status)+"\n"+getString(R.string.stopped));
			
			long logsize = (MyFileUtils.getTotalFileLength(logPath))>>20;
			Log.d(TAG,"logsize1="+(MyFileUtils.getTotalFileLength(logPath)>>10)+"kbytes");
			int leftsize = suggestMaxsize;
			if(leftsize>(logsize+1))
				leftsize-=(logsize+1);
			else
				leftsize = 0;
			
			String text = getString(R.string.storagefree)+freesize+"  Mbytes\n"+getString(R.string.storageexist)+logsize+"  Mbytes\n"+getString(R.string.storagesuggest)+" < "+leftsize+"  Mbytes";
			tview = (TextView)findViewById(R.id.storagetext);
			tview.setText(text);

			if(etFilemaxsize.getText().toString().isEmpty())
			{
				etFilemaxsize.setHint(getString(R.string.sizehint)+leftsize);
			}
			tempFile = null;
			return;
		}
		
		if(mState==STATE_STOPPED)
		{
			sizestr = etFilemaxsize.getText().toString();
			if(sizestr.isEmpty()){
				maxsize = suggestMaxsize > 1000 ? 1000: suggestMaxsize;
			}
			else{
				Pattern regex = Pattern.compile( "[0-9]*"); 
				Matcher match = regex.matcher(sizestr);
				if(!match.matches()){
					Log.d(TAG, "invalid max size:"+sizestr);
					sizedlg = new Dialog(this);
					LayoutInflater inflater = getLayoutInflater();
					final View view =  inflater.inflate(R.layout.invalidsize, null);
					sizedlg.setContentView(view);
					sizedlg.setTitle(R.string.invalidfilesize);
					final Button btCancel = (Button)view.findViewById(R.id.btcancel1);
					btCancel.setOnClickListener(new OnClickListener() {
						@Override
						public void onClick(View v) {
							// TODO Auto-generated method stub
							etFilemaxsize.setText("");
							sizedlg.dismiss();
						}
					});
					sizedlg.show();
					return;
				}
				maxsize = Integer.parseInt(sizestr);	
			}
			
			tempFile = mModel.creatLogFile(etFilename.getText().toString());
			if(tempFile==null){
				Log.d(TAG, "create log file fail");
				return;
			}
			Log.d(TAG, "logfile="+tempFile.getAbsolutePath());
			mState=STATE_STARTED;
			btStart.setText(R.string.stop);
			statusview.setText(getString(R.string.status)+"\n"+getString(R.string.recording)+tempFile.getAbsolutePath());
		}
		
		SharedPreferences sp = getSharedPreferences("actionlogcat", MODE_PRIVATE);
		Editor editor = sp.edit();
		
		editor.putString("lastname", etFilename.getText().toString());
		editor.putString("fullname", tempFile.getAbsolutePath());
		if(sizestr.isEmpty())
			editor.putInt("max_size", 0);
			//editor.putInt("max_size", suggestMaxsize);
		else
			editor.putInt("max_size", maxsize);
		editor.commit();
			
		if(useService)
		{
			mModel.startLogcatService();
	        inLog = true;
			return;
		}
		
		new AsyncTask<Void, Void, Integer>() {
			protected Integer doInBackground(Void... params) {
				// TODO Auto-generated method stub
				try {	
					Log.d(TAG, "begin logstart no service");
					String str;
					StringBuilder sb = new StringBuilder();
					str = getString(R.string.memorysize)+":"+mModel.getTotalMemSize()+"M\n";
					sb.append(str);
					str = getString(R.string.model)+":"+Build.MODEL+"\n";
					sb.append(str);
					str = getString(R.string.kernelversion)+":"+getFormattedKernelVersion()+"\n";
					sb.append(str);
					str = getString(R.string.buildversion)+":"+Build.DISPLAY+"\n";
					sb.append(str);
					str=("\n===========actionslogcat: start at " + getTimeNow() +"================\n");
					sb.append(str);	
					FileWriter fw = new FileWriter(tempFile);
					fw.write(sb.toString());
					fw.close();
					
					actionslogcatDone = false;
					actionslogcatStarted = true;
					
					Log.d(TAG,"switch to su");
				    suProcess = Runtime.getRuntime().exec("su");
				    Log.d(TAG,"switch over");
					DataOutputStream dos = new DataOutputStream(suProcess.getOutputStream());
					
				//	dos.writeBytes("/system/xbin/actionslogcat -v time >> " + tempFile.getAbsolutePath() + "\n");
					dos.writeBytes(LogModel.logcatBinLocation+" -v time -f " + tempFile.getAbsolutePath() + " -m  "+maxsize+ "\n");
		            dos.writeBytes("exit\n");
		            dos.flush();
		            inLog = true;
		            suProcess.waitFor();
				/*
					//String cmd = "/system/xbin/actionslogcat -v time  -f " + tempFile.getAbsolutePath()+" -m  "+maxsize;
					String cmd = LogModel.logcatBinLocation + " -v time  -f " + tempFile.getAbsolutePath()+" -m  "+maxsize;
					Runtime r = Runtime.getRuntime();
					logProcess = r.exec(cmd);
					logProcess.waitFor();*/
					
					Log.d(TAG, "actionslogcat exit");
					actionslogcatDone = true;
					actionslogcatStarted = false;
	/*				fw = new FileWriter(tempFile, true);
					fw.append("\n===========actionslogcat: done at " + getTimeNow() +"================\n");
//					fw.flush();
					fw.close();*/
					inLog = false;
//					InputStream inerr = p.getErrorStream();
//					String errMsg = inputStream2String(inerr);
//					Log.e(TAG, "sh error msg:" + errMsg);
//					if(errMsg != null && (errMsg.contains("/dev/log/main") || errMsg.contains("/dev/log/system"))) {
//						return -1;*/
//					}
					// TODO Auto-generated catch block
				} catch (Exception e) {
					// TODO: handle exception
					e.printStackTrace();
					return -2;
				}
				return 0;
			}

		/*	@Override
			protected void onPostExecute(Integer result) {
				// TODO Auto-generated method stub
				if(result == -1) {
					Toast.makeText(MainActivity.this, R.string.surePermission, Toast.LENGTH_LONG).show();
				//	inLog =false;
					etPath.setText("");
				}
				super.onPostExecute(result);
			}*/

		}.execute();
	}


	private void setDefaultName() {
		etFilename.setText(mModel.getDefaultName());
	}
	
	private String getTimeNow() {
		Date date = new Date(System.currentTimeMillis());
		SimpleDateFormat sdf = new SimpleDateFormat("yy-MM-dd hh:mm:ss");
		return sdf.format(date);
	}

	private void clearAllLogFile() {
		if(logPath != null && logPath.exists()) {
			File[] logs = logPath.listFiles();
			for(File log : logs) {
				log.delete();
			}
		}
	}
	private void copy2External() throws IOException{
		if (canWrite(SDCARD_PATH) && isSdcardMount){
			final File exLogPath = new File(SDCARD_PATH + "log");
			if(!exLogPath.exists() || (exLogPath.exists() && (!exLogPath.isDirectory()))) {
				if(exLogPath.exists()) {
					exLogPath.delete();
				}
				exLogPath.mkdirs();
			}
			if(!exLogPath.canWrite()) {
				Toast.makeText(this, R.string.sdcardinvaliable, Toast.LENGTH_LONG).show();
				return ;
			}

			if(logPath != null) {

				copyThread = new FileCopyThread(this, copyHandler);
				copyThread.setSource(logPath);
				copyThread.setTarget(exLogPath);
				copyThread.start();
			}

		} 
		else {
//			Toast.makeText(this, R.string.sdcardinvaliable, Toast.LENGTH_LONG).show();
			final AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setTitle(R.string.sdcardinvaliable);
		//	builder.setMessage(R.string.copytoflash);
			builder.setOnCancelListener(new OnCancelListener() {
				@Override
				public void onCancel(DialogInterface dialog) {
					// TODO Auto-generated method stub
				}
			});
			builder.setNegativeButton(R.string.NO, new DialogInterface.OnClickListener() {
				
				@Override
				public void onClick(DialogInterface dialog, int which) {
					// TODO Auto-generated method stub
				}
			});
		/*	builder.setPositiveButton(R.string.OK, new DialogInterface.OnClickListener() {
				
				@Override
				public void onClick(DialogInterface dialog, int which) {
					// TODO Auto-generated method stub
					final File exLogPath = new File(FLASH_PATH + "log");
					if(!exLogPath.exists() || (exLogPath.exists() && (!exLogPath.isDirectory()))) {
						if(exLogPath.exists()) {
							exLogPath.delete();
						}
						exLogPath.mkdirs();
					}
					if(!exLogPath.canWrite()) {
						Toast.makeText(MainActivity.this, R.string.flashinvaliable, Toast.LENGTH_LONG).show();
						return ;
					}

					if(logPath != null) {
						copyThread = new FileCopyThread(MainActivity.this, copyHandler);
						copyThread.setSource(logPath);
						copyThread.setTarget(exLogPath);
						copyThread.start();
					}
				}
			}); */
			builder.show();
		}
		/*else {
			Toast.makeText(this, R.string.allinvaliable, Toast.LENGTH_LONG).show();
		}*/
		
	}
	private boolean canWrite(String path) {
		File file = new File(path);
		if(!file.exists()) return false;
		if(file.canWrite()) return true;
		return false;
	}
	void showProgressDlg()
	{
		if( null == mProgressDlg )
		{
			mProgressDlg = new FileCopyDialog(this);
			mProgressDlg.setTitle(R.string.wait);
			mProgressDlg.setMaxProgress(MAX_PROGRESS);
		}
		mProgressDlg.show();
	}

	public void dismissProgressDlg()
	{
		if( null != mProgressDlg )
		{
			if(mProgressDlg.isShowing())
			{
				mProgressDlg.dismiss();
			}
		}

	}
	public void updateProgressDlg(int progress) {
		if( null != mProgressDlg )
		{
			if(mProgressDlg.isShowing())
			{
				mProgressDlg.updateProgress(progress);
			}
		}
	}
	private void dismissAppendDlg() {
		if(null != appendDlg && appendDlg.isShowing()) {
			appendDlg.dismiss();
		}
	}
	private void dismissKeyBackDlg() {
	/*	if(keyBackDlg != null && keyBackDlg.isShowing()) {
			keyBackDlg.dismiss();
			MainActivity.this.finish();
		}*/
	}
	private void dismissRepeatStartDlg() {
		if(repeatStartDlg != null && repeatStartDlg.isShowing()) {
			repeatStartDlg.dismiss();
		}
	}

	private void setCurrentCopyFileName(String name) {
		if( null != mProgressDlg )
		{
			if(mProgressDlg.isShowing())
			{
				mProgressDlg.setCopyFileName(name);
			}
		}
	}

	private void setAlarmMessage(int sourceLen, int targetLen) {
		if( null != mProgressDlg )
		{
			if(mProgressDlg.isShowing())
			{
				String text = getResources().getString(R.string.copyfileexist);

				mProgressDlg.setAlarmMessage(String.format(text, targetLen, sourceLen));
			}
		}
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// TODO Auto-generated method stub
		int id = item.getItemId();
		if(id == MENU_ITEM_ABOUT){
			Intent intent = new Intent(this, AboutActivity.class);
			startActivity(intent);
		}
		return super.onOptionsItemSelected(item);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// TODO Auto-generated method stub
		menu.add(MENU_GROUP, MENU_ITEM_ABOUT, 0, R.string.about);
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public void onResume() 
	{
		IntentFilter intentFilter = new IntentFilter(Intent.ACTION_MEDIA_UNMOUNTED);	
		intentFilter.addAction(Intent.ACTION_MEDIA_MOUNTED);
		intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
		intentFilter.addDataScheme("file");
		registerReceiver(mFileReceiver, intentFilter);
		super.onResume();
	}
	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		Log.d(TAG,"enter onDestroy");
		if(mState==STATE_STARTED) {
		//	mState=STATE_STOPPED;
			if(logProcess!=null)
				logProcess.destroy();
			//if(inLog)
			if(suProcess!=null)
				suProcess.destroy();
		//	stop();
		}
		unregisterReceiver(mFileReceiver);
		super.onDestroy();
	}

	public static String inputStream2String(InputStream is) throws IOException
	{
		BufferedReader in = new BufferedReader(new InputStreamReader(is));
		//		StringBuffer buffer = new StringBuffer();
		StringBuilder sb = new StringBuilder();
		String line = "";
		while ((line = in.readLine()) != null)
		{
			sb.append(line);
		}
		in.close();
		in=null;
		return sb.toString();
	}

	/*public static void print(InputStream in) {
	BufferedReader br = new BufferedReader(new InputStreamReader(in));
	String line= null;
	try {
		while((line = br.readLine()) != null) {
			System.out.println(line);
		}
	} catch (IOException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
}*/
//	static {
//		System.loadLibrary("actionslogcat");
//	}
//	public native void system(String cmd);
//	public native void kill();
}
