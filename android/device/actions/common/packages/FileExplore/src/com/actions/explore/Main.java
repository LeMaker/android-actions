/*
    Open Manager, an open source file manager for the Android system
    Copyright (C) 2009, 2010, 2011  Joe Berria <nexesdevelopment@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package com.actions.explore;

//mac address add
import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.DataOutputStream;

import java.io.File;

//mac address add
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;

import java.util.ArrayList;

import com.actions.explore.R;

import android.app.ActionBar;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemProperties;
import android.provider.MediaStore;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/*
 *  @author lizihao@actions-semi.com
 */
public final class Main extends ListActivity implements FileOperateCallbacks{
	private static final String PREFS_NAME = "ManagerPrefsFile";	//user preference file name
	private static final String PREFS_HIDDEN = "hidden";
	private static final String PREFS_COLOR = "color";
	private static final String PREFS_THUMBNAIL = "thumbnail";
	private static final String PREFS_SORT = "sort";
	//private static final String PREFS_STORAGE = "sdcard space";
	
	private static final int MENU_MKDIR =   0x00;
	private static final int MENU_SETTING = 0x01;
	private static final int MENU_SEARCH =  0x02;
	private static final int MENU_SPACE =   0x03;
	private static final int MENU_QUIT = 	0x04;
	private static final int MENU_BACK = 	0x05;
	private static final int MENU_MULTISELECT = 0x06;
	private static final int MENU_CANCELOPERATION = 0x07;
	private static final int MENU_PASTE = 0x08;
	private static final int SEARCH_B = 	0x09;
	private static final int MENU_CREATE_FILE = 0x10;
	private static final int MENU_APPLICATION = 0x11;
	private static final int MENU_SELECTALL = 0x12;
	
	private static final int D_MENU_DELETE = 0x05;			//context menu id
	private static final int D_MENU_RENAME = 0x06;			//context menu id
	private static final int D_MENU_COPY =   0x07;			//context menu id
	private static final int D_MENU_PASTE =  0x08;			//context menu id
	private static final int D_MENU_ZIP = 	 0x0e;			//context menu id
	private static final int D_MENU_UNZIP =  0x0f;			//context menu id
	private static final int D_MENU_MOVE = 	 0x30;			//context menu id
	
	private static final int F_MENU_BLUETOOTH = 0x11;			//context menu id
	private static final int F_MENU_MOVE = 	 0x20;			//context menu id
	private static final int F_MENU_DELETE = 0x0a;			//context menu id
	private static final int F_MENU_RENAME = 0x0b;			//context menu id
	private static final int F_MENU_ATTACH = 0x0c;			//context menu id
	private static final int F_MENU_COPY =   0x0d;			//context menu id
	
	private static final int MENU_PROPERTY = 0x01;
	
	private static final int SETTING_REQ = 	 0x10;			//request code for intent

	private FileManager mFileMag;
	private EventHandler mHandler;
	private EventHandler.TableRow mTable;
	private CatalogList mCataList;
	private DevicePath  mDevicePath;
	
	private SharedPreferences mSettings;
	private boolean mReturnIntent = false;
	private boolean mHoldingZip = false;
	private boolean mUseBackKey = true;
	private String mCopiedTarget;
	private String mZippedTarget;
	private String mSelectedListItem;				//item from context menu
	private TextView  mPathLabel, mDetailLabel;
		
	private BroadcastReceiver mReceiver;

	private String TAG = "Actions FileManager";
	
	private String openType;
	private File openFile;
	
	//LinearLayout hidden_lay; 
	
	@Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.main);
        
        LinearLayout mLinearLayout = (LinearLayout) findViewById(R.id.left);
        if(android.os.SystemProperties.get("ro.product.manufacturer", "").equals("Meizu")){
        	mLinearLayout.setVisibility(View.GONE);
        }
        
        /*read settings*/
        mSettings = getSharedPreferences(PREFS_NAME, 0);
        boolean hide = mSettings.getBoolean(PREFS_HIDDEN, false);
        boolean thumb = mSettings.getBoolean(PREFS_THUMBNAIL, false);
        //int space = mSettings.getInt(PREFS_STORAGE, View.VISIBLE);
        int color = mSettings.getInt(PREFS_COLOR, Color.BLACK);
        int sort = mSettings.getInt(PREFS_SORT, 1);

		mFileMag = new FileManager(this);
        mFileMag.setShowHiddenFiles(hide);
        mFileMag.setSortType(sort);
        
        mCataList = new CatalogList(this);
        mDevicePath = new DevicePath(this);
        
        mHandler = new EventHandler(Main.this, this, mFileMag,mCataList);
        mHandler.setTextColor(color);
        mHandler.setShowThumbnails(thumb);
        mTable = mHandler.new TableRow();
        
        /*sets the ListAdapter for our ListActivity and
         *gives our EventHandler class the same adapter
         */
        mHandler.setListAdapter(mTable);
        setListAdapter(mTable);
        
        /* register context menu for our list view */
        registerForContextMenu(getListView());
        
        mDetailLabel = (TextView)findViewById(R.id.detail_label);
        mPathLabel = (TextView)findViewById(R.id.path_label);
        mHandler.setUpdateLabels(mPathLabel, mDetailLabel);
        
        ActionBar actionBar = getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
        
        //hidden_lay = (LinearLayout)findViewById(R.id.hidden_buttons);
		
        /*
         * change to the specific file path if this component is started by other applications
         */
        Bundle bundle = getIntent().getExtras();
        if (bundle != null)
        {
			//Log.i(TAG, "intent action ="+getIntent().getAction());
        	String path = bundle.getString("Path");
			//Log.i(TAG, "path = "+path);
			if (path != null)
        	{
        		if(path.equals( mDevicePath.getUsbStoragePath() ) ) {
        			mPathLabel.setText(path);
        			mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_USBHOST));
        			getFocusForButton(R.id.home_usbhost_button);
        		}
        		else if (path.equals(mDevicePath.getInterStoragePath())) {
        			mPathLabel.setText(path);
        			mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_SDCARD));
        			getFocusForButton(R.id.home_sdcard_button);
        		}
        		else if (path.equals(mDevicePath.getSdStoragePath()) ) {
        			mPathLabel.setText(path);
        			mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_FLASH));
        			getFocusForButton(R.id.home_flash_button);
        		}
        	}
			else
			{
				//default path        	
	        	mPathLabel.setText(mDevicePath.getInterStoragePath());
	        	mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_SDCARD));
	        	getFocusForButton(R.id.home_sdcard_button);
			}
        }
        else { 
        	//default path
        	//mPathLabel.setText(mDevicePath.getInterStoragePath());
        	mPathLabel.setText(mDevicePath.getInterStoragePath().substring(5, mDevicePath.getInterStoragePath().length()));
        	mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_SDCARD));
        	getFocusForButton(R.id.home_sdcard_button);
        }

		/* setup buttons */
        int[] img_button_id = {R.id.home_flash_button,
        					   R.id.home_sdcard_button,R.id.home_usbhost_button,
        					   R.id.image_button,R.id.movie_button,
        					   R.id.music_button,R.id.ebook_button,R.id.apk_button};
        
        int[] button_id = {R.id.hidden_copy, R.id.hidden_delete, R.id.hidden_move};
        
        ImageButton[] bimg = new ImageButton[img_button_id.length];
        Button[] bt = new Button[button_id.length];
		
        for(int i = 0; i < img_button_id.length; i++) {
        	bimg[i] = (ImageButton)findViewById(img_button_id[i]);
        	bimg[i].setOnClickListener(mHandler);
        	
        	if(android.os.SystemProperties.get("ro.product.manufacturer", "").equals("Meizu")){
        		bimg[i].setVisibility(View.GONE);
        	}
        	
        	if(i < button_id.length) {
        		bt[i] = (Button)findViewById(button_id[i]);
        		bt[i].setOnClickListener(mHandler);
        	}
        }
        
        /*if(!(android.os.Environment.getExternalStorageState().equals(android.os.Environment.MEDIA_MOUNTED))
        		&& !(android.os.Environment.getTfcardStorageState().equals(android.os.Environment.MEDIA_MOUNTED)
        		&& !(android.os.Environment.getUhostStorageState().equals(android.os.Environment.MEDIA_MOUNTED)))){
        	System.out.println("--->sdcard tfcard uhost is unmount");
        	for(int i = 0; i < img_button_id.length; i++) {
        		bimg[i].setVisibility(View.GONE);
        	}
        }*/

		if( getIntent().getAction() != null ){
			if(getIntent().getAction().equals(Intent.ACTION_GET_CONTENT)) {
        	bimg[5].setVisibility(View.GONE);
			
        	mReturnIntent = true;
			
			}
		}
        
        //register reciver to process sdcard out message
        mReceiver = new BroadcastReceiver() {   
        	@Override  
        	public void onReceive(Context context, Intent intent) {   
        		String tmpstring = intent.getData().toString();
        		
        		String dataOfUsb = mDevicePath.getUsbStoragePath();
        		String dataOfSd = mDevicePath.getInterStoragePath();
        		String dataOfFlash = mDevicePath.getSdStoragePath();
        		
        		if(intent.getAction().equals(Intent.ACTION_MEDIA_REMOVED) ||
        				intent.getAction().equals(Intent.ACTION_MEDIA_BAD_REMOVAL))
        		{
        			//Log.d(TAG, tmpstring);
        			/*
        			 * waiting for unmounted really 
        			 */
        			try
					{
						Thread.currentThread().sleep(1000);
					}
					catch(Exception e) {};

					Log.d("mydebug","getMode:"+mHandler.getMode());
					Log.d("mydebug","tmpstring:"+tmpstring);
        			switch(mHandler.getMode())
        			{
        			case EventHandler.TREEVIEW_MODE:
        				
        				if(tmpstring.contains(dataOfSd))
        				{
        				//	DisplayToast(getResources().getString(R.string.sdcard_out));
        					if(mFileMag.getCurrentDir().startsWith(dataOfSd))
        					{
        						mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_SDCARD));
        					}
        				}
        				else if(tmpstring.contains(dataOfUsb))
        				{ 
        				//	DisplayToast(getResources().getString(R.string.usb_out));	
        					if(mFileMag.getCurrentDir().startsWith(dataOfUsb))
        					{
        						mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_USBHOST));
        					}
        				}
        				else if(tmpstring.contains(dataOfFlash))
        				{
        				//	DisplayToast(getResources().getString(R.string.flash_out));
        					if(mFileMag.getCurrentDir().startsWith(dataOfFlash))
        					{
        						mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_FLASH));
        					}
        				}
        				else
        				{
        					return;
        				}
        				if(mHandler.isMultiSelected()) 
        				{
        					mTable.killMultiSelect(true);
        					DisplayToast(getResources().getString(R.string.Multi_select_off));
        				}
        				if(mPathLabel != null)
        					mPathLabel.setText(mFileMag.getCurrentDir());
        				break;
        				//anyway,remove the list in media storage
        			case EventHandler.CATALOG_MODE:
        				ArrayList<String> content = null;
        				if(tmpstring.contains(dataOfSd))
        				{
        				//	DisplayToast(getResources().getString(R.string.sdcard_out));
        					content = mCataList.DisAttachMediaStorage(CatalogList.STORAGE_SDCARD);
        				}
        				else if(tmpstring.contains(dataOfUsb))
        				{ 
        				//	DisplayToast(getResources().getString(R.string.usb_out));
        					content = mCataList.DisAttachMediaStorage(CatalogList.STORAGE_USBHOST);			
        				}
        				else if(tmpstring.contains(dataOfFlash))
        				{
        				//	DisplayToast(getResources().getString(R.string.flash_out));
        					content = mCataList.DisAttachMediaStorage(CatalogList.STORAGE_FLASH);
        				}
        				else
        				{
        					return;
        				}
        				if(content != null)
        				{
        					mHandler.setFileList(content);
        				}
        				break;
        			}
        		}
        		else if (intent.getAction().equals(Intent.ACTION_MEDIA_MOUNTED))
        		{
        			switch(mHandler.getMode())
        			{
        			case EventHandler.TREEVIEW_MODE:
        				if(tmpstring.contains(dataOfSd))
        				{
        				//	DisplayToast(getResources().getString(R.string.sdcard_in));
        					if(mFileMag.getCurrentDir().startsWith(dataOfSd))
        					{
        						mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_SDCARD));
        					}
        				}
        				else if(tmpstring.contains(dataOfUsb))
        				{
        				//	DisplayToast(getResources().getString(R.string.usb_in));	
        					if(mFileMag.getCurrentDir().startsWith(dataOfUsb))
        					{
        						mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_USBHOST));
        					}
        				}
        				else if(tmpstring.contains(dataOfFlash))
        				{
        				//	DisplayToast(getResources().getString(R.string.flash_in));
        					if(mFileMag.getCurrentDir().startsWith(dataOfFlash))
        					{
        						mHandler.updateDirectory(mFileMag.getHomeDir(FileManager.ROOT_FLASH));
        					}
        				}
        				else 
        				{
        					return;
        				}
        				break;
        			case EventHandler.CATALOG_MODE:
        				if(tmpstring.contains(dataOfSd))
        				{
        				//	DisplayToast(getResources().getString(R.string.sdcard_in));
        					mCataList.AttachMediaStorage(CatalogList.STORAGE_SDCARD);
        				}
        				else if(tmpstring.contains(dataOfUsb))
        				{ 
        				//	DisplayToast(getResources().getString(R.string.usb_in));
        					mCataList.AttachMediaStorage(CatalogList.STORAGE_USBHOST);
        				}
        				else if(tmpstring.contains(dataOfFlash))
        				{
        				//	DisplayToast(getResources().getString(R.string.flash_in));
        					mCataList.AttachMediaStorage(CatalogList.STORAGE_FLASH);
        				}
        				else
        				{
        					return;
        				}
        				
        				mHandler.setFileList(mCataList.listSort());
        				
        				break;
        			}
        		}
        	}   
        };
		
        IntentFilter filter = new IntentFilter();   
        filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        filter.addAction(Intent.ACTION_MEDIA_REMOVED);
        filter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
        filter.addDataScheme("file"); 
        registerReceiver(mReceiver, filter);
    }
	
	private void getFocusForButton(int id)
	{
		View v = findViewById(id);
		mHandler.getInitView(v);
		v.setSelected(true);
	}
	
	private void DisplayToast(String str){
		Toast.makeText(getBaseContext(), str, Toast.LENGTH_SHORT).show();
	}
	
	@Override  
    protected void onDestroy() {   
        // TODO Auto-generated method stub   
        super.onDestroy();   
        unregisterReceiver(mReceiver);   
    }   
	
	/*(non Java-Doc)
	 * Returns the file that was selected to the intent that
	 * called this activity. usually from the caller is another application.
	 */
	private boolean returnIntentResults(File data) {
		mReturnIntent = false;
		String action = getIntent().getAction();
		String type = getIntent().getType();
		Intent ret ;
		if(action.equals(Intent.ACTION_GET_CONTENT)){
			if(type == null) return false;
			if(type.equals("image/*")){
				//CropImage crop = new CropImage(this, getIntent(), data.getAbsolutePath());
				//ret = crop.saveResourceToIntent();
				ret = new Intent();
				ret.setData(Uri.fromFile(data));
				setResult(RESULT_OK, ret);
			}
			else{
				ret = new Intent();
				ret.setData(Uri.fromFile(data));
				setResult(RESULT_OK, ret);
			}
			finish();
			return true;
		}
		return false;
	}
		
	private String getCurrentFileName(int position){
		final String item = mHandler.getData(position);
		
    	if(mHandler.getMode() == EventHandler.TREEVIEW_MODE)
    	{
    		return (mFileMag.getCurrentDir() + "/" + item);
    	}
    	
    	return item;
	}
	/**
	 *  To add more functionality and let the user interact with more
	 *  file types, this is the function to add the ability. 
	 *  
	 *  (note): this method can be done more efficiently 
	 */
    @Override
    public void onListItemClick(ListView parent, View view, int position, long id) {
    	final String item = getCurrentFileName(position);
    	File file = new File(item);
    	boolean multiSelect = mHandler.isMultiSelected();
    	
    	String item_ext = null;
    	
    	try {
    		item_ext = item.substring(item.lastIndexOf(".") + 1, item.length());
    		
    	} catch(IndexOutOfBoundsException e) {
    		item_ext = "";
    	}
    	
    	/*
    	 * If the user has multi-select on, we just need to record the file
    	 * not make an intent for it.
    	 */
    	if(multiSelect) {
    		mTable.addMultiPosition(position, file.getPath());
    		//hidden_lay.setVisibility(LinearLayout.VISIBLE);
    	} else {
    		//hidden_lay.setVisibility(LinearLayout.GONE);
	    	if (file.isDirectory()) {
				if(file.canRead()) {
		    		mHandler.updateDirectory(mFileMag.getNextDir(item, true));
		    		//mPathLabel.setText(mFileMag.getCurrentDir());
		    		mPathLabel.setText(mFileMag.getCurrentDir().substring(5, mFileMag.getCurrentDir().length()));
		    		
		    		/*set back button switch to true 
		    		 * (this will be better implemented later)
		    		 */
		    		if(!mUseBackKey)
		    			mUseBackKey = true;
		    		
	    		} else {
	    			Toast.makeText(this, getResources().getString(R.string.Cant_read_folder), 
	    							Toast.LENGTH_SHORT).show();
	    		}
	    	}
	    	
	    	/*music file selected--add more audio formats*/
	    	else if (TypeFilter.getInstance().isMusicFile(item_ext)) {
	    		
	    		if(mReturnIntent) {
	    			returnIntentResults(file);
	    		} else { 
                    Intent musicIntent = new Intent();
                    musicIntent.setAction(android.content.Intent.ACTION_VIEW);
                    musicIntent.setDataAndType(Uri.fromFile(file), "audio/*");
                    startActivity(musicIntent);
	    		}
	    	}
	    	
	    	/*photo file selected*/
	    	else if(TypeFilter.getInstance().isPictureFile(item_ext)) {
	 			    		
	    		if (file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
			    		Intent picIntent = new Intent();
			    		picIntent.setAction(android.content.Intent.ACTION_VIEW);
			    		picIntent.setDataAndType(Uri.fromFile(file), "image/*");
			    		startActivity(picIntent);
	    			}
	    		}
	    	}
	    	
	    	/*video file selected--add more video formats*/
	    	else if(TypeFilter.getInstance().isMovieFile(item_ext)) {
	    		
	    		if (file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
			    		Intent movieIntent = new Intent();
			    		
			    		//for VideoPlayer to create playlist
			    		//movieIntent.putExtra(MediaStore.PLAYLIST_TYPE, MediaStore.PLAYLIST_TYPE_CUR_FOLDER);
			    		
			    		movieIntent.putExtra(MediaStore.EXTRA_FINISH_ON_COMPLETION, false);
			    		movieIntent.setAction(android.content.Intent.ACTION_VIEW);
			    		movieIntent.setDataAndType(Uri.fromFile(file), "video/*");
			    		startActivity(movieIntent);
	    			}
	    		}
	    	}
	    	
	    	/*zip file */
	    	else if(TypeFilter.getInstance().isZipFile(item_ext)) {
	    		
	    		if(mReturnIntent) {
	    			returnIntentResults(file);
	    			
	    		} else {
		    		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		    		AlertDialog alert;
		    		mZippedTarget = item;
		    		CharSequence[] option = {getResources().getString(R.string.Extract_here), getResources().getString(R.string.Extract_to)};
		    		
		    		builder.setTitle(getResources().getString(R.string.extract));
		    		builder.setItems(option, new DialogInterface.OnClickListener() {
		
						public void onClick(DialogInterface dialog, int which) {
							switch(which) {
								case 0:
									String dir = mFileMag.getCurrentDir();
									mHandler.unZipFile(item, dir + "/");
									break;
									
								case 1:
									mDetailLabel.setText(getResources().getString(R.string.Holding) + " " + item + 
														" " + getResources().getString(R.string.to_extract));
									mHoldingZip = true;
									break;
							}
						}
		    		});
		    		
		    		alert = builder.create();
		    		alert.show();
	    		}
	    	}
	    	
	    	/* gzip files, this will be implemented later */
	    	else if(TypeFilter.getInstance().isGZipFile(item_ext)) {
	    		
	    		if(mReturnIntent) {
	    			returnIntentResults(file);
	    			
	    		} else {
	    			//TODO:
	    		}
	    	}
	    	
	    	/*pdf file selected*/
	    	else if(TypeFilter.getInstance().isPdfFile(item_ext)) {
	    		
	    		if(file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
			    		Intent pdfIntent = new Intent();
			    		pdfIntent.setAction(android.content.Intent.ACTION_VIEW);
			    		pdfIntent.setDataAndType(Uri.fromFile(file), 
			    								 "application/pdf");
			    		
			    		try {
			    			startActivity(pdfIntent);
			    		} catch (ActivityNotFoundException e) {
			    			Toast.makeText(this, getResources().getString(R.string.count_not_open_pdf), 
									Toast.LENGTH_SHORT).show();
			    		}
		    		}
	    		}
	    	}
	    	
	    	/*Android application file*/
	    	else if(TypeFilter.getInstance().isApkFile(item_ext)){
	    		
	    		if(file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
		    			Intent apkIntent = new Intent();
		    			apkIntent.setAction(android.content.Intent.ACTION_VIEW);
		    			apkIntent.setDataAndType(Uri.fromFile(file), "application/vnd.android.package-archive");
		    			startActivity(apkIntent);
	    			}
	    		}
	    	}
			/* powerpoint, excel, word file*/
			else if(TypeFilter.getInstance().isWordFile(item_ext) || TypeFilter.getInstance().isExcelFile(item_ext) || TypeFilter.getInstance().isPptFile(item_ext) ){
				if(file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
	    				String errText;
		    			Intent MSIntent = new Intent();
		    			MSIntent.setAction(android.content.Intent.ACTION_VIEW);
						if(TypeFilter.getInstance().isWordFile(item_ext)){
							errText = getResources().getString(R.string.count_not_open_word);
							MSIntent.setDataAndType(Uri.fromFile(file), "application/ms-word"); 
						}
						else if(TypeFilter.getInstance().isExcelFile(item_ext)){
							errText = getResources().getString(R.string.count_not_open_excel);
		    				MSIntent.setDataAndType(Uri.fromFile(file), "application/vnd.ms-excel");
						}
						else{
							errText = getResources().getString(R.string.count_not_open_ppt);
							MSIntent.setDataAndType(Uri.fromFile(file), "application/vnd.ms-powerpoint");
						}
		    			
		    			try {
		    				startActivity(MSIntent);
		    			} catch(ActivityNotFoundException e) {
		    					Toast.makeText(this, errText, Toast.LENGTH_SHORT).show();
		    			}
	    			}
	    		}
			}

	    	/* HTML file */
	    	else if(TypeFilter.getInstance().isHtml32File(item_ext)) {
	    		
	    		if(file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
		    			Intent htmlIntent = new Intent();
		    			htmlIntent.setAction(android.content.Intent.ACTION_VIEW);
		    			htmlIntent.setDataAndType(Uri.fromFile(file), "text/html");
		    			
		    			try {
		    				startActivity(htmlIntent);
		    			} catch(ActivityNotFoundException e) {
		    				Toast.makeText(this, getResources().getString(R.string.count_not_open_html), 
		    									Toast.LENGTH_SHORT).show();
		    			}
	    			}
	    		}
	    	}
	    	
	    	/* text file*/
	    	else if(TypeFilter.getInstance().isTxtFile(item_ext)) {

	    		if(file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
	    				if((file.length() != -1) && (file.length() <= 204800)) {
	    					Intent txtIntent = new Intent();
		    				txtIntent.setClass(this, CreateFile.class);
		    				txtIntent.putExtra("type", "read");
		    				txtIntent.putExtra("path", file.getPath());
		    				startActivityForResult(txtIntent, 0);
	    				} else {
	    					System.out.println("Txt file is too big");
	    					Intent txtIntent = new Intent();
			    			txtIntent.setAction(android.content.Intent.ACTION_VIEW);
			    			txtIntent.setDataAndType(Uri.fromFile(file), "text/plain");
			    			
			    			try {
			    				startActivity(txtIntent);
			    			} catch(ActivityNotFoundException e) {
			    				txtIntent.setType("text/*");
			    				startActivity(txtIntent);
			    			}
	    				}
	    				
	    			}
	    		}
	    	}
	    	
	    	else if(TypeFilter.getInstance().isPadFile(item_ext)) {
	    		if(file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
	    				Intent txtIntent = new Intent();
	    				txtIntent.setClass(this, CreateFile.class);
	    				txtIntent.putExtra("type", "read");
	    				txtIntent.putExtra("path", file.getPath());
	    				startActivityForResult(txtIntent, 0);
	    			}
	    		}
	    	}
//mac address add start	    	
	    	else if(TypeFilter.getInstance().isShellFile(item_ext)) {
	    		final File sFile = file;
                if(file.exists()) {
                    if(mReturnIntent) {
                        returnIntentResults(file);
                        
                    } else {
                        Log.i("caichsh","show dialog");
                    	AlertDialog ad = new AlertDialog.Builder(Main.this)
                    	.setTitle("SURE TO RUN THIS :")
                    	.setMessage(sFile.getName())
                    	.setPositiveButton("OK", new DialogInterface.OnClickListener() {
							
							@Override
							public void onClick(DialogInterface arg0, int arg1) {
								// TODO Auto-generated method stub
								 // execShell(sFile.getAbsolutePath());
							    new ShellRunTask().execute(new String[]{sFile.getAbsolutePath()});
							}
						})
						.setNegativeButton("NO", new DialogInterface.OnClickListener() {
							
							@Override
							public void onClick(DialogInterface arg0, int arg1) {
								// TODO Auto-generated method stub
							}
						})
						.show();
                    }
                }
            }	    	
//mac address add end
	    	/* generic intent */
	    	else {
	    		if(file.exists()) {
	    			if(mReturnIntent) {
	    				returnIntentResults(file);
	    				
	    			} else {
	    				openFile = file;
			    		selectFileType_dialog();
	    			}
	    		}
	    	}
    	}
	}
//mac address add start 
     class ShellRunTask extends AsyncTask<String, Void, ShellLog> {

        @Override
        protected ShellLog doInBackground(String... arg0) {
            // TODO Auto-generated method stub
            return execShell(arg0[0]);
        }

        @Override
        protected void onPostExecute(ShellLog result) {
            // TODO Auto-generated method stub
            if(result.doesWorkOk()) {
				if(result.needWarn()) {
                    AlertDialog ad = new AlertDialog.Builder(Main.this)
                    .setTitle("MAC ADDRESSES LIMITED!")
                    .setMessage("Warning:"+ result.getErrorString())
                    .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    
                        @Override
                        public void onClick(DialogInterface arg0, int arg1) {
                        // TODO Auto-generated method stub
                        }
                    })
                    .setNegativeButton("NO", new DialogInterface.OnClickListener() {
                    
                        @Override
                        public void onClick(DialogInterface arg0, int arg1) {
                            // TODO Auto-generated method stub
                        }
                    })
                    .show();
                }else{
				
                    Toast.makeText(Main.this, "SUCCESS", Toast.LENGTH_LONG).show();
				    Toast.makeText(Main.this, "SUCCESS", Toast.LENGTH_LONG).show();
				}
				
            }  else {
                Toast.makeText(Main.this, "FAILED" + result.getErrorString(), Toast.LENGTH_LONG).show();
				Toast.makeText(Main.this, "FAILED" + result.getErrorString(), Toast.LENGTH_LONG).show();
            }
        }
        
        
    }
    static final boolean TEST = false;
    
    private ShellLog execShell(String path){
       try{  
            Process p = Runtime.getRuntime().exec("su");  
			int index = path.lastIndexOf('/');
        	String dir = path.substring(0, index);
			Log.i("caichsh", "dir:" +dir);
            OutputStream outputStream = p.getOutputStream();
            DataOutputStream dataOutputStream=new DataOutputStream(outputStream);
			Log.i("caichsh", "/system/bin/sh " + path+" "+dir);
            dataOutputStream.writeBytes("/system/bin/sh " + path+" "+dir);
            dataOutputStream.flush();
            dataOutputStream.close();
            outputStream.close();

			InputStream inputStream = null;
			if(TEST) {
			    File test = new File("/data/test.txt");
			    inputStream = new FileInputStream(test);
			} else {
			    inputStream = p.getInputStream();
			}
			InputStreamReader isr = new InputStreamReader(inputStream);
			BufferedReader bf = new BufferedReader(isr);
			
			StringBuffer sb = new StringBuffer();
			String line;
			while ((line = bf.readLine()) != null) {
			    Log.i("caichsh", "read line:" +line);
			   sb.append(line+"\n");
			}
			inputStream.close();
			
			return parseLog(sb.toString());
			
       } catch(Throwable t) {  
             t.printStackTrace();  
       } 
       
       return new ShellLog(ShellLog.ERR_UNCOUGHT);
    }

    class ShellLog {
        public static final int ERR_NO_ERROR = 0;
        public static final int ERR_UNDEFINEED = -1;
        public static final int ERR_WARNNING = -2;
        public static final int ERR_UNKNOWN = -9;
        public static final int ERR_UNCOUGHT = -10;
        
        private int errno;
        private String state;
        
        public ShellLog(int err) {
            errno = err;
        }
        
        public ShellLog(int err, String stat) {
            errno = err;
            state = stat;
        }
        
        public boolean doesWorkOk() {
            if((errno == ERR_NO_ERROR)||(errno == ERR_WARNNING))
                return true;
            return false;
        }
        
        public boolean needWarn() {
            if(errno == ERR_WARNNING)
                return true;
            return false;
        }
        public String getErrorString() {
            String s;
            switch (errno) {
                case ERR_NO_ERROR:
                    s = "Work Perfectly!";
                    break;

				case ERR_WARNNING:
					s = state;
                    break;
                case ERR_UNDEFINEED:
                    s = state;
                    break;
                default:
                    s = "Unknown error!";
                    break;
            }
            return s;
        }
    }

	private ShellLog parseLog(String line) {
		// TODO Auto-generated method stub
	    Log.i("caichsh","got log:" + line);
		if(line == null || line.length() == 0)
			return new ShellLog(ShellLog.ERR_UNKNOWN);
		if(line.contains("warning:")) {
			int index = line.indexOf("warning:");
			int warnStart = index + 8;
			int warnEnd = line.indexOf("\n", warnStart);
			String warnLog = line.substring(warnStart, warnEnd);
			return new ShellLog(ShellLog.ERR_WARNNING, warnLog);
		    //return new ShellLog(ShellLog.ERR_WARNNING);
		}
		
		if(line.contains("success:"))
		    return new ShellLog(ShellLog.ERR_NO_ERROR);
		
		if(line.contains("error:")) {
			int index = line.indexOf("error:");
			int errStart = index + 5;
			int errEnd = line.indexOf("\n", errStart);
			String errLog = line.substring(errStart, errEnd);
			return new ShellLog(ShellLog.ERR_UNDEFINEED, errLog);
		}
		

		return new ShellLog(ShellLog.ERR_UNKNOWN);
	}   
//mac address add end	
    private void selectFileType_dialog() {
    	String mFile = Main.this.getResources().getString(R.string.open_file);
		String mText = Main.this.getResources().getString(R.string.text);
		String mAudio = Main.this.getResources().getString(R.string.audio);
		String mVideo = Main.this.getResources().getString(R.string.video);
		String mImage = Main.this.getResources().getString(R.string.image);
		CharSequence[] FileType = {mText,mAudio,mVideo,mImage};
		AlertDialog.Builder builder;
    	AlertDialog dialog;
		builder = new AlertDialog.Builder(Main.this);
		builder.setTitle(mFile);
		builder.setIcon(R.drawable.help);
		builder.setItems(FileType, new DialogInterface.OnClickListener() {
			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				// TODO Auto-generated method stub
				Intent mIntent = new Intent();
				switch(which) {
				case 0:
					openType = "text/*";
					break;
				case 1:
					openType = "audio/*";
					break;
				case 2:
					openType = "video/*";
					break;
				case 3:
					openType = "image/*";
					break;
				}
				mIntent.setAction(android.content.Intent.ACTION_VIEW);
				mIntent.setDataAndType(Uri.fromFile(openFile), openType);
				try {
	    			startActivity(mIntent);
	    		} catch(ActivityNotFoundException e) {
	    			Toast.makeText(Main.this, getResources().getString(R.string.count_not_open) +
	    						   " " + getResources().getString(R.string.to_open) + " " + openFile.getName(), 
	    						   Toast.LENGTH_SHORT).show();
	    		}
			}
		});	
		dialog = builder.create();
    	dialog.show();
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    	super.onActivityResult(requestCode, resultCode, data);
    	
    	SharedPreferences.Editor editor = mSettings.edit();
    	boolean check;
    	boolean thumbnail;
    	int color, sort;
    	
    	/* resultCode must equal RESULT_CANCELED because the only way
    	 * out of that activity is pressing the back button on the phone
    	 * this publishes a canceled result code not an ok result code
    	 */
    	if(requestCode == SETTING_REQ && resultCode == RESULT_CANCELED) {
    		//save the information we get from settings activity
    		check = data.getBooleanExtra("HIDDEN", false);
    		thumbnail = data.getBooleanExtra("THUMBNAIL", false);
    		color = data.getIntExtra("COLOR", -1);
    		sort = data.getIntExtra("SORT", 0);
    		
    		editor.putBoolean(PREFS_HIDDEN, check);
    		editor.putBoolean(PREFS_THUMBNAIL, thumbnail);
    		editor.putInt(PREFS_COLOR, color);
    		editor.putInt(PREFS_SORT, sort);
    		//editor.putInt(PREFS_STORAGE, space);
    		editor.commit();
    		
    		mFileMag.setShowHiddenFiles(check);
    		mFileMag.setSortType(sort);
    		mHandler.setTextColor(color);
    		mHandler.setShowThumbnails(thumbnail);
    		if(mHandler.getMode() == EventHandler.TREEVIEW_MODE){
    			mHandler.updateDirectory(mFileMag.getNextDir(mFileMag.getCurrentDir(), true));
    		}
    	} else {
    		if(mHandler.getMode() == EventHandler.TREEVIEW_MODE){
    			mHandler.updateDirectory(mFileMag.getNextDir(mFileMag.getCurrentDir(), true));
    		}
    	}
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	// add menu by lizihao
    	menu.add(0, MENU_CREATE_FILE, 0, getResources().getString(R.string.new_file)).setVisible(true);
    	menu.add(0, MENU_MKDIR, 0, getResources().getString(R.string.New_Directory));
    	menu.add(0, MENU_SEARCH, 0, getResources().getString(R.string.Search));
    	menu.add(0, MENU_MULTISELECT, 0, getResources().getString(R.string.Multiselect));
    	menu.add(0, MENU_SELECTALL, 0, getResources().getString(R.string.select_all)).setVisible(false);
    	menu.add(0, MENU_CANCELOPERATION, 0, getResources().getString(R.string.cancel_file_operation)).setVisible(false);
    	menu.add(0, MENU_PASTE, 0, getResources().getString(R.string.Paste)).setVisible(true);
    	menu.add(0, MENU_BACK, 0, getResources().getString(R.string.Back)).setIcon(R.drawable.ic_back).setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
    	menu.add(0, MENU_SETTING, 0, getResources().getString(R.string.Settings)).setIcon(R.drawable.ic_menu_preferences).setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
    	menu.add(0, MENU_APPLICATION, 0, getResources().getString(R.string.app_management)).setIcon(R.drawable.appicon).setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
    	menu.add(0, MENU_QUIT, 0, getResources().getString(R.string.Quit)).setIcon(R.drawable.ic_exit).setVisible(false).setShowAsAction(MenuItem.SHOW_AS_ACTION_IF_ROOM);
    	
    	return true;
    }
    
    @Override
	public boolean onPrepareOptionsMenu(Menu menu) {
		// TODO Auto-generated method stub
    	MenuItem menuCreateFile = menu.findItem(MENU_CREATE_FILE);
    	MenuItem menuMK = menu.findItem(MENU_MKDIR);
    	MenuItem menuCancelOperation = menu.findItem(MENU_CANCELOPERATION);
    	MenuItem menuMulti = menu.findItem(MENU_MULTISELECT);
    	MenuItem menuSelAll = menu.findItem(MENU_SELECTALL);
    	MenuItem menuPaste = menu.findItem(MENU_PASTE);
    	MenuItem menuSearch = menu.findItem(MENU_SEARCH);
    	boolean mount_status = false;
    	
    	if(FileManager.canMkdir){
	    	if(mFileMag.createDir(mFileMag.getCurrentDir() + "/", ".actions_mount") == -1){
	    		mount_status = false;
	    		//System.out.println("" + mFileMag.getCurrentDir() + " unmount");
	    	} else {
	    		mount_status = true;
	    		//System.out.println("" + mFileMag.getCurrentDir() + " mounted");
	    		mFileMag.deleteTarget(mFileMag.getCurrentDir() + "/.actions_mount");
	    	}
    	}
    	
    	if(FileManager.mAllowSelectAll){
        	menuSelAll.setVisible(true);
    	} else {
    		menuSelAll.setVisible(false);
    	}
    	
    	if(FileManager.moveOrCopyFile || mHoldingZip) {
    		menuMulti.setVisible(false);
    		menuCreateFile.setVisible(false);
    		menuMK.setVisible(false);
    		menuSearch.setVisible(false);
    		menuCancelOperation.setVisible(true);
    		if(FileManager.mHoldingFile){
    		    menuPaste.setVisible(true);
    		} else {
    			menuPaste.setVisible(false);
    		}
    	} else {
    		menuMulti.setVisible(true);
    		menuCreateFile.setVisible(true);
    		menuCancelOperation.setVisible(false);
    		menuPaste.setVisible(false);
    		menuMK.setVisible(true);
    		menuSearch.setVisible(true);
    	}
    	
    	if(mFileMag.canMkdir && mount_status){
    		menuMK.setEnabled(true);
    		menuMulti.setEnabled(true);
    		menuCreateFile.setEnabled(true);
    		menuSearch.setEnabled(true);
    	} else if(mFileMag.canMkdir && !mount_status){
    		menuMK.setEnabled(false);
    		menuMulti.setEnabled(false);
    		menuCreateFile.setEnabled(false);
    		menuSearch.setEnabled(false);
    	} else {
    		menuMK.setEnabled(false);
    		menuMulti.setEnabled(false);
    		menuCreateFile.setEnabled(false);
    		menuSearch.setEnabled(false);
    	}
		return super.onPrepareOptionsMenu(menu);
	}

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
    	switch(item.getItemId()) {
    		case android.R.id.home:
    			mFileMag.canMkdir = true;
    			mFileMag.moveOrCopyFile = false;
    			mFileMag.mAllowSelectAll = false;
    			finish();
    			return true;
    		case MENU_MKDIR:
    			showDialog_fun(MENU_MKDIR);
    			return true;
    			
    		case MENU_SEARCH:
    			showDialog_fun(MENU_SEARCH);
    			return true;
    			
    		case MENU_SPACE:
    			return true;
    			
    		case MENU_SETTING:
    			Intent settings_int = new Intent(this, SettingPreference.class);
    			settings_int.putExtra("HIDDEN", mSettings.getBoolean(PREFS_HIDDEN, false));
    			settings_int.putExtra("THUMBNAIL", mSettings.getBoolean(PREFS_THUMBNAIL, false));
    			settings_int.putExtra("COLOR", mSettings.getInt(PREFS_COLOR, -1));
    			settings_int.putExtra("SORT", mSettings.getInt(PREFS_SORT, 1));
    			//settings_int.putExtra("SPACE", mSettings.getInt(PREFS_STORAGE, View.VISIBLE));
    			
    			startActivityForResult(settings_int, SETTING_REQ);
    			return true;
    			
    		case MENU_QUIT:
    			mFileMag.canMkdir = true;
    			mFileMag.moveOrCopyFile = false;
    			mFileMag.mAllowSelectAll = false;
    			finish();
    			return true;
    			
    		case MENU_MULTISELECT:
    			mFileMag.moveOrCopyFile = true;
    			mFileMag.mAllowSelectAll = true;
    			mHandler.multiselect_button();
    			return true;
    			
    		case MENU_CANCELOPERATION:
    			mFileMag.mHoldingFile = false;
    			mHoldingZip = false;
    			mDetailLabel.setText("");
    			mFileMag.moveOrCopyFile = false;
    			mFileMag.mAllowSelectAll = false;
    			mTable.killMultiSelect(true);
    			Toast.makeText(Main.this, getResources().getString(R.string.cancel_file_operation), Toast.LENGTH_SHORT).show();
    			return true;
    			
    		case MENU_PASTE:
    			//System.out.println("--->action bar file size " + mFileMag.mCopyOrMoveSize);
    			//System.out.println("--->action bar memory size " + mFileMag.getMemorySize());
    			// checkout the remaining space and select file size
				if(mFileMag.mCopyOrMoveSize > mFileMag.getMemorySize()) {
				//	Toast.makeText(this, "Lack of storage space", Toast.LENGTH_SHORT).show();
					Toast.makeText(this, getResources().getString(R.string.lack_storage_space), Toast.LENGTH_SHORT).show();
					System.out.println("ActExplorer Lack of storage space");
					Log.d("mydebug","--->action bar file size " + mFileMag.mCopyOrMoveSize);
    				Log.d("mydebug","--->action bar memory size " + mFileMag.getMemorySize());
					mFileMag.mHoldingFile = false;
	    			mDetailLabel.setText("");
	    			mFileMag.moveOrCopyFile = false;
	    			mTable.killMultiSelect(true);
					return true;
				}
				
    			String des = mFileMag.getCurrentDir();
    			paste(des);
    			mFileMag.mHoldingFile = false;
    			mDetailLabel.setText("");
    			mFileMag.moveOrCopyFile = false;
    			//mTable.killMultiSelect(true);
    			return true;
    			
    		case MENU_CREATE_FILE:
    			Intent txtIntent = new Intent();
    			txtIntent.setClass(this, CreateFile.class);
    			txtIntent.putExtra("type", "create");
    			txtIntent.putExtra("path", mFileMag.getCurrentDir());
    			startActivityForResult(txtIntent, 0);
    			return true;
    			
    		case MENU_BACK:
    			if(mFileMag.isRoot() && !mHandler.multi_select_flag){
    				return true;
    			} else {
    				this.onKeyDown(4, null);
    			}
    			return true;
    		case MENU_APPLICATION:
    			Intent mSettingsIntent = new Intent("android.settings.MANAGE_APPLICATIONS_SETTINGS");
    			startActivity(mSettingsIntent);
    			return true;
    		case MENU_SELECTALL:
    			mFileMag.moveOrCopyFile = true;
    			mHandler.multi_select_flag = true;
    			for(int i=0; i< mTable.getCount(); i++){
            		File file = new File(getCurrentFileName(i));
            		mTable.selectAllPosition(i, file.getPath());
            	}
    			return true;
    	}
    	return false;
    }
    
    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo info) {
    	super.onCreateContextMenu(menu, v, info);
    	
    	boolean multi_data = mHandler.hasMultiSelectData();
    	AdapterContextMenuInfo _info = (AdapterContextMenuInfo)info;
    	if(info == null)
    	{
    		return;
    	}
    	mSelectedListItem = mHandler.getData(_info.position);
    	
    	if(mHandler.getMode() != EventHandler.TREEVIEW_MODE)
    	{
    		return;
    	}
    	
    	/* is it a directory and is multi-select turned off */
    	if(mFileMag.isDirectory(mSelectedListItem) && !mHandler.isMultiSelected()) {
    		menu.setHeaderTitle(getResources().getString(R.string.Folder_operations));
        	menu.add(0, D_MENU_DELETE, 0, getResources().getString(R.string.Delete_Folder));
        	menu.add(0, D_MENU_RENAME, 0, getResources().getString(R.string.Rename_Folder));
        	menu.add(0, D_MENU_COPY, 0, getResources().getString(R.string.Copy_Folder));
        	menu.add(0, D_MENU_MOVE, 0, getResources().getString(R.string.Move_Folder));
        	menu.add(0, D_MENU_ZIP, 0, getResources().getString(R.string.Zip_Folder));
        	menu.add(0, D_MENU_PASTE, 0, getResources().getString(R.string.Paste_into_folder)).setEnabled(mFileMag.mHoldingFile || multi_data);
        	menu.add(0, D_MENU_UNZIP, 0, getResources().getString(R.string.Extract_here)).setEnabled(mHoldingZip);
        	menu.add(0, MENU_PROPERTY, 0, getResources().getString(R.string.property));
    		
        /* is it a file and is multi-select turned off */
    	} else if(!mFileMag.isDirectory(mSelectedListItem) && !mHandler.isMultiSelected()) {
        	menu.setHeaderTitle(getResources().getString(R.string.File_Operations));
    		menu.add(0, F_MENU_DELETE, 0, getResources().getString(R.string.Delete_File));
    		menu.add(0, F_MENU_RENAME, 0, getResources().getString(R.string.Rename_File));
    		menu.add(0, F_MENU_COPY, 0, getResources().getString(R.string.Copy_File));
    		menu.add(0, F_MENU_MOVE, 0, getResources().getString(R.string.Move_File));
    		menu.add(0, F_MENU_ATTACH, 0, getResources().getString(R.string.Email_File));
    	/*	if(getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH)){
    			menu.add(0, F_MENU_BLUETOOTH, 0, getResources().getString(R.string.Bluetooth_File));
    		}*/
    		
    	/*	try {  
	                Class<?> classType = Class.forName("android.os.SystemProperties");   
	                Method getMethod = classType.getDeclaredMethod("get", new Class<?>[]{String.class});  
	                String bluetoothInSetting = (String) getMethod.invoke(classType, new Object[]{"ro.settings.support.bluetooth"});  
	             //   Log.i(TAG, supportBT); 
	                if(bluetoothInSetting.equals("true"))
	                {
	                 //   Log.d("aaaa","support bluetooth");
	                    menu.add(0, F_MENU_BLUETOOTH, 0, getResources().getString(R.string.Bluetooth_File));
	                }
            	} 	catch (Exception e) {  
                //    Log.e(TAG, e.getMessage(),e);  
            }
            */
    		
    		String bluetoothInSetting = SystemProperties.get("ro.settings.support.bluetooth");
    		if(bluetoothInSetting.equals("true"))
            {
                menu.add(0, F_MENU_BLUETOOTH, 0, getResources().getString(R.string.Bluetooth_File));
            }

    		menu.add(0, MENU_PROPERTY, 0, getResources().getString(R.string.property));
    	}	
    }
    
    @Override
    public boolean onContextItemSelected(MenuItem item) {

    	switch(item.getItemId()) {
    		case D_MENU_DELETE:
    		case F_MENU_DELETE:
    			AlertDialog.Builder builder = new AlertDialog.Builder(this);
    			builder.setTitle(getResources().getString(R.string.Warning));
    			builder.setIcon(R.drawable.warning);
    			builder.setMessage(getResources().getString(R.string.Deleting) + mSelectedListItem +
    					getResources().getString(R.string.cannot_be_undone));
    			builder.setCancelable(false);
    			
    			builder.setNegativeButton(getResources().getString(R.string.Cancel), new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						dialog.dismiss();
					}
    			});
    			builder.setPositiveButton(getResources().getString(R.string.Delete), new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						String selectItem = mFileMag.getCurrentDir() + "/" + mSelectedListItem;
						if(mCopiedTarget != null && mCopiedTarget.equals(selectItem)){
							mCopiedTarget = null;
						}
						mHandler.deleteFile(selectItem);
					}
    			});
    			AlertDialog alert_d = builder.create();
    			alert_d.show();
    			return true;
    			
    		case D_MENU_RENAME:
    			showDialog_fun(D_MENU_RENAME);
    			return true;
    			
    		case F_MENU_RENAME:
    			showDialog_fun(F_MENU_RENAME);
    			return true;
    			
    		case F_MENU_ATTACH:
    			File file = new File(mFileMag.getCurrentDir() +"/"+ mSelectedListItem);
    			Intent mail_int = new Intent();
    			mail_int.setAction(android.content.Intent.ACTION_SEND);
    			mail_int.setType("application/mail");
    			mail_int.putExtra(Intent.EXTRA_BCC, "");
    			mail_int.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(file));
    			try{
    				startActivity(mail_int);
    			}
    			catch(ActivityNotFoundException e)
    			{
    				DisplayToast(getResources().getString(R.string.Activity_No_Found));
    				//Log.e(TAG,"activity no found");
    			}
    			return true;
    		
    		case F_MENU_BLUETOOTH:
    			File shareFile = new File(mFileMag.getCurrentDir() + "/" + mSelectedListItem);
    			Intent bluetooth = new Intent();
    			bluetooth.setAction(Intent.ACTION_SEND);
    			bluetooth.setType("*/*");
    			ComponentName component = new ComponentName("com.android.bluetooth", "com.android.bluetooth.opp.BluetoothOppLauncherActivity");
    			bluetooth.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(shareFile));
    			bluetooth.setComponent(component);
    			try{
    				startActivity(bluetooth);
    			}
    			catch(ActivityNotFoundException e)
    			{
    				DisplayToast(getResources().getString(R.string.Activity_No_Found));
    				//Log.e(TAG,"activity no found");
    			}
    			return true;
    		case F_MENU_MOVE:
    		case D_MENU_MOVE:
    		case F_MENU_COPY:
    		case D_MENU_COPY:
    			if(item.getItemId() == F_MENU_MOVE || item.getItemId() == D_MENU_MOVE){
    				mHandler.setDeleteAfterCopy(true);
    			}
				else 
					mHandler.setDeleteAfterCopy(false);
    			
    			mFileMag.mHoldingFile = true;
    			mFileMag.moveOrCopyFile = true;
    			
    			mCopiedTarget = mFileMag.getCurrentDir() +"/"+ mSelectedListItem;
    			mDetailLabel.setText(getResources().getString(R.string.Holding) + mSelectedListItem);

				Log.d("mydebug","copy mCopiedTarget:"+mCopiedTarget);
    			
    			// get the file size or folder size from select items
    			mFileMag.mCopyOrMoveSize = 0;
				mFileMag.resetDirSize();
				mFileMag.mCopyOrMoveSize += mFileMag.getDirOrFileSize(mCopiedTarget);
				
    			return true;
    			
    		
    		case D_MENU_PASTE:
    			//System.out.println("--->context item file size " + mFileMag.mCopyOrMoveSize);
    			//System.out.println("--->context item memory size " + mFileMag.getMemorySize());
    			// checkout the remaining space and select file size
				if(mFileMag.mCopyOrMoveSize > mFileMag.getMemorySize()) {
					Toast.makeText(this, getResources().getString(R.string.lack_storage_space), Toast.LENGTH_SHORT).show();
					System.out.println("ActExplorer Lack of storage space");
					Log.d("mydebug","--->context item file size " + mFileMag.mCopyOrMoveSize);
    				Log.d("mydebug","--->context item memory size " + mFileMag.getMemorySize());
					mFileMag.mHoldingFile = false;
	    			mFileMag.moveOrCopyFile = false;
	    			mDetailLabel.setText("");
					return true;
				}
    			boolean multi_select = mHandler.hasMultiSelectData();
    			
    			if(multi_select) {
    				mHandler.copyFileMultiSelect(mFileMag.getCurrentDir() +"/"+ mSelectedListItem);
    				
    			} else if(mFileMag.mHoldingFile && mCopiedTarget.length() > 1) {
    				mHandler.copyFile(mCopiedTarget, mFileMag.getCurrentDir() +"/"+ mSelectedListItem);
    				mDetailLabel.setText("");
    			}
    			mCopiedTarget = null;			   			
    			mFileMag.mHoldingFile = false;
    			mFileMag.moveOrCopyFile = false;
    			return true;
    			
    		case D_MENU_ZIP:
    			String dir = mFileMag.getCurrentDir();
    			mHandler.zipFile(dir + "/" + mSelectedListItem);
    			return true;
    			
    		case D_MENU_UNZIP:
    			if(mHoldingZip && mZippedTarget.length() > 1) {
    				String current_dir = mFileMag.getCurrentDir() + "/" + mSelectedListItem + "/";
    				String old_dir = mZippedTarget.substring(0, mZippedTarget.lastIndexOf("/"));
    				String name = mZippedTarget.substring(mZippedTarget.lastIndexOf("/") + 1, mZippedTarget.length());
    				if(new File(mZippedTarget).canRead() && new File(current_dir).canWrite()) {
    					mHandler.unZipFileToDir(name, current_dir, old_dir);				
	    				mPathLabel.setText(current_dir);
	    				
    				} else {
    					Toast.makeText(this, getResources().getString(R.string.no_permission) + name, 
    							Toast.LENGTH_SHORT).show();
    				}
    			}
    			
    			mHoldingZip = false;
    			mDetailLabel.setText("");
    			mZippedTarget = "";
    			return true;
    			
    		case MENU_PROPERTY:
    			showPropertyDialog();
    			return true;
    	}
    	return false;
    }
    
    public void showPropertyDialog(){
    	String mProperty = "";
    	File mFile = new File(mFileMag.getCurrentDir() + "/" + mSelectedListItem);
    	if(mFile.isDirectory()){
    		mProperty = "\n Name:   " + mSelectedListItem + "\n" 
    				+ "\n Path:   " + mFileMag.getCurrentDir() + "\n" 
    				+ "\n Size:   " + getFileSize(mFile) + " byte" + "\n" 
    				+ "\n Chmod:   " + mTable.getFilePermissions(mFile) + "\n";
    	} else if(mFile.isFile()){
    		mProperty = "\n Name:   " + mSelectedListItem + "\n" 
    				+ "\n Path:   " + mFileMag.getCurrentDir() + "\n" 
    				+ "\n Size:   " + mFile.length() + " byte" + "\n" 
    				+ "\n Chmod:   " + mTable.getFilePermissions(mFile) + "\n";
    	}
    	AlertDialog ad = new AlertDialog.Builder(this).create();
    	ad.setTitle(getResources().getString(R.string.file_property));
        ad.setMessage(mProperty);
        ad.setButton(getResources().getString(R.string.sure), new DialogInterface.OnClickListener() {
        	public void onClick(DialogInterface dialog, int which) {
            }});
        ad.setCancelable(false);
        ad.show();
    }
    
    public long getFileSize(File f)
    {
        long size = 0;
        File flist[] = f.listFiles();
        for (int i = 0; i < flist.length; i++){
            if (flist[i].isDirectory()){
                size = size + getFileSize(flist[i]);
            } else{
                size = size + flist[i].length();
            }
        }
        return size;
    }
    
    // 
    @Override
    protected void onPrepareDialog(int id, Dialog dialog) 
    {
    }
    
    public String getFileNameNoEx(String filename) {   
        if ((filename != null) && (filename.length() > 0)) {   
            int dot = 0;
            dot = filename.lastIndexOf('.');
            if(dot == 0) {
            	return filename;
            } else if ((dot >-1) && (dot < (filename.length()))) {   
                return filename.substring(0, dot);   
            }
        }
        return filename;   
    }
    
    public void showDialog_fun(int id) {
    	final Dialog dialog = new Dialog(Main.this);
    	
    	switch(id) {
    		case MENU_MKDIR:
    			dialog.setContentView(R.layout.input_layout);
    			dialog.setTitle(getResources().getString(R.string.Create_Directory));
    			dialog.setCancelable(false);
    			
    			ImageView icon = (ImageView)dialog.findViewById(R.id.input_icon);
    			icon.setImageResource(R.drawable.newfolder);
    			
    			TextView label = (TextView)dialog.findViewById(R.id.input_label);
    			label.setText(mFileMag.getCurrentDir());
    			final EditText input = (EditText)dialog.findViewById(R.id.input_inputText);
    			
    			Button cancel = (Button)dialog.findViewById(R.id.input_cancel_b);
    			Button create = (Button)dialog.findViewById(R.id.input_create_b);
    			
    			create.setOnClickListener(new OnClickListener() {
    				public void onClick (View v) {
    					if (input.getText().length() >= 1) {
    						if(input.getText().toString().substring(input.getText().toString().length()-1, input.getText().toString().length()).equals(" ")){
    							System.out.println("create file, but inputmethod input space");
    							input.setText(input.getText().toString().substring(0, input.getText().length()-1));
    						}
    						if (mFileMag.createDir(mFileMag.getCurrentDir() + "/", input.getText().toString()) == 0)
    							Toast.makeText(Main.this, 
    										   getResources().getString(R.string.Folder) + " " + input.getText().toString() + " " + getResources().getString(R.string.created), 
    										   Toast.LENGTH_LONG).show();
    						else
    							Toast.makeText(Main.this, getResources().getString(R.string.not_created), Toast.LENGTH_SHORT).show();
    					}
    					
    					dialog.dismiss();
    					String temp = mFileMag.getCurrentDir();
    					mHandler.updateDirectory(mFileMag.getNextDir(temp, true));
    				}
    			});
    			cancel.setOnClickListener(new OnClickListener() {
    				public void onClick (View v) {	dialog.dismiss(); }
    			});
    			break; 
    		case D_MENU_RENAME:
    		case F_MENU_RENAME:
    			dialog.setContentView(R.layout.input_layout);
    			dialog.setTitle(getResources().getString(R.string.Rename)/* + mSelectedListItem*/);
    			dialog.setCancelable(false);
    			
    			ImageView rename_icon = (ImageView)dialog.findViewById(R.id.input_icon);
    			rename_icon.setImageResource(R.drawable.rename);
    			
    			TextView rename_label = (TextView)dialog.findViewById(R.id.input_label);
    			rename_label.setText(mFileMag.getCurrentDir().substring(5, mFileMag.getCurrentDir().length()));
    			rename_label.setTextColor(Color.BLACK);
    			final EditText rename_input = (EditText)dialog.findViewById(R.id.input_inputText);
    			rename_input.setText(getFileNameNoEx(mSelectedListItem));
    			
    			Button rename_cancel = (Button)dialog.findViewById(R.id.input_cancel_b);
    			Button rename_create = (Button)dialog.findViewById(R.id.input_create_b);
    			rename_create.setText(getResources().getString(R.string.Rename));
    			
    			rename_create.setOnClickListener(new OnClickListener() {
    				public void onClick (View v) {
    					if(rename_input.getText().length() < 1)
    						dialog.dismiss();
    					
    					if(mFileMag.renameTarget(mFileMag.getCurrentDir() +"/"+ mSelectedListItem, rename_input.getText().toString()) == 0) {
    						Toast.makeText(Main.this, mSelectedListItem + getResources().getString(R.string.be_renamed) +rename_input.getText().toString(),
    								Toast.LENGTH_LONG).show();
    					}else
    						Toast.makeText(Main.this, mSelectedListItem + getResources().getString(R.string.not_renamed), Toast.LENGTH_LONG).show();
    					
    					dialog.dismiss();
    					String temp = mFileMag.getCurrentDir();
    					mHandler.updateDirectory(mFileMag.getNextDir(temp, true));
    				}
    			});
    			rename_cancel.setOnClickListener(new OnClickListener() {
    				public void onClick (View v) {	dialog.dismiss(); }
    			});
    			break;
    		
    		case SEARCH_B:
    		case MENU_SEARCH:
    			dialog.setContentView(R.layout.input_layout);
    			dialog.setTitle(getResources().getString(R.string.Search));
    			dialog.setCancelable(false);
    			
    			ImageView searchIcon = (ImageView)dialog.findViewById(R.id.input_icon);
    			searchIcon.setImageResource(R.drawable.search);
    			
    			TextView search_label = (TextView)dialog.findViewById(R.id.input_label);
    			search_label.setText(getResources().getString(R.string.Search_file));
    			final EditText search_input = (EditText)dialog.findViewById(R.id.input_inputText);
    			
    			Button search_button = (Button)dialog.findViewById(R.id.input_create_b);
    			Button cancel_button = (Button)dialog.findViewById(R.id.input_cancel_b);
    			search_button.setText(getResources().getString(R.string.Search));
    			
    			search_button.setOnClickListener(new OnClickListener() {
    				public void onClick(View v) {
    					String temp = search_input.getText().toString();
    					
    					if (temp.length() > 0)
    						mHandler.searchForFile(temp);
    					dialog.dismiss();
    				}
    			});
    			
    			cancel_button.setOnClickListener(new OnClickListener() {
    				public void onClick(View v) { dialog.dismiss(); }
    			});

    			break;
    	}
    	dialog.show();
    	//return dialog;
    }
    
    /*
     * (non-Javadoc)
     * This will check if the user is at root directory. If so, if they press back
     * again, it will close the application. 
     * @see android.app.Activity#onKeyDown(int, android.view.KeyEvent)
     */
    @Override
   public boolean onKeyDown(int keycode, KeyEvent event) { 
    	String current = mFileMag.getCurrentDir();
    	
    	if(keycode == KeyEvent.KEYCODE_SEARCH) {
    		showDialog_fun(SEARCH_B);
    		
    		return true;
    		
    	} else if(keycode == KeyEvent.KEYCODE_BACK && 
    			mHandler.getMode() == EventHandler.CATALOG_MODE) {
    		mFileMag.canMkdir = true;
    		mFileMag.moveOrCopyFile = false;
    		finish();
    		
    		return false;
    	} else if(keycode == KeyEvent.KEYCODE_BACK && mUseBackKey &&
    			!(mFileMag.isRoot()) && mHandler.multi_select_flag ) {
    		if(mHandler.isMultiSelected()) {
    			mTable.killMultiSelect(true);
    			mFileMag.moveOrCopyFile = false;
    			mFileMag.mAllowSelectAll = false;
    			Toast.makeText(Main.this, getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
    		}
    		//mHandler.updateDirectory(mFileMag.getPreviousDir());
    		//mPathLabel.setText(mFileMag.getCurrentDir());
    		
    		return true;
    	} else if(keycode == KeyEvent.KEYCODE_BACK && mUseBackKey &&
    			!(mFileMag.isRoot()) && !mHandler.multi_select_flag ) {
    		/*if(mHandler.isMultiSelected()) {
    			mTable.killMultiSelect(true);
    			Toast.makeText(Main.this, getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
    		}*/
    		
    		mHandler.updateDirectory(mFileMag.getPreviousDir());
    		//mPathLabel.setText(mFileMag.getCurrentDir());
    		mPathLabel.setText(mFileMag.getCurrentDir().substring(5, mFileMag.getCurrentDir().length()));

    		return true;
    		
    	} else if(keycode == KeyEvent.KEYCODE_BACK && mUseBackKey && 
    			mFileMag.isRoot() && !mHandler.multi_select_flag ) 
    	{
    		Toast.makeText(Main.this, getResources().getString(R.string.Press_back), Toast.LENGTH_SHORT).show();
    		mUseBackKey = false;
    		//mPathLabel.setText(mFileMag.getCurrentDir());
    		mPathLabel.setText(mFileMag.getCurrentDir().substring(5, mFileMag.getCurrentDir().length()));
    		
    		return false;
    		
    	} else if(keycode == KeyEvent.KEYCODE_BACK && !mUseBackKey && 
    			mFileMag.isRoot() ) 
    	{
    		mFileMag.moveOrCopyFile = false;
    		finish();
    		
    		return false;
    	} else if((keycode == KeyEvent.KEYCODE_BACK) && mHandler.multi_select_flag) {
    		if(mHandler.isMultiSelected()) {
    			mTable.killMultiSelect(true);
    			mFileMag.moveOrCopyFile = false;
    			mFileMag.mAllowSelectAll = false;
    			Toast.makeText(Main.this, getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
    		}
    		//mHandler.multiselect_button();
    		
    		return false;
    	}
    	return false;
    }

	@Override
	public void paste(String destination) {
		boolean multi_select = mHandler.hasMultiSelectData();
		
		if(multi_select) {
			mHandler.copyFileMultiSelect(destination);
			
		} else if(mFileMag.mHoldingFile) {
		//	if(!mCopiedTarget || mCopiedTarget.length() <2)

			mHandler.copyFile(mCopiedTarget, destination);
			mDetailLabel.setText("");
		}
		    			   			
		mFileMag.mHoldingFile = false;
	}
}
