
package com.actions.explore;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.Stack;
import java.util.zip.ZipInputStream;
import android.content.Context;
import android.os.StatFs;
import android.util.Log;

/*
 * @author lizihao@actions-semi.com
 */
public class FileManager {
	public  static final int ROOT_FLASH = 0;
	public  static final int ROOT_SDCARD = 1;
	public  static final int ROOT_USBHOST = 2;
	public	static final int ROOT_UNKNOWN = 3;
	
	private static final int BUFFER = 2048;
	private static final int SORT_NONE = 0;
	private static final int SORT_ALPHA = 1;
	private static final int SORT_TYPE = 2;
	
	private String flashPath;
	private String sdcardPath;
	private String usbPath;
	public DevicePath mDevices;
	
	public static boolean canMkdir = true;
	public static boolean moveOrCopyFile = false;
	public static boolean mHoldingFile = false;
	public static boolean mAllowSelectAll = false;
	
	private boolean mShowHiddenFiles = false;
	private int mSortType = SORT_ALPHA;
	private long mDirSize = 0;
	private Stack<String> mPathStack;
	private ArrayList<String> mDirContent;
	
	public static long mCopyOrMoveSize = 0;
	
	private Context mContext ;

	public FileManager(Context context) {
		mDirContent = new ArrayList<String>();
		mPathStack = new Stack<String>();
		
		mDevices = new DevicePath(context);
		flashPath = mDevices.getSdStoragePath();
		sdcardPath = mDevices.getInterStoragePath();
		usbPath = mDevices.getUsbStoragePath();
		mPathStack.push("/");
		mPathStack.push(sdcardPath);
		
		mContext = context;
	}
	
	/**
	 * This will return a string of the current directory path
	 * @return the current directory
	 */
	public String getCurrentDir() {
		return mPathStack.peek();
	}
	
        /**
         * Return the upper dir absolutely path
         * @author caichsh
         * @param src
         * @return
         */
        private String getUpperDir(String src) {
                if(src == null) 
					return "";
                return src.substring(0, src.lastIndexOf("/"));
        }

	private boolean checkCopyValid (String src, String dst)
	{
		if(src==null || dst==null)
			return false;

		String src_name = src;
		if(src.charAt(src.length() - 1) != '/')
			src_name += "/";
		
		if(dst.contains(src_name)){
			Log.d("action_explore","src:"+src_name);
			Log.d("action_explore","dst:"+dst);
			return false;
		}

		return true;
	}
           
	/**
	 * This will return a string of the current home path.
	 * @return	the home directory
	 */
	public ArrayList<String> getHomeDir(int root_type) {
		//This will eventually be placed as a settings item
		mPathStack.clear();
		mPathStack.push("/");
		switch(root_type)
			{
			case ROOT_SDCARD:
				mPathStack.push(sdcardPath);
				break;
				
			case ROOT_USBHOST:
				mPathStack.push(usbPath);
				break;
				
			case ROOT_FLASH:
			default:
				mPathStack.push(flashPath);
				break;
			}

		return populate_list();
	}
	
	/**
	 * This will tell if current path is root
	 * @return	is root?
	 */
	public boolean isRoot() {
		String tmp = mPathStack.peek();
		
		if(tmp.equals(sdcardPath) ||
			tmp.equals(usbPath) ||
			tmp.equals(flashPath)){
			return true;
		}
		
		return false;
	}
	
	/**
	 * This will tell which root we are in;
	 * @return	which root?
	 */
	public int whichRoot() {
		//This will eventually be placed as a settings item
		String tmp = mPathStack.peek();
		
	//	Log.d("mydebug","mPathStack:"+mPathStack);
	//	Log.d("mydebug","tmp:"+tmp);
		
		if(tmp.startsWith(sdcardPath))
		{
			return ROOT_SDCARD;
		}
		if(tmp.startsWith(usbPath))
		{
			return ROOT_USBHOST;
		}
		if(tmp.startsWith(flashPath))
		{
			return ROOT_FLASH;
		}
		
		return ROOT_UNKNOWN;
	}
	
	/**
	 * This will determine if hidden files and folders will be visible to the
	 * user.
	 * @param choice	true if user is veiwing hidden files, false otherwise
	 */
	public void setShowHiddenFiles(boolean choice) {
		mShowHiddenFiles = choice;
	}
	
	/**
	 * 
	 * @param type
	 */
	public void setSortType(int type) {
		mSortType = type;
	}
	
	/**
	 * This will return a string that represents the path of the previous path
	 * @return	returns the previous path
	 */
	public ArrayList<String> getPreviousDir() {
		int size = mPathStack.size();
		
		if (size >= 2)
			mPathStack.pop();
		
		else if(size == 0)
			mPathStack.push("/");
		
		return populate_list();
	}
	
	/**
	 * 
	 * @param path
	 * @param isFullPath
	 * @return
	 */
	public ArrayList<String> getNextDir(String path, boolean isFullPath) {
		int size = mPathStack.size();
		
		if(!path.equals(mPathStack.peek()) && !isFullPath) {
			if(size == 1)
				mPathStack.push("/" + path);
			else
				mPathStack.push(mPathStack.peek() + "/" + path);
		}
		
		else if(!path.equals(mPathStack.peek()) && isFullPath) {
			mPathStack.push(path);
		}
		
		return populate_list();
	}

	/**
	 * 
	 * @param old		the file to be copied
	 * @param newDir	the directory to move the file to
	 * @return
	 */
	public int copyToDirectory(String old, String newDir) {

		if(getUpperDir(old).equals(newDir)) 
			return 1;

		if(checkCopyValid(old,newDir)==false)
			return 1;

		File old_file = new File(old);
		File temp_dir = new File(newDir);
		byte[] data = new byte[BUFFER];
		int read = 0;


		
		if(old_file.isFile() && temp_dir.isDirectory() && temp_dir.canWrite()){
			String file_name = old.substring(old.lastIndexOf("/"), old.length());
			String new_name = newDir + file_name;

			File cp_file = new File(new_name);
			try {
				BufferedOutputStream o_stream = new BufferedOutputStream(
												new FileOutputStream(cp_file));
				BufferedInputStream i_stream = new BufferedInputStream(
											   new FileInputStream(old_file));
				
				while((read = i_stream.read(data, 0, BUFFER)) != -1)
					o_stream.write(data, 0, read);
				
				o_stream.flush();
				i_stream.close();
				o_stream.close();
				
				RefreshMedia mRefresh = new RefreshMedia(mContext);
				mRefresh.notifyMediaAdd(new_name);
			} catch (FileNotFoundException e) {
				Log.e("FileNotFoundException", e.getMessage());
				return -1;
				
			} catch (IOException e) {
				Log.e("IOException", e.getMessage());
				return -1;
			}
			
		}else if(old_file.isDirectory() && temp_dir.isDirectory() && temp_dir.canWrite()) {
			String files[] = old_file.list();
			String dir = newDir + old.substring(old.lastIndexOf("/"), old.length());
			int len = files.length;
			int ret;
			File dirFile = new File(dir);
			
			if(!dirFile.exists())
			{
				if(!dirFile.mkdir())
				{
					return -1;
				}
			}
			
			for(int i = 0; i < len; i++)
			{
				ret = copyToDirectory(old + "/" + files[i], dir);
				if(ret!=0)
				{
					return -1;
				}
			}
			
		} else if(!temp_dir.canWrite())
		{
			return -1;
		}
		
		return 0;
	}
	
	public void extractZipFilesFromDir_forEng(String zipName, String toDir, String fromDir) {
		byte[] data = new byte[BUFFER];
		java.util.zip.ZipEntry entry;
		ZipInputStream zipstream;
		
		String org_path = fromDir + "/" + zipName;
		String dest_path = toDir + zipName.substring(0, zipName.length() - 4);
		String zipDir = dest_path + "/";
		
		new File(zipDir).mkdir();
		try {
			zipstream = new ZipInputStream(new FileInputStream(org_path));
			
			while((entry = (java.util.zip.ZipEntry) zipstream.getNextEntry()) != null) {
				if(entry.isDirectory()) {
					String ndir = zipDir + entry.getName() + "/";
					
					new File(ndir).mkdir();
					
				} else {
					int read = 0;
					FileOutputStream out = new FileOutputStream(
												zipDir + entry.getName());
					while((read = zipstream.read(data, 0, BUFFER)) != -1)
						out.write(data, 0, read);
					
					zipstream.closeEntry();
					out.close();
					
					RefreshMedia mRefresh = new RefreshMedia(mContext);
					mRefresh.notifyMediaAdd(zipDir + entry.getName());
				}
			}
			
		} catch (FileNotFoundException e) {
			e.printStackTrace();
			
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
		
	public void extractZipFilesFromDir_forCh(String zipName, String toDir, String fromDir) {
		String zip_file = fromDir + "/" + zipName;
		extractZipFiles_forCh(zip_file, toDir);
	}
	
	public void extractZipFiles_forEng(String zip_file, String directory) {
		byte[] data = new byte[BUFFER];
		java.util.zip.ZipEntry entry;
		ZipInputStream zipstream;
		
		String path = zip_file;
		String name = path.substring(path.lastIndexOf("/") + 1, 
									 path.length() - 4);
		String zipDir = path.substring(0, path.lastIndexOf("/") +1) + 
									   name + "/";
		new File(zipDir).mkdir();
		try {
			zipstream = new ZipInputStream(new FileInputStream(path));
			
			while((entry = zipstream.getNextEntry()) != null) {
				if(entry.isDirectory()) {
					String ndir = zipDir + entry.getName() + "/";

					new File(ndir).mkdir();
					
				} else {
					int read = 0;
					FileOutputStream out = new FileOutputStream(
											zipDir + entry.getName());
					while((read = zipstream.read(data, 0, BUFFER)) != -1)
						out.write(data, 0, read);
					
					zipstream.closeEntry();
					out.close();
					
					RefreshMedia mRefresh = new RefreshMedia(mContext);
					mRefresh.notifyMediaAdd(zipDir + entry.getName());
				}
			}
			
		} catch (FileNotFoundException e) {
			e.printStackTrace();
			
		} catch (IOException e) {
			e.printStackTrace();
		}
	}
	
	public void extractZipFiles_forCh(String zip_file, String directory){
	//	directory = directory + zip_file.subSequence(zip_file.lastIndexOf("/")+1, zip_file.lastIndexOf("."))+"/";
		try {
			BufferedInputStream bi;
			org.apache.tools.zip.ZipFile zfile = new org.apache.tools.zip.ZipFile(zip_file, "GBK");
			Enumeration zList = zfile.getEntries();
			org.apache.tools.zip.ZipEntry ze = null;
			
			if (!zList.hasMoreElements()) { //means empty folder
				String path = directory ;
				File fileDirFile = new File(path);
				if (!fileDirFile.exists()){
					fileDirFile.mkdirs();
				}
				zfile.close();
				return;
			}
			
			while(zList.hasMoreElements()){
				ze = (org.apache.tools.zip.ZipEntry)zList.nextElement();
				String entryName = ze.getName();
				String path = directory + "/" + entryName;
				
				if(ze.isDirectory()){
					File decompressDirFile = new File(path);
					if (!decompressDirFile.exists()) {
						decompressDirFile.mkdirs();
					}
				} else {
					String fileDir = path.substring(0, path.lastIndexOf("/"));
					File fileDirFile = new File(fileDir);
					if (!fileDirFile.exists()){
						fileDirFile.mkdirs();
					}
					BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(directory + "/" + entryName));
					bi = new BufferedInputStream(zfile.getInputStream(ze));
					byte[] readContent = new byte[1024];
					int readCount = bi.read(readContent);
					while (readCount != -1){
						bos.write(readContent, 0, readCount);
						readCount = bi.read(readContent);
					}
					bos.close();
					
					RefreshMedia mRefresh = new RefreshMedia(mContext);
					mRefresh.notifyMediaAdd(fileDir + ze.getName());
				}
			}
			zfile.close();
			
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	/**
	 * 
	 * @param path
	 */
	public void createZipFile(String path) {
		String mPath = path;
		String zipFilePath = path.substring(path.lastIndexOf("/")+1, path.length()) + ".zip";
		path = path.substring(0, path.lastIndexOf("/")+1);
		path = path + zipFilePath;
		ZipControl mZipControl = new ZipControl();
		try {
			mZipControl.writeByApacheZipOutputStream(new String[]{mPath}, path, "lizihao@actions-semi.com");	
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}finally {
			File file = new File(path);
			if(file.exists()){
				RefreshMedia mRefresh = new RefreshMedia(mContext);
				mRefresh.notifyMediaAdd(path);
			}
		}
	}
	
	/**
	 * 
	 * @param filePath
	 * @param newName
	 * @return
	 */
	public int renameTarget(String filePath, String newName) {
		File src = new File(filePath);
		String ext = "";
		File dest;
		
		if(src.isFile())
			ext = filePath.substring(filePath.lastIndexOf("."), filePath.length());
		
		if(newName.length() < 1)
			return -1;
	
		String temp = filePath.substring(0, filePath.lastIndexOf("/"));
		
		String destPath = temp + "/" + newName + ext;
		dest = new File(destPath);
		if(src.renameTo(dest))
		{
			RefreshMedia mRefresh = new RefreshMedia(mContext);
			if(src.isFile())
				mRefresh.notifyMediaAdd(destPath);
			else
				mRefresh.notifyMediaAdd(src.getParent());
			return 0;
		}

		return -1;
	}
	
	/**
	 * 
	 * @param path
	 * @param name
	 * @return
	 */
	public int createDir(String path, String name) {
		int len = path.length();
		
		if(len < 1 || len < 1)
			return -1;
		
		if(path.charAt(len - 1) != '/')
			path += "/";

		String filename = path+name;
		
		if (new File(filename).mkdir()){
			if(name.equals(".actions_mount")==false){
				/*File hideFile = new File(filename+"/"+".actHide");  
				if(hideFile.exists()==false){
					 //
					try {  
						if (hideFile.createNewFile()) {
							RefreshMedia mRefresh = new RefreshMedia(mContext);
							mRefresh.notifyMediaAdd(filename+"/"+".actHide");	
						//	hideFile.delete();
							return 0;
						} 
					} catch (IOException e) {  
						e.printStackTrace();   
					} 
					
				}
				RefreshMedia mRefresh = new RefreshMedia(mContext);
				mRefresh.notifyMediaAdd(filename+"/"+".actHide");
				*/
				RefreshMedia mRefresh = new RefreshMedia(mContext);
				mRefresh.notifyMediaAdd(path);
			}	
			return 0; 
		}
		
		return -1;
	}
	
	/**
	 * The full path name of the file to delete.
	 * 
	 * @param path name
	 * @return
	 */
	public int deleteTarget(String path) {
		File target = new File(path);
		
		if(target.exists() && target.isFile() && target.canWrite()) {
			target.delete();
			RefreshMedia mRefresh = new RefreshMedia(mContext);
			mRefresh.notifyMediaDelete(path);
			
			return 0;
		}
		else if(target.exists() && target.isDirectory() && target.canRead()) {
			String[] file_list = target.list();
			
			if(file_list != null && file_list.length == 0) {
				target.delete();
				RefreshMedia mRefresh = new RefreshMedia(mContext);
				mRefresh.notifyMediaDelete(target.getAbsolutePath());
				return 0;
				
			} else if(file_list != null && file_list.length > 0) {
				Log.d("mydebug","del target:"+path);
				for(int i = 0; i < file_list.length; i++) {
					String filePath = target.getAbsolutePath() + "/" + file_list[i];
					File temp_f = new File(filePath);

					Log.d("mydebug","del dir:"+filePath);

					if(temp_f.isDirectory())
					{
						deleteTarget(temp_f.getAbsolutePath());
						RefreshMedia mRefresh = new RefreshMedia(mContext);
						mRefresh.notifyMediaDelete(temp_f.getAbsolutePath());
					}
					else if(temp_f.isFile())
					{
						temp_f.delete();
						
						RefreshMedia mRefresh = new RefreshMedia(mContext);
						mRefresh.notifyMediaDelete(filePath);
					}
				}
				Log.d("mydebug","del di2r:");
			}
			if(target.exists()){
				if(target.delete()){
					RefreshMedia mRefresh = new RefreshMedia(mContext);
					mRefresh.notifyMediaDelete(target.getAbsolutePath());
					return 0;
				}
			}
		}	
		return -1;
	}
	
	/**
	 * 
	 * @param name
	 * @return
	 */
	public boolean isDirectory(String name) {
		return new File(mPathStack.peek() + "/" + name).isDirectory();
	}
		
	/**
	 * converts integer from wifi manager to an IP address. 
	 * 
	 * @param des
	 * @return
	 */
	public static String integerToIPAddress(int ip) {
		String ascii_address = "";
		int[] num = new int[4];
		
		num[0] = (ip & 0xff000000) >> 24;
		num[1] = (ip & 0x00ff0000) >> 16;
		num[2] = (ip & 0x0000ff00) >> 8;
		num[3] = ip & 0x000000ff;
		 
		ascii_address = num[0] + "." + num[1] + "." + num[2] + "." + num[3];
		 
		return ascii_address;
	 }
	
	/**
	 * 
	 * @param dir
	 * @param pathName
	 * @return
	 */
	public ArrayList<String> searchInDirectory(String dir, String pathName) {
		ArrayList<String> names = new ArrayList<String>();
		search_file(dir, pathName, names);

		return names;
	}
	
	/**
	 * 
	 * @param path
	 * @return
	 */
	public long getDirSize(String path) {
		get_dir_size(new File(path));

		return mDirSize;
	}
	
	
	private static final Comparator alph = new Comparator<String>() {
		@Override
		public int compare(String arg0, String arg1) {
			return arg0.toLowerCase().compareTo(arg1.toLowerCase());
		}
	};
	
	private static final Comparator type = new Comparator<String>() {
		@Override
		public int compare(String arg0, String arg1) {
			String ext = null;
			String ext2 = null;
			
			try {
				ext = arg0.substring(arg0.lastIndexOf(".") + 1, arg0.length());
				ext2 = arg1.substring(arg1.lastIndexOf(".") + 1, arg1.length());
				
			} catch (IndexOutOfBoundsException e) {
				return 0;
			}
			
			return ext.compareTo(ext2);
		}
	};
	
	/* (non-Javadoc)
	 * this function will take the string from the top of the directory stack
	 * and list all files/folders that are in it and return that list so 
	 * it can be displayed. Since this function is called every time we need
	 * to update the the list of files to be shown to the user, this is where 
	 * we do our sorting (by type, alphabetical, etc).
	 * 
	 * @return
	 */
	private ArrayList<String> populate_list() {

		if(!mDirContent.isEmpty())
			mDirContent.clear();
		try
		{
			String path = mPathStack.peek();
			File file = new File(path); 
		
			if(file.exists() && file.canRead() && file.isDirectory()) {
//				String[] list = file.list();
				File[] fList = file.listFiles();
				boolean isPartition = false;
				if(mDevices.hasMultiplePartition(path)){
					Log.d("chen",path + " has multi partition");
					isPartition = true;
				}
				if(fList != null)
				{
					int len = fList.length;
					
					for (int i = 0; i < len; i++) {
						if(isPartition){
							try{
								StatFs statFs = new StatFs(fList[i].getAbsolutePath());
								int count = statFs.getBlockCount();
								Log.d("lizihao ",fList[i].getName() + "  " + count);
								if(count == 0){
									continue;
								}
							}catch(Exception e){
								Log.d("lizihao ",fList[i].getName() + "  exception");
								continue;
							}
						}
						String name = fList[i].getName();
						if(!mShowHiddenFiles) {
							if(name.charAt(0) != '.')
								mDirContent.add(name);
					
						} else {
							mDirContent.add(name);
						}
					}
			
					switch(mSortType) 
					{
						case SORT_NONE:
							//no sorting needed
							break;
					
						case SORT_ALPHA:
							Object[] tt = mDirContent.toArray();
							mDirContent.clear();
					
							Arrays.sort(tt, alph);
					
							for (Object a : tt){
								mDirContent.add((String)a);
							}
							break;
					
						case SORT_TYPE:
							Object[] t = mDirContent.toArray();
							String dir = mPathStack.peek();
					
							Arrays.sort(t, type);
							mDirContent.clear();
					
							for (Object a : t){
								if(new File(dir + "/" + (String)a).isDirectory())
									mDirContent.add(0, (String)a);
								else
									mDirContent.add((String)a);
							}
							break;
					}
				}	
			} 
		}catch(Exception e)
		{
			/* clear any operate made above */
			Log.e("FileManager", "unknow exception");
			mDirContent.clear();
		}
		return mDirContent;
	}
	
	private void zip_folder(File file, java.util.zip.ZipOutputStream zout) throws IOException {
		byte[] data = new byte[BUFFER];
		int read;
		
		if(file.isFile()){
			java.util.zip.ZipEntry entry = new java.util.zip.ZipEntry(file.getName());
			zout.putNextEntry(entry);
			BufferedInputStream instream = new BufferedInputStream(
										   new FileInputStream(file));
			
			while((read = instream.read(data, 0, BUFFER)) != -1)
				zout.write(data, 0, read);
			
			zout.closeEntry();
			instream.close();
			
		} else {
			String[] list = file.list();
			int len = list.length;
			
			for(int i = 0; i < len; i++)
				zip_folder(new File(file.getPath() +"/"+ list[i]), zout);
		}
	}
	
	private void get_dir_size(File path) {
		File[] list = path.listFiles();
		int len;
		
		if(list != null) {
			len = list.length;
			
			for (int i = 0; i < len; i++) {
				if(list[i].isFile() && list[i].canRead()) {
					mDirSize += list[i].length();
					if( list[i].length()>(100*0x100000))
					{
						Log.d("mydebug","pathSize1:"+list[i]);
					}
					
				} else if(list[i].isDirectory() && list[i].canRead()) { 
					get_dir_size(list[i]);
				}
			}
		}
	}

	private void search_file(String dir, String fileName, ArrayList<String> n) {
		File root_dir = new File(dir);
		String[] list = root_dir.list();
		
		if(list != null && root_dir.canRead()) {
			int len = list.length;
			
			for (int i = 0; i < len; i++) {
				File check = new File(dir + "/" + list[i]);
				String name = check.getName();
					
				if(check.isFile() && name.toLowerCase().
										contains(fileName.toLowerCase())) {
					n.add(check.getPath());
				}
				else if(check.isDirectory()) {
					if(name.toLowerCase().contains(fileName.toLowerCase()))
						n.add(check.getPath());
					
					else if(check.canRead() && !dir.equals("/"))
						search_file(check.getAbsolutePath(), fileName, n);
				}
			}
		}
	}

	public void resetDirSize(){
		Log.d("mydebug","resetDirSize:"+mDirSize);
		mDirSize = 0;
	}
	
	public long getDirOrFileSize(String mPath){
    	String mSelectTarget = mPath;
		File mFile = new File(mSelectTarget);
		if(mFile.isDirectory()){
			return getDirSize(mSelectTarget);
		} else if(mFile.isFile()){
			if(mFile.length()>(100*0x100000))
			{
				Log.d("mydebug","pathSize0:"+mPath);
			}
			return mFile.length();
		}
    	return 0;
    }
    
    public long getMemorySize() {
    	String path ="";
        switch (whichRoot()) {
		case 0:
			path = mDevices.getSdStoragePath();
			break;
		case 1:
			path = mDevices.getInterStoragePath();
			break;
		case 2:
			path = mDevices.getUsbStoragePath();
		    break;
		default:
			Log.d("mydebug","whichRoot fail");
			break;
		}
        Log.d("mydebug","pathroot:"+path);
        StatFs stat = getStatFs(path);
        return calculateSizeInMB(stat);
    }
    
    public long calculateSizeInMB(StatFs stat) {
        if (stat != null)
		{
			Log.d("mydebug","block1:"+stat.getAvailableBlocks());
			Log.d("mydebug","block2:"+stat.getBlockSize());
			return (long)(stat.getAvailableBlocks() * (stat.getBlockSize() / (1024f * 1024f)) * 1024 * 1024);
		}
        return 0;
    }
    
    public StatFs getStatFs(String path) {
        try {
            return new StatFs(path);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
}
