
package com.actions.explore;

import java.io.File;
import java.util.ArrayList;
import com.actions.explore.R;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageParser;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.provider.MediaStore;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

/**
 * @author lizihao@actions-semi.com
 */
public class EventHandler implements OnClickListener{
	/*
	 * Unique types to control which file operation gets
	 * performed in the background
	 */
    private static final int SEARCH_TYPE =		0x00;
	private static final int COPY_TYPE =		0x01;
	private static final int UNZIP_TYPE =		0x02;
	private static final int UNZIPTO_TYPE =		0x03;
	private static final int ZIP_TYPE =			0x04;
	private static final int DELETE_TYPE = 		0x05;
	public static final int MANAGE_DIALOG =	 0x06;
	
	public static final int TREEVIEW_MODE = 1;
	public static final int CATALOG_MODE = 2;
	
	private int	mlistmode = TREEVIEW_MODE;
	
	private final Context mContext;
	private final FileOperateCallbacks mCallbacks;
	private final FileManager mFileMang;
	private final CatalogList mCataList;
	private TableRow mDelegate;
	public boolean multi_select_flag = false;
	private boolean delete_after_copy = false;
	
	private boolean thumbnail_flag = true;
	//private int mColor = Color.WHITE;
	private int mColor = Color.BLACK;
	
	//the list used to feed info into the array adapter and when multi-select is on
	private ArrayList<String> mDataSource, mMultiSelectData;
	private TextView mPathLabel;
	private TextView mInfoLabel;
	
	private View preView;
	
	public EventHandler(Context context, FileOperateCallbacks callbacks, final FileManager manager,final CatalogList CataList) {
		mContext = context;
		mFileMang = manager;
		mCataList = CataList;
		mCallbacks = callbacks;
		
		mDataSource = new ArrayList<String>(mFileMang.getHomeDir(FileManager.ROOT_FLASH));
	}

	public void setListAdapter(TableRow adapter) {
		mDelegate = adapter;
	}
	
	/**
	 * This method is called from the Main activity and this has the same
	 * reference to the same object so when changes are made here or there
	 * they will display in the same way.
	 * 
	 * @param adapter	The TableRow object
	 */
	public int getMode() {
		return mlistmode;
	}
	/**
	 * This method is called from the Main activity and is passed
	 * the TextView that should be updated as the directory changes
	 * so the user knows which folder they are in.
	 * 
	 * @param path	The label to update as the directory changes
	 * @param label	the label to update information
	 */
	public void setUpdateLabels(TextView path, TextView label) {
		mPathLabel = path;
		mInfoLabel = label;
	}
	
	/**
	 * 
	 * @param color
	 */
	public void setTextColor(int color) {
		mColor = color;
	}
	
	/**
	 * Set this true and thumbnails will be used as the icon for image files. False will
	 * show a default image. 
	 * 
	 * @param show
	 */
	public void setShowThumbnails(boolean show) {
		thumbnail_flag = show;
	}
	
	/**
	 * If you want to move a file (cut/paste) and not just copy/paste use this method to 
	 * tell the file manager to delete the old reference of the file.
	 * 
	 * @param delete true if you want to move a file, false to copy the file
	 */
	public void setDeleteAfterCopy(boolean delete) {
		delete_after_copy = delete;
	}
	
	/**
	 * Indicates whether the user wants to select 
	 * multiple files or folders at a time.
	 * <br><br>
	 * false by default
	 * 
	 * @return	true if the user has turned on multi selection
	 */
	public boolean isMultiSelected() {
		return multi_select_flag;
	}
	
	/**
	 * Use this method to determine if the user has selected multiple files/folders
	 * 
	 * @return	returns true if the user is holding multiple objects (multi-select)
	 */
	public boolean hasMultiSelectData() {
		return (mMultiSelectData != null && mMultiSelectData.size() > 0);
	}
	
	/**
	 * Will search for a file then display all files with the 
	 * search parameter in its name
	 * 
	 * @param name	the name to search for
	 */
	public void searchForFile(String name) {
		new BackgroundWork(SEARCH_TYPE).execute(name);
	}
	
	/**
	 * Will delete the file name that is passed on a background
	 * thread.
	 * 
	 * @param name
	 */
	public void deleteFile(String name) {
		new BackgroundWork(DELETE_TYPE).execute(name);
	}
	
	/**
	 * Will copy a file or folder to another location.
	 * 
	 * @param oldLocation	from location
	 * @param newLocation	to location
	 */
	public void copyFile(String oldLocation, String newLocation) {
		String[] data = {oldLocation, newLocation};
		
		new BackgroundWork(COPY_TYPE).execute(data);
	}
	
	/**
	 * 
	 * @param newLocation
	 */
	public void copyFileMultiSelect(String newLocation) {
		String[] data;
		int index = 1;
		if (mMultiSelectData.size() > 0) {
			data = new String[mMultiSelectData.size() + 1];
			data[0] = newLocation;
			
			for(String s : mMultiSelectData)
				data[index++] = s;
			
			new BackgroundWork(COPY_TYPE).execute(data);
		}
	}
	
	/**
	 * This will extract a zip file to the same directory.
	 * 
	 * @param file	the zip file name
	 * @param path	the path were the zip file will be extracted (the current directory)
	 */
	public void unZipFile(String file, String path) {
		new BackgroundWork(UNZIP_TYPE).execute(file, path);
	}
	
	/**
	 * This method will take a zip file and extract it to another
	 * location
	 *  
	 * @param name		the name of the of the new file (the dir name is used)
	 * @param newDir	the dir where to extract to
	 * @param oldDir	the dir where the zip file is
	 */
	public void unZipFileToDir(String name, String newDir, String oldDir) {
		new BackgroundWork(UNZIPTO_TYPE).execute(name, newDir, oldDir);
	}
	
	/**
	 * Creates a zip file
	 * 
	 * @param zipPath	the path to the directory you want to zip
	 */
	public void zipFile(String zipPath) {
		new BackgroundWork(ZIP_TYPE).execute(zipPath);
	}

	/**
	 *  This method, handles the button presses of the top buttons found
	 *  in the Main activity. 
	 */
	@Override
	public void onClick(View v) {
		
		switch(v.getId()) {
			case R.id.home_sdcard_button:
				mFileMang.canMkdir = true;
				refreshFocus(preView,v);
				if(mFileMang.whichRoot() == FileManager.ROOT_SDCARD &&
					mlistmode == TREEVIEW_MODE)
				{
					break;
				}
				mlistmode = TREEVIEW_MODE;
				if(multi_select_flag) {
					mDelegate.killMultiSelect(true);
					mFileMang.mHoldingFile = false;
					mFileMang.mAllowSelectAll = false;
					mInfoLabel.setText("");
	    			mFileMang.moveOrCopyFile = false;
	    			Toast.makeText(mContext, mContext.getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
				}
				updateDirectory(mFileMang.getHomeDir(FileManager.ROOT_SDCARD));
				if(mPathLabel != null)
					mPathLabel.setText(mFileMang.getCurrentDir().substring(5, mFileMang.getCurrentDir().length()));
				break;
				
			case R.id.home_usbhost_button:
				mFileMang.canMkdir = true;
				refreshFocus(preView,v);
				if(mFileMang.whichRoot() == FileManager.ROOT_USBHOST &&
					mlistmode == TREEVIEW_MODE)
				{
					break;
				}
				mlistmode = TREEVIEW_MODE;
				if(multi_select_flag) {
					mDelegate.killMultiSelect(true);
					mFileMang.mHoldingFile = false;
					mFileMang.mAllowSelectAll = false;
					mInfoLabel.setText("");
	    			mFileMang.moveOrCopyFile = false;
	    			Toast.makeText(mContext, mContext.getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
				}
				updateDirectory(mFileMang.getHomeDir(FileManager.ROOT_USBHOST));
				if(mPathLabel != null)
					mPathLabel.setText(mFileMang.getCurrentDir().substring(5, mFileMang.getCurrentDir().length()));
				break;
				
			case R.id.home_flash_button:
				mFileMang.canMkdir = true;
				refreshFocus(preView,v);
				if(mFileMang.whichRoot() == FileManager.ROOT_FLASH &&
					mlistmode == TREEVIEW_MODE)
				{
					break;
				}
				mlistmode = TREEVIEW_MODE;
				if(multi_select_flag) {
					mDelegate.killMultiSelect(true);
					mFileMang.mHoldingFile = false;
					mFileMang.mAllowSelectAll = false;
					mInfoLabel.setText("");
	    			mFileMang.moveOrCopyFile = false;
	    			Toast.makeText(mContext, mContext.getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
				}
				updateDirectory(mFileMang.getHomeDir(FileManager.ROOT_FLASH));
				if(mPathLabel != null)
					mPathLabel.setText(mFileMang.getCurrentDir().substring(5, mFileMang.getCurrentDir().length()));
				break;
				
			case R.id.music_button:
				mFileMang.canMkdir = false;
				mlistmode = CATALOG_MODE;
				if(multi_select_flag) {
					mDelegate.killMultiSelect(true);
					mFileMang.mHoldingFile = false;
					mFileMang.mAllowSelectAll = false;
					mInfoLabel.setText("");
	    			mFileMang.moveOrCopyFile = false;
	    			Toast.makeText(mContext, mContext.getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
				}
				setFileList(mCataList.SetFileTyp(CatalogList.TYPE_MUSIC));
				if(mPathLabel != null)
					mPathLabel.setText(mContext.getResources().getString(R.string.music));
				refreshFocus(preView,v);
				break;
				
			case R.id.image_button:
				mFileMang.canMkdir = false;
				mlistmode = CATALOG_MODE;
				if(multi_select_flag) {
					mDelegate.killMultiSelect(true);
					mFileMang.mHoldingFile = false;
					mFileMang.mAllowSelectAll = false;
					mInfoLabel.setText("");
	    			mFileMang.moveOrCopyFile = false;
	    			Toast.makeText(mContext, mContext.getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
				}
				setFileList(mCataList.SetFileTyp(CatalogList.TYPE_PICTURE));
				if(mPathLabel != null)
					mPathLabel.setText(mContext.getResources().getString(R.string.picture));
				refreshFocus(preView,v);
				break;
				
			case R.id.movie_button:
				mFileMang.canMkdir = false;
				mlistmode = CATALOG_MODE;
				if(multi_select_flag) {
					mDelegate.killMultiSelect(true);
					mFileMang.mHoldingFile = false;
					mFileMang.mAllowSelectAll = false;
					mInfoLabel.setText("");
	    			mFileMang.moveOrCopyFile = false;
	    			Toast.makeText(mContext, mContext.getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
				}
				setFileList(mCataList.SetFileTyp(CatalogList.TYPE_MOVIE));
				if(mPathLabel != null)
					mPathLabel.setText(mContext.getResources().getString(R.string.movie));
				refreshFocus(preView,v);
				break;
				
			case R.id.ebook_button:
				mFileMang.canMkdir = false;
				mlistmode = CATALOG_MODE;
				if(multi_select_flag) {
					mDelegate.killMultiSelect(true);
					mFileMang.mHoldingFile = false;
					mFileMang.mAllowSelectAll = false;
					mInfoLabel.setText("");
					mFileMang.moveOrCopyFile = false;
					Toast.makeText(mContext, mContext.getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
				}
				setFileList(mCataList.SetFileTyp(CatalogList.TYPE_EBOOK));
				if(mPathLabel != null)
					mPathLabel.setText(mContext.getResources().getString(R.string.ebook));
				refreshFocus(preView,v);
				break;
				
			case R.id.apk_button:
				mFileMang.canMkdir = false;
				mlistmode = CATALOG_MODE;
				if(multi_select_flag) {
					mDelegate.killMultiSelect(true);
					mFileMang.mHoldingFile = false;
					mFileMang.mAllowSelectAll = false;
					mInfoLabel.setText("");
					mFileMang.moveOrCopyFile = false;
					Toast.makeText(mContext, mContext.getResources().getString(R.string.Multi_select_off), Toast.LENGTH_SHORT).show();
				}
				setFileList(mCataList.SetFileTyp(CatalogList.TYPE_APK));
				if(mPathLabel != null)
					mPathLabel.setText(mContext.getResources().getString(R.string.apk));
				refreshFocus(preView,v);
				break;
				
			case R.id.hidden_move:
			case R.id.hidden_copy:
				if(mMultiSelectData == null || mMultiSelectData.isEmpty()) {
					mDelegate.killMultiSelect(true);
					mFileMang.moveOrCopyFile = false;
					mFileMang.mAllowSelectAll = false;
					Toast.makeText(mContext, mContext.getResources().getString(R.string.no_select), 
							Toast.LENGTH_SHORT).show();
					break;
				}
				
				mFileMang.mCopyOrMoveSize = 0;
				for(String s : mMultiSelectData){
					System.out.println("--->" + s);
					mFileMang.mCopyOrMoveSize += mFileMang.getDirOrFileSize(s);
				}
				
				if(v.getId() == R.id.hidden_move)
					delete_after_copy = true;
					
				mInfoLabel.setText(mContext.getResources().getString(R.string.Holding) + " " + mMultiSelectData.size() + 
								   " " + mContext.getResources().getString(R.string.files));
				mFileMang.mHoldingFile = true;
				mFileMang.mAllowSelectAll = false;
				
				mDelegate.killMultiSelect(false);
				break;
				
			case R.id.hidden_delete:	
				/* check if user selected objects before going further */
				if(!mFileMang.moveOrCopyFile) {
					mMultiSelectData = null;
				}
				if(mMultiSelectData == null || mMultiSelectData.isEmpty()) {
					mDelegate.killMultiSelect(true);
					mFileMang.moveOrCopyFile = false;
					mFileMang.mAllowSelectAll = false;
					Toast.makeText(mContext, mContext.getResources().getString(R.string.no_select), 
							Toast.LENGTH_SHORT).show();
					break;
				}

				final String[] data = new String[mMultiSelectData.size()];
				int at = 0;
				
				for(String string : mMultiSelectData)
					data[at++] = string;
				
				AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
				builder.setMessage(mContext.getResources().getString(R.string.delete_msg1) + " " +
								    data.length + " " + mContext.getResources().getString(R.string.delete_msg2));
				builder.setCancelable(false);
				builder.setPositiveButton(mContext.getResources().getString(R.string.Delete), new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						new BackgroundWork(DELETE_TYPE).execute(data);
						mDelegate.killMultiSelect(true);
						mFileMang.moveOrCopyFile = false;
						mFileMang.mAllowSelectAll = false;
					}
				});
				builder.setNegativeButton(mContext.getResources().getString(R.string.Cancel), new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int which) {
						mDelegate.killMultiSelect(true);
						mFileMang.moveOrCopyFile = false;
						mFileMang.mAllowSelectAll = false;
						dialog.cancel();
					}
				});
				
				builder.create().show();
				break;
		}
	}
	
	public void multiselect_button(){
		if(getMode() != TREEVIEW_MODE)
		{
			return;
		}
		
		if(multi_select_flag) {
			mDelegate.killMultiSelect(true);				
			
		} else {
			LinearLayout hidden_lay = 
				(LinearLayout)((Activity) mContext).findViewById(R.id.hidden_buttons);
			
			multi_select_flag = true;
			hidden_lay.setVisibility(LinearLayout.VISIBLE);
		}
	}
	
	public void getInitView(View v){
		preView = v;
	}
	
	private void refreshFocus(View pre,View cur) {
		if( pre != cur) {
			cur.setSelected(true);
			pre.setSelected(false);
			cur.setBackground(mContext.getResources().getDrawable(R.drawable.selected_bg));
			pre.setBackgroundColor(0xffe8e8e8);
			preView = cur;
		}
	}
	
	/**
	 * will return the data in the ArrayList that holds the dir contents. 
	 * 
	 * @param position	the indext of the arraylist holding the dir content
	 * @return the data in the arraylist at position (position)
	 */
	public String getData(int position) {
		
		if(position > mDataSource.size() - 1 || position < 0)
			return null;
		
		return mDataSource.get(position);
	}

	/**
	 * called to update the file contents as the user navigates there
	 * phones file system. 
	 * 
	 * @param content	an ArrayList of the file/folders in the current directory.
	 */
	public void updateDirectory(ArrayList<String> content) {	
		if(!mDataSource.isEmpty())
			mDataSource.clear();
		
		for(String data : content)
			mDataSource.add(data);
		
		mDelegate.notifyDataSetChanged();
	}

	/**
	 * called to refresh the file list
	 * 
	 * @param content	an ArrayList of the file/folders in the current directory.
	 */
	public void setFileList(ArrayList<String> content) {
		if(mDataSource.equals(content))
		{
			return;
		}
		if(!mDataSource.isEmpty())
			mDataSource.clear();
		
		mDataSource.addAll(content);
		
		/*
		 * File list have been change,so clear the thumbnail
		 */
		mDelegate.clearThumbnail();
		mDelegate.notifyDataSetChanged();
	}
	
	private static class ViewHolder {
		TextView topView;
		TextView bottomView;
		ImageView icon;
		ImageView mSelect;	//multi-select check mark icon
	}

	
	/**
	 * A nested class to handle displaying a custom view in the ListView that
	 * is used in the Main activity. If any icons are to be added, they must
	 * be implemented in the getView method. This class is instantiated once in Main
	 * and has no reason to be instantiated again. 
	 * 
	 */
    public class TableRow extends ArrayAdapter<String> {
    	private final int KB = 1024;
    	private final int MG = KB * KB;
    	private final int GB = MG * KB;
    	private String display_size;
    	private String dir_name;
    	private ArrayList<Integer> positions;
    	private LinearLayout hidden_layout;
    	private ThumbnailCreator thumbnail;
    	
    	public TableRow() {
    		super(mContext, R.layout.tablerow, mDataSource);
    		
    		thumbnail = new ThumbnailCreator(48, 48);
    		dir_name = new DevicePath(mContext).getInterStoragePath();
    	}
    	
    	public void addMultiPosition(int index, String path) {
    		
    		if(positions == null)
    			positions = new ArrayList<Integer>();
    		
    		if(mMultiSelectData == null) {
    			positions.add(index);
    			add_multiSelect_file(path);
    			
    		} else if(mMultiSelectData.contains(path)) {
    			if(positions.contains(index)) {
    				positions.remove(new Integer(index));
    				mMultiSelectData.remove(path);
    			}
    			
    		} else {
    			positions.add(index);
    			add_multiSelect_file(path);
    		}
    		
    		notifyDataSetChanged();
    	}
    	
    	public void selectAllPosition(int index, String path) {
    		if(positions == null)
    			positions = new ArrayList<Integer>();
    		
    		if(mMultiSelectData == null) {
    			positions.add(index);
    			add_multiSelect_file(path);
    			
    		} else if(mMultiSelectData.contains(path)) {
    			if(positions.contains(index) && !mFileMang.mAllowSelectAll){
    				positions.remove(new Integer(index));
	    			mMultiSelectData.remove(path);
    			}
    			
    		} else {
    			positions.add(index);
    			add_multiSelect_file(path);
    		}
    		
    		notifyDataSetChanged();
    	}
   	
    	/**
    	 * This will turn off multi-select and hide the multi-select buttons at the
    	 * bottom of the view. 
    	 * 
    	 * @param clearData if this is true any files/folders the user selected for multi-select
    	 * 					will be cleared. If false, the data will be kept for later use. Note:
    	 * 					multi-select copy and move will usually be the only one to pass false, 
    	 * 					so we can later paste it to another folder.
    	 */
    	public void killMultiSelect(boolean clearData) {
    		hidden_layout = (LinearLayout)((Activity)mContext).findViewById(R.id.hidden_buttons);
    		hidden_layout.setVisibility(LinearLayout.GONE);
    		multi_select_flag = false;
    		
    		if(positions != null && !positions.isEmpty())
    			positions.clear();
    		
    		if(clearData)
    			if(mMultiSelectData != null && !mMultiSelectData.isEmpty())
    				mMultiSelectData.clear();
    		
    		notifyDataSetChanged();
    	}
    	
    	public String getFilePermissions(File file) {
    		String per = "-";
    	    		
    		if(file.isDirectory())
    			per += "d";
    		if(file.canRead())
    			per += "r";
    		if(file.canWrite())
    			per += "w";
    		
    		return per;
    	}
    	
    	public void clearThumbnail() {
    		if(thumbnail_flag) {
    			thumbnail.clearBitmapCache();
    		}
    	}
    	@Override
    	public View getView(int position, View convertView, ViewGroup parent){
    		if(mlistmode == CATALOG_MODE)
    		{
    			return getView_catalog(position,convertView,parent);
    		}
    		else if (mlistmode == TREEVIEW_MODE)
    		{
        		return getView_tree(position,convertView,parent);
    		}
    		
    		return getView_tree(position,convertView,parent);
    	}
    	
    	private View getView_catalog(int position, View convertView, ViewGroup parent){
    		ViewHolder holder;
    		File file = new File(mDataSource.get(position));
    		
    		if(convertView == null) {
    			LayoutInflater inflater = (LayoutInflater) mContext.
    						getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    			convertView = inflater.inflate(R.layout.tablerow, parent, false);
    			
    			holder = new ViewHolder();
    			holder.topView = (TextView)convertView.findViewById(R.id.top_view);
    			holder.bottomView = (TextView)convertView.findViewById(R.id.bottom_view);
    			holder.icon = (ImageView)convertView.findViewById(R.id.row_image);
    			holder.mSelect = (ImageView)convertView.findViewById(R.id.multiselect_icon);
    			
    			convertView.setTag(holder);
    		} else {
    			holder = (ViewHolder)convertView.getTag();
    		}
    				
    		  		
    		if (positions != null && positions.contains(position))
    			holder.mSelect.setVisibility(ImageView.VISIBLE);
    		else
    			holder.mSelect.setVisibility(ImageView.GONE);
    		
    		holder.topView.setTextColor(mColor);
    		holder.bottomView.setTextColor(mColor);
    		
    		if(file != null && file.isFile()) {
    			String ext = file.toString();
    			String sub_ext = ext.substring(ext.lastIndexOf(".") + 1);
    			
    			/* This series of else if statements will determine which 
    			 * icon is displayed 
    			 */
    			if (TypeFilter.getInstance().isPdfFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.pdf);
    			
    			} else if (TypeFilter.getInstance().isMusicFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.music);
    			
    			} else if (TypeFilter.getInstance().isPictureFile(sub_ext)) {
    				if(thumbnail_flag && file.length() != 0) {
	    				Bitmap thumb = thumbnail.hasBitmapCached(position);
	    				if(thumb == null) {

	    					holder.icon.setImageResource(R.drawable.image);
	    					final Handler mHandler = new Handler();
	    					boolean isJPG = false;
	    					if(sub_ext.equalsIgnoreCase("jpeg") || sub_ext.equalsIgnoreCase("jpg")){
	    						isJPG = true;
	    					}
	   						thumbnail.setBitmapToImageView(file.getPath(), 
	   													   mHandler, 
	   													   holder.icon,
	   													   isJPG,
	   													   position);
	   						
	    				} else {
	    					holder.icon.setImageBitmap(thumb);
	    				}
	    				
    				} else {
    					holder.icon.setImageResource(R.drawable.image);
    				}
    				
    			} else if(TypeFilter.getInstance().isMovieFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.movies);
    			
    			} else if(TypeFilter.getInstance().isTxtFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.text);
    			} else if(TypeFilter.getInstance().isWordFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.word);
    			} else if(TypeFilter.getInstance().isExcelFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.excel);
    			} else if(TypeFilter.getInstance().isPptFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.ppt);
    			} else if(TypeFilter.getInstance().isApkFile(sub_ext)) {
    				holder.icon.setImageDrawable(getApkIcon(ext, mContext));
    			} else {
    				holder.icon.setImageResource(R.drawable.text);
    			}	
    		}
    		
    		String permission = getFilePermissions(file);
    		
    		if(file.isFile()) {
    			double size = file.length();
        		if (size > GB)
    				display_size = String.format("%.2f Gb ", (double)size / GB);
    			else if (size < GB && size > MG)
    				display_size = String.format("%.2f Mb ", (double)size / MG);
    			else if (size < MG && size > KB)
    				display_size = String.format("%.2f Kb ", (double)size/ KB);
    			else
    				display_size = String.format("%.2f bytes ", (double)size);
        		
        		if(file.isHidden())
        			holder.bottomView.setText("(hidden) | " + display_size +" | "+ permission);
        		else
        			holder.bottomView.setText(display_size +" | "+ permission);
        		
    		}
    		
    		holder.topView.setText(file.getName());
    		
    		return convertView;
    	}
    	
    	private View getView_tree(int position, View convertView, ViewGroup parent) {
    		ViewHolder holder;
    		int num_items = 0;
    		String temp = mFileMang.getCurrentDir();
    		File file = new File(temp + "/" + mDataSource.get(position));
    		String[] list = file.list();
    		
    		if(list != null)
    			num_items = list.length;
   
    		if(convertView == null) {
    			LayoutInflater inflater = (LayoutInflater) mContext.
    						getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    			convertView = inflater.inflate(R.layout.tablerow, parent, false);
    			
    			holder = new ViewHolder();
    			holder.topView = (TextView)convertView.findViewById(R.id.top_view);
    			holder.bottomView = (TextView)convertView.findViewById(R.id.bottom_view);
    			holder.icon = (ImageView)convertView.findViewById(R.id.row_image);
    			holder.mSelect = (ImageView)convertView.findViewById(R.id.multiselect_icon);
    			
    			convertView.setTag(holder);
    		} else {
    			holder = (ViewHolder)convertView.getTag();
    		}
    		
    		/* This will check if the thumbnail cache needs to be cleared by checking
    		 * if the user has changed directories. This way the cache wont show
    		 * a wrong thumbnail image for the new image file 
    		 */
    		if(!dir_name.equals(temp) && thumbnail_flag) {
    			thumbnail.clearBitmapCache();
    			dir_name = temp;
    		}
    		  		
    		if (positions != null && positions.contains(position))
    			holder.mSelect.setVisibility(ImageView.VISIBLE);
    		else
    			holder.mSelect.setVisibility(ImageView.GONE);

    		holder.topView.setTextColor(mColor);
    		holder.bottomView.setTextColor(mColor);
    		
    		if(file != null && file.isFile()) {
    			String ext = file.toString();
    			String sub_ext = ext.substring(ext.lastIndexOf(".") + 1);
    			
    			/* This series of else if statements will determine which 
    			 * icon is displayed 
    			 */
    			if (TypeFilter.getInstance().isPdfFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.pdf);
    			
    			} else if (TypeFilter.getInstance().isMusicFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.music);
    			
    			} else if (TypeFilter.getInstance().isPictureFile(sub_ext)) {
    				if(thumbnail_flag && file.length() != 0) {
	    				Bitmap thumb = thumbnail.hasBitmapCached(position);
	    				boolean isJPG = false;
						if(sub_ext.equalsIgnoreCase("jpeg") || sub_ext.equalsIgnoreCase("jpg")){
							isJPG = true;
						}
	    				if(thumb == null) {
	    					holder.icon.setImageResource(R.drawable.image);
	    					final Handler mHandler = new Handler();
	   						thumbnail.setBitmapToImageView(file.getPath(), 
	   													   mHandler, 
	   													   holder.icon,
	   													   isJPG,
	   													   position);
	   						
	    				} else {
	    					holder.icon.setImageBitmap(thumb);
	    				}
	    				
    				} else {
    					holder.icon.setImageResource(R.drawable.image);
    				}
    				
    			} else if (TypeFilter.getInstance().isZipFile(sub_ext) ||
    					TypeFilter.getInstance().isGZipFile(sub_ext) || 
    					TypeFilter.getInstance().isRarFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.zip);
    			
    			} else if(TypeFilter.getInstance().isMovieFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.movies);
    			
    			} else if(TypeFilter.getInstance().isWordFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.word);
    			
    			} else if(TypeFilter.getInstance().isExcelFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.excel);
    				
    			} else if(TypeFilter.getInstance().isPptFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.ppt);   	
    				
    			} else if(TypeFilter.getInstance().isHtml32File(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.html32);  
    				
    			} else if(TypeFilter.getInstance().isXml32File(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.xml32);
    				
    			} else if(TypeFilter.getInstance().isConfig32File(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.config32);
    				
    			} else if(TypeFilter.getInstance().isApkFile(sub_ext)) {
    				holder.icon.setImageDrawable(getApkIcon(ext, mContext));
    				
    			} else if(TypeFilter.getInstance().isJarFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.jar32);
    				
    			} else if(TypeFilter.getInstance().isTxtFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.text);
    			} else if(TypeFilter.getInstance().isPadFile(sub_ext)) {
    				holder.icon.setImageResource(R.drawable.text);
    			} else {
    				holder.icon.setImageResource(R.drawable.config32);
    			}
    			
    		} else if (file != null && file.isDirectory()) {
    			holder.icon.setImageResource(R.drawable.folder);
    		} else{
				holder.icon.setImageResource(R.drawable.text);
			}
    		String permission = getFilePermissions(file);
    		
    		if(file.isFile()) {
    			double size = file.length();
        		if (size > GB)
    				display_size = String.format("%.2f Gb ", (double)size / GB);
    			else if (size < GB && size > MG)
    				display_size = String.format("%.2f Mb ", (double)size / MG);
    			else if (size < MG && size > KB)
    				display_size = String.format("%.2f Kb ", (double)size/ KB);
    			else
    				display_size = String.format("%.2f bytes ", (double)size);
        		
        		if(file.isHidden())
        			holder.bottomView.setText("(hidden) | " + display_size +" | "+ permission);
        		else
        			holder.bottomView.setText(display_size +" | "+ permission);
        		
    		} else {
    			if(file.isHidden())
    				holder.bottomView.setText("(hidden) | " + num_items + " items | " + permission);
    			else
    				holder.bottomView.setText(num_items + " items | " + permission);
    		}
    		
    		holder.topView.setText(file.getName());
    		
    		return convertView;
    	}
    	
    	private void add_multiSelect_file(String src) {
    		if(mMultiSelectData == null)
    			mMultiSelectData = new ArrayList<String>();
    		
    		mMultiSelectData.add(src);
    	}
    }
    
    public static Drawable getApkIcon(String path, Context mContext) {
        Drawable icon = null;    
    	PackageManager pm = mContext.getPackageManager();
	    PackageInfo pinfo = pm.getPackageArchiveInfo(path, PackageManager.GET_ACTIVITIES);
	    if (pinfo != null) {
            pinfo.applicationInfo.publicSourceDir = path;
			icon = pm.getApplicationIcon(pinfo.applicationInfo);
        }

        return icon;
    }
/*    
    public static Drawable getApkIcon(String path, Context mContext) {
        Drawable icon = null;
        try {
            String apkPath = path;
            PackageParser packageParser = new PackageParser(apkPath);
            DisplayMetrics metrics = new DisplayMetrics();
            metrics.setToDefaults();
            PackageParser.Package mPkgInfo = packageParser.parsePackage(
                    new File(apkPath), apkPath, metrics, 0);
            ApplicationInfo info = mPkgInfo.applicationInfo;
            Resources pRes = mContext.getResources();
            AssetManager assmgr = new AssetManager();
            assmgr.addAssetPath(apkPath);
            Resources res = new Resources(assmgr, pRes.getDisplayMetrics(),
                    pRes.getConfiguration());
            if (info.icon != 0) {
                icon = res.getDrawable(info.icon);
            }
            packageParser = null;
            metrics = null;
            mPkgInfo = null;
            info = null;
            pRes = null;
            assmgr = null;
            res = null;
        } catch (Exception e) {
            icon = null;
        }
        return icon;
    }
*/    
    /**
     * A private inner class of EventHandler used to perform time extensive 
     * operations. So the user does not think the the application has hung, 
     * operations such as copy/past, search, unzip and zip will all be performed 
     * in the background. This class extends AsyncTask in order to give the user
     * a progress dialog to show that the app is working properly.
     * 
     * (note): this class will eventually be changed from using AsyncTask to using
     * Handlers and messages to perform background operations. 
     * 
     * @author Joe Berria
     */
    private class BackgroundWork extends AsyncTask<String, Void, ArrayList<String>> {
    	private String file_name;
    	private ProgressDialog pr_dialog;
    	private int type;
    	private int copy_rtn;
    	private static final String WAKE_LOCK = "wakelock";
    	private WakeLock wl = null;
    	
    	private BackgroundWork(int type) {
    	//	Log.d("mydebug","newbgtask");
    		this.type = type;
    	}
    	
    	/**
    	 * This is done on the EDT thread. this is called before 
    	 * doInBackground is called
    	 */
    	@Override
    	protected void onPreExecute() {
    		PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
    		wl = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, WAKE_LOCK);
    		wl.acquire();
    		
    		switch(type) {
    			case SEARCH_TYPE:
    				pr_dialog = ProgressDialog.show(mContext, mContext.getResources().getString(R.string.search_title), 
    						mContext.getResources().getString(R.string.search_msg),
    												true, true);
    				break;
    				
    			case COPY_TYPE:
    				pr_dialog = ProgressDialog.show(mContext, mContext.getResources().getString(R.string.copy_title), 
    						mContext.getResources().getString(R.string.copy_msg), 
    												true, false);
    				break;
    				
    			case UNZIP_TYPE:
    				pr_dialog = ProgressDialog.show(mContext, mContext.getResources().getString(R.string.unzip_title), 
    						mContext.getResources().getString(R.string.unzip_msg),
    												true, false);
    				break;
    				
    			case UNZIPTO_TYPE:
    				pr_dialog = ProgressDialog.show(mContext, mContext.getResources().getString(R.string.unzip_title), 
    						mContext.getResources().getString(R.string.unzip_msg),
    												true, false);
    				break;
    			
    			case ZIP_TYPE:
    				pr_dialog = ProgressDialog.show(mContext, mContext.getResources().getString(R.string.zip_title), 
    						mContext.getResources().getString(R.string.zip_msg), 
    												true, false);
    				break;
    				
    			case DELETE_TYPE:
    				pr_dialog = ProgressDialog.show(mContext, mContext.getResources().getString(R.string.delete_title), 
    						mContext.getResources().getString(R.string.delete_msg), 
    												true, false);
    				break;
    		}
    	}

    	/**
    	 * background thread here
    	 */
    	@Override
		protected ArrayList<String> doInBackground(String... params) {
			
			switch(type) {
				case SEARCH_TYPE:
					file_name = params[0];
					ArrayList<String> found = mFileMang.searchInDirectory(mFileMang.getCurrentDir(), 
																	    file_name);
					return found;
					
				case COPY_TYPE:
					int len = params.length;
					
					if(mMultiSelectData != null && !mMultiSelectData.isEmpty()) {
						Log.d("mydebug","copy multifile="+len);
						for(int i = 1; i < len; i++) {
							copy_rtn = mFileMang.copyToDirectory(params[i], params[0]);
							
							if(delete_after_copy && copy_rtn >= 0 && copy_rtn != 1) //return 1 means it cut on the same dir.
								mFileMang.deleteTarget(params[i]);
						}
					} else {
						Log.d("mydebug","copy singlefile="+len);
						if(params[0]==null || params[1]==null)
							copy_rtn = -1;
						else
							copy_rtn = mFileMang.copyToDirectory(params[0], params[1]);
						
						if(delete_after_copy && copy_rtn >= 0 && copy_rtn != 1)
							mFileMang.deleteTarget(params[0]);
					}
					
					delete_after_copy = false;
					return null;
					
				case UNZIP_TYPE:
					mFileMang.extractZipFiles_forCh(params[0], params[1]);
					return null;
					
				case UNZIPTO_TYPE:
					mFileMang.extractZipFilesFromDir_forCh(params[0], params[1], params[2]);
					return null;
					
				case ZIP_TYPE:
					mFileMang.createZipFile(params[0]);
					return null;
					
				case DELETE_TYPE:
					int size = params.length;
					ArrayList<String> tmpSelectData = null;
					
					if(mMultiSelectData!=null)
					{
						tmpSelectData = new ArrayList<String>();
						tmpSelectData.addAll(mMultiSelectData);
					}
					
					for(int i = 0; i < size; i++)
					{
						mFileMang.deleteTarget(params[i]);
						Log.d("mydebug","params[i]:"+params[i]);
						if(tmpSelectData!=null && mMultiSelectData != null && !mMultiSelectData.isEmpty()) {
						//	mMultiSelectData.clear();
						//	multi_select_flag = false;
							
							for(String s : tmpSelectData)
							{
								//data[index++] = s;
								if(params[i].equals(s))
								{
									mMultiSelectData.remove(s);
									Log.d("mydebug","ss:"+s);
								}
							}
							if(mMultiSelectData.size()==0)
							{
								Log.d("mydebug","clear mMultiSelectData after delete:"+mMultiSelectData.size());
							//	mMultiSelectData.clear();
								mMultiSelectData = null;	
								multi_select_flag = false;
							}
						}
					}
					tmpSelectData = null;
					
					return null;
			}
			return null;
		}
		
    	/**
    	 * This is called when the background thread is finished. Like onPreExecute, anything
    	 * here will be done on the EDT thread. 
    	 */
    	@Override
		protected void onPostExecute(final ArrayList<String> file) {			
			final CharSequence[] names;
			int len = file != null ? file.size() : 0;
			
			if(wl != null)
				wl.release();
			
			switch(type) {
				case SEARCH_TYPE:
					if(len == 0) {
						Toast.makeText(mContext, mContext.getResources().getString(R.string.could_not_find) + " " + file_name, 
											Toast.LENGTH_SHORT).show();
					
					} else {
						names = new CharSequence[len];
						
						for (int i = 0; i < len; i++) {
							String entry = file.get(i);
							names[i] = entry.substring(entry.lastIndexOf("/") + 1, entry.length());
						}
						
						AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
						builder.setTitle(mContext.getResources().getString(R.string.find) + " " + len + " " + mContext.getResources().getString(R.string.files));
						builder.setItems(names, new DialogInterface.OnClickListener() {
							
							public void onClick(DialogInterface dialog, int position) {
								String path = file.get(position);

								File f = new File(path);
						    	String item_ext = null;
						    	
						    	try {
						    		item_ext = path.substring(path.lastIndexOf(".") + 1, path.length());
						    		
						    	} catch(IndexOutOfBoundsException e) {	
						    		item_ext = ""; 
						    	}
								if(f.exists())
								{
									if(f.isDirectory())
									{
										if(f.canRead()) {
								    		updateDirectory(mFileMang.getNextDir(path, true));
								    		//mPathLabel.setText(mFileMang.getCurrentDir());
								    		mPathLabel.setText(mFileMang.getCurrentDir().substring(5, mFileMang.getCurrentDir().length()));
							    		}
									}
									else if (TypeFilter.getInstance().isMusicFile(item_ext)) {
						                Intent picIntent = new Intent();
						                picIntent.setAction(android.content.Intent.ACTION_VIEW);
						                picIntent.setDataAndType(Uri.fromFile(f), "audio/*");
						                try{
						                	mContext.startActivity(picIntent);
						                }catch(ActivityNotFoundException e)
						                {
						                	//Log.e("EventHandler", "can not find activity to open it");
						                }
							    	}
							    	else if(TypeFilter.getInstance().isPictureFile(item_ext)) {  		
								    	Intent picIntent = new Intent();
								    	picIntent.setAction(android.content.Intent.ACTION_VIEW);
								    	picIntent.setDataAndType(Uri.fromFile(f), "image/*");
								    	try
								    	{
								    		mContext.startActivity(picIntent);
								    	}catch(ActivityNotFoundException e)
								    	{
								    		//Log.e("EventHandler", "can not find activity to open it");
								    	}
							    	}
							    	else if(TypeFilter.getInstance().isMovieFile(item_ext)) {
									    Intent movieIntent = new Intent();
									    movieIntent.putExtra(MediaStore.EXTRA_FINISH_ON_COMPLETION, false);
									    movieIntent.setAction(android.content.Intent.ACTION_VIEW);
									    movieIntent.setDataAndType(Uri.fromFile(f), "video/*");
									    try{
									    	mContext.startActivity(movieIntent);
									    }catch(ActivityNotFoundException e)
									    {
									    	//Log.e("EventHandler", "can not find activity to open it");
									    }
							    	}
							    	else if(TypeFilter.getInstance().isApkFile(item_ext)){
								    	Intent apkIntent = new Intent();
								    	apkIntent.setAction(android.content.Intent.ACTION_VIEW);
								    	apkIntent.setDataAndType(Uri.fromFile(f), "application/vnd.android.package-archive");
								    	try {
								    		mContext.startActivity(apkIntent);
										} catch (ActivityNotFoundException e) {
											//Log.e("EventHandler", "can not find activity to open it");
										}
								    	
							    	}
									/* text file*/
									else if(TypeFilter.getInstance().isTxtFile(item_ext)) {
										if(f.length() != -1) {
											Intent txtIntent = new Intent();
											txtIntent.setAction(android.content.Intent.ACTION_VIEW);
											txtIntent.setDataAndType(Uri.fromFile(f), "text/plain");
											try {
													mContext.startActivity(txtIntent);
												} catch(ActivityNotFoundException e) {
													txtIntent.setType("text/*");
													mContext.startActivity(txtIntent);
											}
										}
									}
								}
							}
						});
						AlertDialog dialog = builder.create();
						dialog.show();
					}
					pr_dialog.dismiss();
					break;
					
				case COPY_TYPE:
				/*	if(mMultiSelectData != null && !mMultiSelectData.isEmpty()) {
						multi_select_flag = false;
						mMultiSelectData.clear();
					}*/
					if(mDelegate!=null)
						mDelegate.killMultiSelect(true);
					
					if(copy_rtn == 0)
						Toast.makeText(mContext, mContext.getResources().getString(R.string.copied_pasted_success), 
											Toast.LENGTH_SHORT).show();
					else
						Toast.makeText(mContext, mContext.getResources().getString(R.string.copy_pasted_failed), Toast.LENGTH_SHORT).show();
					updateDirectory(mFileMang.getNextDir(mFileMang.getCurrentDir(), true));
					pr_dialog.dismiss();
					mInfoLabel.setText("");
					break;
					
				case UNZIP_TYPE:
					updateDirectory(mFileMang.getNextDir(mFileMang.getCurrentDir(), true));
					pr_dialog.dismiss();
					break;
					
				case UNZIPTO_TYPE:
					updateDirectory(mFileMang.getNextDir(mFileMang.getCurrentDir(), true));
					pr_dialog.dismiss();
					break;
					
				case ZIP_TYPE:
					updateDirectory(mFileMang.getNextDir(mFileMang.getCurrentDir(), true));
					pr_dialog.dismiss();
					break;
					
				case DELETE_TYPE:
				/*	if(mMultiSelectData != null && !mMultiSelectData.isEmpty()) {
					//	mMultiSelectData.clear();
					//	multi_select_flag = false;

						for(String s : mMultiSelectData)
							data[index++] = s;
					}
					*/
					
					updateDirectory(mFileMang.getNextDir(mFileMang.getCurrentDir(), true));
					pr_dialog.dismiss();
					mInfoLabel.setText("");
					break;
			}
		}
    }
}
