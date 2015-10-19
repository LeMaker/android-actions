/**
 * @author caichsh<caichsh@artekmicro.com>
 * this class use for show a dialog, perform the file copy process and communicate with users;
 */
package com.actions.logcat;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

public class FileCopyDialog extends Dialog implements android.view.View.OnClickListener, DialogInterface.OnDismissListener, DialogInterface.OnShowListener{

	private static final String TAG = "FileCopyDialog";
	LinearLayout alarmLayout = null;
	ProgressBar progress = null;
	TextView copyPath = null;
	TextView alarmText = null;
	Button btOverOne = null;
	Button btOverAll = null;
	Button btOverNo = null;
	Button btOverNone = null;
	View dialogView = null;
	 MainActivity mContext = null;
	public FileCopyDialog(Context context) {
		super(context);
		// TODO Auto-generated constructor stub
		mContext = (MainActivity)context;
		LayoutInflater factory = LayoutInflater.from(context);
		dialogView = factory.inflate(R.layout.progressdlg, null);
		this.setContentView(dialogView);
		findViews();
		setLisener();
	}

	private void setLisener() {
		// TODO Auto-generated method stub
		this.setOnDismissListener(this);
		this.setOnShowListener(this);
		btOverAll.setOnClickListener(this);
		btOverNo.setOnClickListener(this);
		btOverOne.setOnClickListener(this);
		btOverNone.setOnClickListener(this);
	}

	void findViews() {
		progress = (ProgressBar)dialogView.findViewById(R.id.dlg_progress);
		btOverOne = (Button)dialogView.findViewById(R.id.dlg_over);
		btOverAll = (Button)dialogView.findViewById(R.id.dlg_allover);
		btOverNo = (Button)dialogView.findViewById(R.id.dlg_noover);
		btOverNone = (Button)dialogView.findViewById(R.id.dlg_allnoover);
		alarmText = (TextView)dialogView.findViewById(R.id.dlg_alarm);
		copyPath = (TextView)dialogView.findViewById(R.id.dlg_filepath);
		alarmLayout = (LinearLayout)dialogView.findViewById(R.id.alarmLayout);
	}
	
	
	public void updateProgress(int value) {
		if(progress != null) {
			progress.setProgress(value);
		}
	}
	public void setCopyFileName(String name) {
		if(copyPath != null) {
			copyPath.setText(name);
		}
	}
	public void setMaxProgress(int value) {
		if(progress != null) {
			progress.setMax(value);
		}
	}
	
	public void setAlarmMessage(String text) {
		if(alarmText != null) {
			alarmText.setText(text);
			btOverAll.setClickable(true);
			btOverOne.setClickable(true);
			btOverNo.setClickable(true);
			btOverNone.setClickable(true);
		}
	}
	public void hideAlarmMessage() {
		if(alarmText != null) {
			alarmText.setText(R.string.defaultalarm);
			btOverAll.setClickable(false);
			btOverOne.setClickable(false);
			btOverNo.setClickable(false);
			btOverNone.setClickable(false);
		}
	}
/*	public void setAlarmMessage(String text) {
		if(alarmLayout != null) {
			alarmLayout.setVisibility(View.VISIBLE);
			alarmText.setText(text);
		}
	}
	public void hideAlarmMessage() {
		if(alarmLayout != null) {
			alarmLayout.setVisibility(View.GONE);
		}
	}
*/	@Override
	public void onClick(View view) {
		// TODO Auto-generated method stub
		switch (view.getId()) {
		case R.id.dlg_over:
			synchronized (mContext.copyThread) {
				mContext.copyThread.setCustomAction(MyFileUtils.ACTION_OVERRIDE_ONE);
				mContext.copyThread.notify();
			}
			hideAlarmMessage();
			break;
		case R.id.dlg_allover:
			synchronized (mContext.copyThread) {
				mContext.copyThread.setCustomAction(MyFileUtils.ACTION_OVERRIDE_ALL);
				mContext.copyThread.notify();
			}
			hideAlarmMessage();
			break;
		case R.id.dlg_noover:
			synchronized (mContext.copyThread) {
				mContext.copyThread.setCustomAction(MyFileUtils.ACTION_OVERRIDE_NO);
				mContext.copyThread.notify();
			}
			hideAlarmMessage();
			break;
		case R.id.dlg_allnoover:
			synchronized (mContext.copyThread) {
				mContext.copyThread.setCustomAction(MyFileUtils.ACTION_OVERRIDE_NONE);
				mContext.copyThread.notify();
			}
			hideAlarmMessage();
			break;
		default:
			break;
			
		}
	}

	@Override
	public void onDismiss(DialogInterface dialog) {
		// TODO Auto-generated method stub
		Log.v(TAG, "onDismiss");
	}

	@Override
	public void onShow(DialogInterface dialog) {
		// TODO Auto-generated method stub
		Log.v(TAG, "onShow");
		if(progress != null) {
			progress.setMax(getDefaultMax());
			progress.setProgress(0);
		}
	}

	private int getDefaultMax() {
		// TODO Auto-generated method stub
		return MainActivity.MAX_PROGRESS;
	}

	
}
