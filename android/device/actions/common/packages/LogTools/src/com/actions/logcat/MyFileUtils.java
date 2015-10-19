/**
 * @author caichsh<caichsh@artekmicro.com>
 * this class is a file copy manager tool;
 */
package com.actions.logcat;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.Context;
import android.os.Handler;
import android.os.StatFs;
import android.util.Log;

public class MyFileUtils {
	private static final String TAG = "MyFileUtils";
	private static int MAX_BUFFER_SIZE = 10240;
	private long totalLength = 0;
	private long haveCopied = 0;
	private long sendRate = 1;
	private Handler mHandler = null;
	private int customAction = -1;
	private boolean isOverAll = false;
	private boolean isOverNone = false;

	public static final int ACTION_OVERRIDE_ONE = 1;
	public static final int ACTION_OVERRIDE_ALL =2;
	public static final int ACTION_OVERRIDE_NO=3;
	public static final int ACTION_OVERRIDE_NONE = 4;
	public MyFileUtils(Context context, Handler handler) {
		super();
		this.mHandler = handler;
	}
	public MyFileUtils(Handler handler) {
		super();
		this.mHandler = handler;
	}

	public void copy(File source, File target) throws FileNotFoundException, StorageFullException, IOException{
		if(source.isDirectory() && target.isDirectory()) {
			copyDir2Dir(source, target);
		} else if(source.isFile() && target.isFile()) {
			copyFile2File(source, target);
		} else if(source.isFile() && target.isDirectory()) {
			copyFile2Dir(source, target);
		} else {
			Log.w(TAG, "you can not copy a directory to a file!");
		}
	}
	private void copyFile2Dir(File source, File target) {
		// TODO Auto-generated method stub

	}
	private void copyFile2File(File source, File target) {
		// TODO Auto-generated method stub

	}
	private void copyDir2Dir(File source, File target) throws FileNotFoundException, StorageFullException, IOException{
		// TODO Auto-generated method stub
		if(!source.exists()) {
			throw new FileNotFoundException();
		}

		if(!target.exists()) {
			target.createNewFile();
		}

		File[] sources = source.listFiles();
		totalLength = getTotalFileLength(sources);
		haveCopied = 0;
		sendRate = 1;
		for(File file : sources) {
			Log.d(TAG, "tart path:" + target.getAbsolutePath());
			StatFs sf = new StatFs(target.getAbsolutePath());
			if((long)((long)sf.getAvailableBlocks()*(long)sf.getBlockSize()) < file.length()) {
				Log.d(TAG, "available blcoks:"+sf.getAvailableBlocks()+" block size:" + sf.getBlockSize()+" file lengh:"+ file.length());
				throw new StorageFullException();
			}
			mHandler.sendMessage(mHandler.obtainMessage(MainActivity.WHAT_COPY_FILE_NAME, file.getAbsolutePath()));
			copyFile(file, new File(target.getAbsolutePath() + File.separator + file.getName()));
			haveCopied += file.length();
			mHandler.sendMessage(mHandler.obtainMessage(MainActivity.WHAT_COPY_PROGRESS, (int)(((float)(haveCopied)/totalLength)*MainActivity.MAX_PROGRESS), 0));
		}
	}
	public static  long getTotalFileLength(File[] files) {
		long ret = 0;
		for(File file : files) {
			ret += file.length();
		}
		return ret;
	}
	public static  long getTotalFileLength(File dir) {
		if(dir.isDirectory()) {
			return getTotalFileLength(dir.listFiles());
		} 
		return dir.length();
	}
	private void copyFile(File source, File target) throws FileNotFoundException, IOException {
		if(!source.exists()) {
			throw new FileNotFoundException();
		}

		if(!target.exists()) {
			target.createNewFile();
		} else if(target.length() !=0 ){
			try {
				if(isOverAll) {
					doCopy(source, target);
					return;
				} else if(isOverNone) {
					return;
				}
				/*
				 * @carefully:
				 * we just format the file length (long type) to integer type simply, 
				 * if your file is more than 2G, you will get a wrong result;
				 */
				mHandler.sendMessage(mHandler.obtainMessage(MainActivity.WHAT_FILE_EXIST, (int)source.length(), (int)target.length()));
				Thread.currentThread().wait();
				switch (customAction) {
				case ACTION_OVERRIDE_ONE:
					doCopy(source, target);
					customAction = -1;
					break;
				case ACTION_OVERRIDE_ALL:
					isOverAll =true;
					doCopy(source, target);
					customAction = -1;
					break;
				case ACTION_OVERRIDE_NO:
					customAction = -1;
					break;
				case ACTION_OVERRIDE_NONE:
					isOverNone = true;
					customAction = -1;
					break;

				default:
					break;
				}
				return;
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		doCopy(source, target);

	}

	private void doCopy(File source, File target) throws FileNotFoundException, IOException{
		InputStream is = new FileInputStream(source);
		FileOutputStream fos = new FileOutputStream(target);
		byte[] buffer = new byte[MAX_BUFFER_SIZE];
		int len = -1;
		long totalLen = 0;
		int progress = 0;
		while ((len = is.read(buffer)) != -1) {
			if(len != 0) {
				fos.write(buffer, 0, len);
				totalLen += len;
				progress = (int)(((float)(totalLen+haveCopied)/totalLength)*MainActivity.MAX_PROGRESS);
				if(progress >= sendRate) {
					mHandler.sendMessage(mHandler.obtainMessage(MainActivity.WHAT_COPY_PROGRESS, progress, 0));
					sendRate++;
				}
			}
		}
		is.close();
		fos.close();
	}
	public int getCustomAction() {
		return customAction;
	}
	public void setCustomAction(int customAction) {
		this.customAction = customAction;
	}


}
