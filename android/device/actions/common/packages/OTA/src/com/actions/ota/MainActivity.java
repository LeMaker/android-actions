package com.actions.ota;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.Notification.Builder;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.PowerManager;
import android.text.method.ScrollingMovementMethod;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.actions.model.CheckResult;
import com.actions.model.CheckVersionCallBack;
import com.actions.model.CheckVersionMachine;
import com.actions.model.DownloadStateCallBack;
import com.actions.model.DownloadStatus;
import com.actions.model.UpdateInfo;
import com.actions.model.DownloadStateMachine;
import com.actions.utils.Debug;
import com.actions.utils.Utilities;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.io.File;
import android.util.Log;
import com.actions.service.DownloadService;
import java.util.Timer;
import java.util.TimerTask;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;

/**
 * OTA(Over-the-Air)
 * 
 * @author:Laura Wan
 * 
 * */
public class MainActivity extends Activity implements DownloadStateCallBack, CheckVersionCallBack {
	private Context mContext = this;

	private ProgressDialog mScanWaitDialog;
	private ProgressDialog mDeleteDialog;

	private TextView RightPromptInfo = null;
	private TextView LeftPromptInfo = null;
	private Button btnOperation = null;
	private TextView Progresstext  = null ;
    private static final String TAG = "ota.MainActivity";
    private static final boolean DEBUG = true;
    private int preLastId = 0;
    private int lastId = 0;

    public static final int DISABLE_DOWNLOAD_BUTTON = 1 << 0;
    
	/**
	 * Used to perform zip file downloads
	 */
	private DownloadStateMachine mDownMachine = null;
	
	/**
	 * Used to perform xml file  downloads
	 */
	private CheckVersionMachine mCheckMachine = null;

    private CustomReceiver mReceiver = null;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		mDownMachine = new DownloadStateMachine(this, this);
		mCheckMachine = new CheckVersionMachine(this, this);
		
		getViewObjects();

		updateUpdatedTime();
		
		registerCustomReceiver();
		
		/**
		 * if this activity is launched from notification, then go into download views.
		 */
		Intent intent = getIntent();
		if(intent.getBooleanExtra("LaunchFromNotification", false)) {
		    initDownloadViews();
		}
	}

	protected void onDestroy() {
        // TODO Auto-generated method stub
	    unRegisterCustomReceiver();
        super.onDestroy();
    }

	
    @Override
    protected void onNewIntent(Intent intent) {
        // TODO Auto-generated method stub
        Debug.d("onNewIntent");
        /**
         * TODO think about this activity is opened by notification...
         * is any thing should I do here?
         */
        super.onNewIntent(intent);
    }

    @Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.add(1, 1, 1, getResources().getString(R.string.update_native));
		menu.add(1, 2, 1, getResources().getString(R.string.menu_settings));		
		return true;
	}

	@Override
	public boolean onMenuItemSelected(int featureId, MenuItem item) {
		super.onMenuItemSelected(featureId, item);
		if (item.getItemId() == 1) {
			//local update
			Intent intent = new Intent();
			intent.setClass(this, InstallActivity.class);
			startActivity(intent);			
		} else if (item.getItemId() == 2) {
			//Setting
			Intent intent = new Intent();
			intent.setClass(this, SettingActivity.class);
			startActivity(intent);
		}
		return true;
	}
	
	@Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(keyCode == KeyEvent.KEYCODE_BACK) {
            /*
             *still thinking.... 
             */
            if(mDownMachine.getState() == DownloadStateMachine.STATE_DOWNLOADING 
                    || mDownMachine.getState() == DownloadStateMachine.STATE_PAUSED){
                showCancelDialog();
                return true;
            } else{ 
                finish();
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
	}
	
//	private Timer mTick;
//	TimerTask mTask = new TimerTask() {		
//		@Override
//		public void run() {
//		    btnOperation.setEnabled(true);
//		    mTick.cancel();
//		}
//	};	
	
	Handler myHandler = new Handler() {  
          public void handleMessage(Message msg) {   
               switch (msg.what) {   
                    case DISABLE_DOWNLOAD_BUTTON:   
                         btnOperation.setVisibility(View.VISIBLE);
                         btnOperation.setEnabled(true);
                         break;   
               }    
          }   
     };  
     
	private void showCancelDialog() {
	    if(DEBUG) Log.d(TAG,"showCancelDialog");
        new AlertDialog.Builder(this)
                .setTitle(getString(R.string.title_activity_main))
                .setMessage(getString(R.string.cancel_download))
                .setCancelable(false)
                .setPositiveButton(getString(R.string.sure),
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                mDownMachine.stop();
                                mDownMachine.CancleDownload();
                                //btnOperation.setVisibility(View.VISIBLE);
                                btnOperation.setEnabled(false);
                                Message message = new Message();   
                                message.what = DISABLE_DOWNLOAD_BUTTON;   
                                myHandler.sendMessageDelayed(message, 20000);  
                                 
                                //mTick = new Timer();
                                //mTick.schedule(mTask, 3000, 900000);
                                //LeftPromptInfo.setText(getString(R.string.Updates_available));
                                updateLeftPromptInfo(R.string.Updates_available);
                                Progresstext.setText("");
                                Progresstext.setVisibility(View.GONE);
                                if(DEBUG) Log.d(TAG,"sure OnClickListener");
                            }
                        }).setNegativeButton(getString(R.string.cancel),
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int which) {
                                if(DEBUG) Log.d(TAG,"cancel OnClickListener");
                            }
                        }
                ).show();
    }

	/**
	 * @Title: getViewObjects
	 * @Description: Get view objects through the view Id
	 * @param
	 * @return void
	 * @throws
	 */
	private void getViewObjects() {
		RightPromptInfo = (TextView) findViewById(R.id.RightPromptInfo);
		LeftPromptInfo = (TextView) findViewById(R.id.LeftPromptInfo);
		btnOperation = (Button) findViewById(R.id.btnOperation);
		Progresstext = (TextView) findViewById(R.id.progresstext);
		RightPromptInfo.setMovementMethod(ScrollingMovementMethod.getInstance());
	}

	private void initDownloadViews() {
	    dumpUpdateInfo();
        btnOperation.setText(getString(R.string.download)) ;
        //LeftPromptInfo.setText(getString(R.string.Updates_available));
        updateLeftPromptInfo(R.string.Updates_available);
	}
	/**
	 * @Title: updateUpdatedTime
	 * @Description: Get System last updated and append to RightPromptInfo
	 * @param
	 * @return void
	 * @throws
	 */
	private void updateUpdatedTime() {
		RightPromptInfo.setText("");
		RightPromptInfo.append("\n\t");
		RightPromptInfo.append(getString(R.string.last_update_time));
		
		//get current time		
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy.MM.dd "+ "HH:mm:ss") ;     
		Date curDate = new Date(System.currentTimeMillis());
		String deftime = sdf.format(curDate);		
		
		SharedPreferences preferences = getSharedPreferences("time", MODE_PRIVATE);
		String time = preferences.getString("time", deftime);
		RightPromptInfo.append(time);
	}

	// go to update online activity
	/** 
	 * @Title: updateOnLine 
	 * @Description: Button Event Listeners
	 * @param @param view
	 * @return void
	 * @throws 
	 */  
	public void updateOnLine(View view) {
	    if(DEBUG) Log.d(TAG,"updateOnLine");
		// check whether open wifi or 3
		if (Utilities.checkConnectivity(mContext)) {
			if (btnOperation.getText().toString()
					.equals(getString(R.string.Check_updates))) {
			    if(DEBUG) Log.d(TAG,"going to check");
				mCheckMachine.check();
				
			} else if(btnOperation.getText().toString()
                    .equals(getString(R.string.download))) {
                long filesize = Long.parseLong(UpdateApplication.instance().mUpdateInfo.getFileSize());
                if(DEBUG) Log.d(TAG,"updateOnLine filesize=" + filesize);
                File  f = new File(Utilities.SdCardRoot);
                long  freeSize = 0;
                if (f != null)freeSize = f.getFreeSpace();
                if(DEBUG) Log.d(TAG,"updateOnLine freeSize=" + freeSize);
                f = null;
                f = new File(Utilities.SdCardRoot + File.separator + Utilities.mRecoveryFileName);
                if (f != null)freeSize = freeSize + f.length();
                if(DEBUG) Log.d(TAG,"updateOnLine file.length=" + f.length());
                f = null;
                
                if (freeSize > filesize) {
			        if(DEBUG) Log.d(TAG,"going to download");
			        mDownMachine.start();
			    } else {
			        updateLeftPromptInfo(R.string.sdcard_low_free_space);
			        Toast.makeText(MainActivity.this, getString(R.string.sdcard_low_free_space),
					Toast.LENGTH_SHORT).show();
			    }
			    
			} else if (btnOperation.getText().equals(getString(R.string.Quit))) {
				finish();
			}
		} else {
		    if(DEBUG) Log.d(TAG,"connectivity lost");
		    updateLeftPromptInfo(R.string.not_connect);
			Toast.makeText(MainActivity.this, getString(R.string.not_connect),
					Toast.LENGTH_SHORT).show();
		}
	}

	/** 
	 * @Title: Show_ProgressDialog 
	 * @Description: wait progress
	 * @param 
	 * @return void
	 * @throws 
	 */  
	private void showProgressDialog() {
	    if(this.isFinishing())
	        return;
	    if(mScanWaitDialog == null)
	    {
	        mScanWaitDialog = new ProgressDialog(this);
	    }
		mScanWaitDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
		mScanWaitDialog.setTitle(R.string.progress_dialog_title);
		mScanWaitDialog.setMessage(getString(R.string.progress_dialog_mgs));
		mScanWaitDialog.setCancelable(false);
		mScanWaitDialog.show();
	}
	
	private void dismissProgressDialog() {
	    if(this.isFinishing())
            return;
	    if(mScanWaitDialog != null && mScanWaitDialog.isShowing()) {
	        mScanWaitDialog.dismiss();
	    }
	}
	
    private void showDeleteDialog() {
        if(this.isFinishing())
            return;
        if(mDeleteDialog == null)
            mDeleteDialog = new ProgressDialog(this);
        mDeleteDialog.setProgressStyle(ProgressDialog.STYLE_SPINNER);
        mDeleteDialog.setTitle(R.string.check_local_dialog_title);
        mDeleteDialog.setMessage(getString(R.string.check_local_dialog_msg));
        mDeleteDialog.setCancelable(false);
        mDeleteDialog.show();
    }
    private void dismissDeleteDialog() {
        if(this.isFinishing())
            return;
        if(mDeleteDialog == null)
            return;
        if(mDeleteDialog.isShowing())
            mDeleteDialog.dismiss();
    }
    
    private void registerCustomReceiver() {
        mReceiver = new CustomReceiver();
        IntentFilter filter=new IntentFilter();
        filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        filter.addAction(Intent.ACTION_SHUTDOWN);
        mContext.registerReceiver(mReceiver, filter);
    }

	private void unRegisterCustomReceiver() {
	   unregisterReceiver(mReceiver);
	}
	
	private class CustomReceiver extends BroadcastReceiver{
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if(DEBUG) Log.d(TAG,"onReceive action=" + action);
            if(action.equals(ConnectivityManager.CONNECTIVITY_ACTION)){
                if(!Utilities.checkConnectivity(MainActivity.this)) {
                    if(DEBUG) Log.d(TAG,"connectivity lost, goint to pause");
                    Toast.makeText(MainActivity.this, getString(R.string.not_connect),
        					Toast.LENGTH_SHORT).show();
        		    //LeftPromptInfo.setText(getString(R.string.not_connect));
        		    int staute = mDownMachine.getState();
        		    if(staute == DownloadStateMachine.STATE_DOWNLOADING 
                    || staute == DownloadStateMachine.STATE_PAUSED
                    || staute == DownloadStateMachine.STATE_INITED
                    || staute == DownloadStateMachine.STATE_UNKNOWN){
                        updateLeftPromptInfo(R.string.not_connect);
                    }
                    mCheckMachine.pause();
                    mDownMachine.pause();
                } else {
                    if(DEBUG) Log.d(TAG,"connectivity resumed, goint to resume");
                    //LeftPromptInfo.setText(getString(R.string.wait_downloading));
                    //Toast.makeText(MainActivity.this, getString(R.string.connect),
        			//		Toast.LENGTH_SHORT).show();
                    mCheckMachine.resume();
                    mDownMachine.resume();
                    int staute = mDownMachine.getState();
                    if(staute == DownloadStateMachine.STATE_DOWNLOADING 
                        || staute == DownloadStateMachine.STATE_PAUSED){
                        updateLeftPromptInfo(preLastId);
                    } else if(staute == DownloadStateMachine.STATE_INITED 
                        || staute == DownloadStateMachine.STATE_UNKNOWN){
                        updateLeftPromptInfo(R.string.connect);
                    }
                }
            } else if(action.equals(Intent.ACTION_SHUTDOWN)) {
                if(DEBUG) Log.d(TAG,"shut down");
                mCheckMachine.stop();
                mDownMachine.stop();
                finish();
            }
        }
	}
	
    public void onDownloadPended(DownloadStatus us) {
        // TODO Auto-generated method stub
        
    }
    
    

    public void onDownloadPause(DownloadStatus us) {
        // TODO Auto-generated method stub
    	if (Utilities.checkConnectivity(mContext)) {
    		setUpNotification(mContext.getString(R.string.download_pause), 
    		    mContext.getString(R.string.download_pause), reasonToString(us.getStatus()));
		}
    	else {
    		setUpNotification(mContext.getString(R.string.download_pause), 
    		    mContext.getString(R.string.download_pause), getString(R.string.not_connect));
		}
    
    }

    public void onDownloadStart() {
        // TODO Auto-generated method stub
        //LeftPromptInfo.setText(getString(R.string.wait_downloading));
        updateLeftPromptInfo(R.string.wait_downloading);
        btnOperation.setVisibility(View.GONE);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        
        if(RightPromptInfo.getText() != null && !RightPromptInfo.getText().toString().contains(getString(R.string.Tip))) {
            //RightPromptInfo.append("\n\t");
            //RightPromptInfo.append(getString(R.string.Tip));
            RightPromptInfo.setText("\n\t"+getString(R.string.Tip)+"\n\t"+RightPromptInfo.getText());
        }
        
        Progresstext.setBackground(getResources().getDrawable(R.drawable.ic_progress_pic));
        Progresstext.setVisibility(View.VISIBLE);
        Progresstext.setText("0%");
    }


    /**
     * NOTICE: DO NOT attempt to change state machine by calling mMachine.xxx().
     */
    public void onDownloadStop() {
        // TODO Auto-generated method stub
        
    }


    public void onDownloadResume(DownloadStatus us) {
        // TODO Auto-generated method stub
        setUpNotification(mContext.getString(R.string.download_resume), mContext.getString(R.string.download_resume), mContext.getString(R.string.download_resume));
    }


    public void onDownloadFinish(DownloadStatus us) {
        // TODO Auto-generated method stub
        Debug.i("download finished, may gonna check md5");
        
    }


    public void onDownloadRuning(DownloadStatus us) {
        // TODO Auto-generated method stub
        Progresstext.setVisibility(View.VISIBLE);
        Progresstext.setText(String.valueOf(us.getProgress())+"%");
    }


    public void onDownloadFailed(DownloadStatus us) {
        // TODO Auto-generated method stub
        if(DEBUG) Log.d(TAG,"onDownloadFailed");
        if(DEBUG) Log.d(TAG,"download encounter error!!");
        btnOperation.setVisibility(View.VISIBLE);
        //LeftPromptInfo.setText(getString(R.string.Updates_available));
        updateLeftPromptInfo(R.string.Updates_available);
        Progresstext.setText("");
        Progresstext.setVisibility(View.GONE);
        
        mDownMachine.stop();
        int errId = R.string.download_error_http;
        if(us.getErr() == DownloadStatus.ERROR_FILE_NOT_FOUND)
            errId = R.string.download_error_file_not_found;
        
        Toast.makeText(getApplicationContext(), getString(errId), Toast.LENGTH_LONG).show();
    }

    public void showInstallDialog() {
	    if(DEBUG) Log.d(TAG,"showInstallDialog");
        new AlertDialog.Builder(this)
            .setTitle(getString(R.string.title_activity_main))
            .setMessage(getString(R.string.is_local_upgrade))
            .setCancelable(false)
            .setPositiveButton(getString(R.string.local_upgrade),
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        if(DEBUG) Log.d(TAG,"Check local file before starting download successful!");
                        mDownMachine.setState(DownloadStateMachine.STATE_CHECKED);
                        onCheckMD5Finish(true);           
                    }
                }).setNegativeButton(getString(R.string.re_download),
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            File sFile = new File(Utilities.SdCardRoot + File.separator + Utilities.mRecoveryFileName);
                            sFile.delete();
                            mDownMachine.setState(DownloadStateMachine.STATE_UNKNOWN);
                            bindService(new Intent(mContext, DownloadService.class), mDownMachine.getServiceConnection(), Context.BIND_AUTO_CREATE);
                        }
                    }
                ).show();
    }
        
    public void onCheckMD5Start() {
        // TODO Auto-generated method stub
        
    }


    public void onCheckMD5Finish(boolean passed) {
        // TODO Auto-generated method stub
        if(passed) {
            enterInstallActivity();
        } else {
            btnOperation.setText(getString(R.string.Quit));
            //LeftPromptInfo.setText(getString(R.string.check_fail));
            updateLeftPromptInfo(R.string.check_fail);
            Utilities.setViewDisText(RightPromptInfo, getString(R.string.MD5_Error)) ;
            Progresstext.setBackground(getResources().getDrawable(R.drawable.ic_error_pic)) ;
        }
    }
    
    public void onDeleteOldStart() {
        // TODO Auto-generated method stub
        showDeleteDialog();
    }

    public void onDeleteOldFinished() {
        // TODO Auto-generated method stub
        dismissDeleteDialog();
    }

    /**
     * CV=CheckVersion
     */
    public void onCVStart() {
        // TODO Auto-generated method stub
        showProgressDialog();
    }

    public void onCVError(CheckResult cr) {
        // TODO Auto-generated method stub
        updateLeftPromptInfo(R.string.connect_server_wrong);
        dismissProgressDialog();
        Toast.makeText(getApplicationContext(),
                getString(R.string.connect_server_wrong),
                Toast.LENGTH_LONG).show();
    }

    public void onCVServerNotFound(CheckResult cr) {
        // TODO Auto-generated method stub
        updateLeftPromptInfo(R.string.connect_server_wrong);
        dismissProgressDialog();
        Toast.makeText(getApplicationContext(),
                getString(R.string.connect_server_wrong),
                Toast.LENGTH_LONG).show();
    }


    public void onCVContentNotFound(CheckResult cr) {
        // TODO Auto-generated method stub
        dismissProgressDialog();
        Toast.makeText(getApplicationContext(),
                getString(R.string.xml_not_exist),
                Toast.LENGTH_LONG).show();
    }


    public void onCVUpToDate(CheckResult cr) {
        // TODO Auto-generated method stub
        dismissProgressDialog();
        updateUpdatedTime();
        updateLeftPromptInfo(R.string.current_system_isnew);
        Toast.makeText(getApplicationContext(),
                getString(R.string.current_system_isnew),
                Toast.LENGTH_LONG).show();
    }


    public void onCVUpdateNeeded(CheckResult cr) {
        // TODO Auto-generated method stub
        dismissProgressDialog();
        updateUpdatedTime();
       
        dumpUpdateInfo();
        btnOperation.setText(getString(R.string.download)) ;
        //LeftPromptInfo.setText(getString(R.string.Updates_available));
        updateLeftPromptInfo(R.string.Updates_available);
    }


    public void onCVCheckNotReady(CheckResult cr) {
        // TODO Auto-generated method stub
        dismissProgressDialog();
        Debug.w("not ready for checking");
    }


    public void onCVUnknown(CheckResult cr) {
        // TODO Auto-generated method stub
        updateLeftPromptInfo(R.string.connect_server_wrong);
        dismissProgressDialog();
        Toast.makeText(getApplicationContext(),
                getString(R.string.connect_server_wrong),
                Toast.LENGTH_LONG).show();
    }
    
    private void enterInstallActivity(){
        NotificationManager mNotificationManager = (NotificationManager) getSystemService(android.content.Context.NOTIFICATION_SERVICE);
        
        Intent intent = new Intent();
        intent.setClass(this, InstallActivity.class) ;
        PendingIntent mTemp = PendingIntent.getActivity(mContext, 0, intent, 0);    
                

        Builder mBuilder = new Notification.Builder(mContext);
        mBuilder.setSmallIcon(R.drawable.ic_notification);
        mBuilder.setTicker(mContext.getString(R.string.download_complete));
        mBuilder.setContentTitle(mContext.getString(R.string.download_complete));
        mBuilder.setContentText(mContext.getString(R.string.update_package_downloaded));
        mBuilder.setContentIntent(mTemp);
        Notification mNotification = mBuilder.build();
        mNotification.flags = Notification.FLAG_AUTO_CANCEL;

        mNotificationManager.notify(127 ,mNotification);
        
        startActivity(intent);
        finish();
    }
    
    public void setUpNotification(String ticker, String title, String msg){   
        NotificationManager mNotificationManager = (NotificationManager) mContext.getSystemService(android.content.Context.NOTIFICATION_SERVICE);
        
        Builder mBuilder = new Notification.Builder(mContext);
        mBuilder.setSmallIcon(R.drawable.ic_notification);
        mBuilder.setTicker(ticker);
        mBuilder.setContentTitle(title);
        mBuilder.setContentText(msg);
        Notification mNotification = mBuilder.build();
        mNotification.flags = Notification.FLAG_AUTO_CANCEL;
        mNotificationManager.notify(128 ,mNotification);
    }
    
    private void dumpUpdateInfo() {
        String mTemp = "" ;
        List<String> mUpdateMessageList = null;
        mUpdateMessageList = UpdateApplication.instance().mUpdateInfo.getUpdateMessage();

        Utilities.setViewDisText(RightPromptInfo, getString(R.string.version_id)) ;
        mTemp = UpdateApplication.instance().mUpdateInfo.getNewVersion();
        RightPromptInfo.append(mTemp);
        RightPromptInfo.append("\n\t");
        RightPromptInfo.append(getString(R.string.Upgrade_information));
        RightPromptInfo.append("\n\t");
        for (Iterator iterator = mUpdateMessageList.iterator(); iterator.hasNext();) {
            mTemp = (String) iterator.next();
            RightPromptInfo.append(mTemp);
            RightPromptInfo.append("\n\t");
        }           
    }
    
    private String reasonToString(int errNo) {
        String ret = null;
        switch(errNo) {
            case DownloadStatus.ERROR_CONNECTIVITY:
                ret = mContext.getString(R.string.download_error_http);
                break;
            case DownloadStatus.ERROR_NO_SPACE:
                ret = mContext.getString(R.string.download_error_insufficient_space);
                break;
            case DownloadStatus.ERROR_UNKNOWN:
            default:
                ret = mContext.getString(R.string.download_error_unkowon);
        }
        return ret;
    }
    
    private void updateLeftPromptInfo(int id) {
        if (id != lastId) {
            preLastId = lastId;
            lastId = id;
            LeftPromptInfo.setText(getString(id));
        }
    }
}
