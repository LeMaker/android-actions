/**
 * @author caichsh<caichsh@artekmicro.com>
 * this class is a model(in MVC),
 *  you can new a instance of this so that you can let the file operation in a non-ui thread
 */
package com.actions.logcat;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import android.os.Handler;
import android.os.Looper;

public class FileCopyThread extends Thread{
	
	private MainActivity mContext = null;
	private Handler mHandler = null;
	private File source = null;
	private File target = null;
	private MyFileUtils fileUtils = null;
	public File getSource() {
		return source;
	}


	public void setSource(File source) {
		this.source = source;
	}

	
	public File getTarget() {
		return target;
	}


	public void setTarget(File target) {
		this.target = target;
	}


	public FileCopyThread(MainActivity mContext, Handler handler) {
		super();
		this.mContext = mContext;
		this.mHandler = handler;
	}

	

	public FileCopyThread(MainActivity mContext, Handler handler, File source, File target) {
		super();
		this.mContext = mContext;
		this.mHandler = handler;
		this.source = source;
		this.target = target;
	}


	@Override
	public void run() {
		// TODO Auto-generated method stub
		Looper.prepare();
		try {
			mHandler.sendEmptyMessage(MainActivity.WHAT_COPY_SHOW_DLG);
			synchronized (this) {
				fileUtils = new MyFileUtils(mHandler);
				fileUtils.copy(source, target);
				fileUtils = null;
			}
			mHandler.sendEmptyMessage(MainActivity.WHAT_COPY_DISMISS_DLG);
//			mHandler.sendEmptyMessage(MainActivity.WHAT_COPY_SHOW_TOAST);
			mHandler.sendMessage(mHandler.obtainMessage(MainActivity.WHAT_COPY_SHOW_TOAST, target.getAbsolutePath()));
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch(StorageFullException e) {
			if(mHandler != null) {
				mHandler.sendEmptyMessage(MainActivity.WHAT_STORAGE_FULL);
			}
			e.printStackTrace();
		}
		catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally {
			Looper.loop();
		}
	}


	public void setCustomAction(int action) {
		// TODO Auto-generated method stub
		if(fileUtils != null) {
			fileUtils.setCustomAction(action);
		}
	}

}
