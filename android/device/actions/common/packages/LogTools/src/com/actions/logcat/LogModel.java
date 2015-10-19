package com.actions.logcat;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.annotation.SuppressLint;  
import android.app.ActivityManager;
import android.content.Context;
import android.os.Build;
import android.os.StatFs;
import android.os.SystemProperties;
import android.util.Log;

@SuppressLint("SdCardPath")
public class LogModel
{
	protected static final String TAG = "LogModel"; 
	static final String SDCARD_PATH = "/mnt/sd-ext";
	static final String FLASH_PATH = "/mnt/sdcard";
	static final String DATA_PATH = "/data/data";
	static final String logcatServiceBinLocation = "/system/xbin/actionslogcat"; //if use actionslogcat as service, it will default put in this location
	static final String logcatBinLocation = "/sdcard/actionslogcat"; //if use actionslogcat as exe, apk will copy it to this location from /res/raw
	static final String rootUserName = "root";
	static final int SERVICE_MODE = 1;
	static final int EXE_MODE = 2;
	
	private int serviceMode = 0;
	private Context mContext;
	private String logFullName;
	private long    storageTotalSize = 0;
	private long    storageAvailableSize = 0;
	private int    suggestSize = 0;
	private String  LogDir = null;
	private File 	logFile;
	
	public LogModel(Context context) {
		mContext = context;
		LogDir = FLASH_PATH;
		scanStorageDevice();
		
		Log.d(TAG,"logdir="+LogDir);
		if(LogDir!=null){
			File dirFile = new File(LogDir);	
			if (!dirFile.exists()){
				dirFile.mkdirs();
				if(!dirFile.exists()) {
					LogDir = null;
				} 
			}
		}
		
		if(LogDir!=null){
			storageTotalSize = getTotalSize(LogDir);
			storageAvailableSize = getFreeSize(LogDir);
			suggestSize = Math.round(storageAvailableSize*0.7f);
		}
	
		if(fileIsExist (logcatServiceBinLocation))
		{
			serviceMode = SERVICE_MODE;
		}
		else{
			if(LogDir!=null)
				copyRawFile();
			
			serviceMode = EXE_MODE;
		}			
	}

	public int getServiceMode(){
		return serviceMode;
	}
	
	public String getLogDir(){
		return LogDir;
	}
	
	public String getLogFullName (){
		return logFullName;
	}
	
	public long getStorageTotalSize() {
		storageTotalSize = getTotalSize(LogDir);
		return storageTotalSize;
	}
	
	public long getStorageAvailableSize(){
		storageAvailableSize = getFreeSize(LogDir);
		return storageAvailableSize;
	}

	public int getSuggestSize(){
		suggestSize = Math.round(getFreeSize(LogDir)*0.7f);
		return suggestSize > 1000? 1000: suggestSize;
	}
	
	public int scanStorageDevice ()
	{
		if (canWrite(SDCARD_PATH)){
			Log.d(TAG,"TF card is valid");
			LogDir = SDCARD_PATH;
		} 
		else  
		{
			if (canWrite(FLASH_PATH))
				LogDir = FLASH_PATH;
			else if (canWrite(DATA_PATH))
				LogDir = DATA_PATH;
		}
		LogDir += File.separator+mContext.getPackageName()+File.separator+"log";
		return 0;
	}

	static public long getTotalSize(String path){ 
		StatFs sf = new StatFs(path);   
		long blockSize = sf.getBlockSizeLong();//sf.getBlockSize();   
		long allBlocks = sf.getBlockCountLong();   
	// return (allBlocks * blockSize)/1024/1024; //MB  
		return (allBlocks * blockSize)>>20;
		}  

	static public long getFreeSize(String path){    
		StatFs sf = new StatFs(path);    
		long blockSize = sf.getBlockSizeLong();  
		long freeBlocks = sf.getAvailableBlocksLong();  	      
		// return (freeBlocks * blockSize)/1024 /1024;  
		return (freeBlocks * blockSize)>>20;
	} 

	
	public boolean fileIsExist(String filename)
	{
		File file = new File(filename);
		if(file.exists())
			return true;

		return false;
	}
	
	void copyRawFile(){
		String dstfile = logcatBinLocation;
		if(fileIsExist(dstfile)){
			Log.d("mydebug","file:"+dstfile+"is already existed!");
			return;
		}
		
		File file = new File(dstfile);
		if (file==null || file.exists()){
			return;
		}
		
		File parent = file.getParentFile();
		if(parent==null)
			return;
		
		if (!parent.exists()){
			parent.mkdirs();
			if(!parent.exists()) {
				Log.d(TAG,"create dir fail:"+parent.getPath());
				return;
			} 
		} 
		
		try {   
				file.createNewFile();
				InputStream is = mContext.getResources().openRawResource(R.raw.actionslogcat);   
		        FileOutputStream fos = new FileOutputStream(file);   
		        byte[] buffer = new byte[8192];   
		        int count = 0; 
		        while ((count = is.read(buffer)) > 0) { 
					fos.write(buffer, 0, count);   
		        }   
				fos.close();   
		        is.close();  
			} catch (Exception e) {   
				e.printStackTrace();  
			} 
	}

	public boolean processIsExist(String name){
		Process psProcess = null;
		try {
			psProcess = Runtime.getRuntime().exec("ps");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		InputStream is = psProcess.getInputStream(); 
		BufferedReader br = new BufferedReader(new InputStreamReader(is));
		String line = null;
		String[] ps = null;
		boolean isFirst = true;
		try {
			while((line = br.readLine()) != null) {
				if(isFirst) {
					isFirst = false;
					continue;
				}
				line = line.replaceAll(" {2,}", " ");
				ps =line.trim().split(" ");
			//	Log.d(TAG,"ps[0]="+ps[0]);
			//	Log.d(TAG,"ps[1]="+ps[1]);
			//	Log.d(TAG,"process="+ps[ps.length-1]);
				if(name.equals(ps[ps.length-1]) || ("sh" == ps[ps.length-1] && (rootUserName != null && rootUserName.equals(ps[0])))) 
				{
					Log.d(TAG,"find process:"+name);
					psProcess.destroy();
					return true;
				}
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		psProcess.destroy();
		return false;
	}
	
	/**
	 * 
	 * @param name
	 * @return the list of the pid by name, in fact, it is only fit to this app
	 * @throws Exception
	 */
	private List<String> getPidByProcessName(String name) {
		if(rootUserName == null) {
			Log.e(TAG, "user name is null, kill process failed");
			return null;
		}
		List<String> pids = new ArrayList<String>();
		Process psProcess = null;
		try {
			psProcess = Runtime.getRuntime().exec("ps");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		InputStream is = psProcess.getInputStream(); 
		BufferedReader br = new BufferedReader(new InputStreamReader(is));
		String line = null;
		String[] ps = null;
		boolean isFirst = true;
		try {
			while((line = br.readLine()) != null) {
				if(isFirst) {
					isFirst = false;
					continue;
				}
				line = line.replaceAll(" {2,}", " ");
				ps =line.trim().split(" ");
			//	Log.d(TAG,"ps[0]="+ps[0]);
			//	Log.d(TAG,"ps[1]="+ps[1]);
			//	Log.d(TAG,"process="+ps[ps.length-1]);
				if(name.equals(ps[ps.length-1]) || ("sh" == ps[ps.length-1] && (rootUserName != null && rootUserName.equals(ps[0])))) {
				//	Log.d(TAG,"add");
					pids.add(ps[1]);
				}
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		psProcess.destroy();
		return pids;
	}
	
	private void killProcessByName(String name){
		List<String> pids;
		pids = getPidByProcessName(name);
		String str="";
		final int Count = pids.size();
		for (int i = 0; i < Count; i++) {
			String pid = pids.get(i);
			Log.d(TAG,"actionlogcat pid="+pid);
			str += "kill "+pid+";";
			//android.os.Process.killProcess(Integer.parseInt(pid));
		}
		Process process;
		try {
			process = Runtime.getRuntime().exec("su");
			DataOutputStream dos = new DataOutputStream(process.getOutputStream());
			dos.writeBytes(str);
			dos.writeBytes("exit\n");
			dos.flush();
			process.waitFor();
			process.destroy();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	
	public void startLogcatService(){
		if(logFile!=null){
			appendFileHeaderInfo(logFile);
		}
		if(serviceMode == SERVICE_MODE){
			Log.d(TAG,"set prop full="+logFile.getAbsolutePath());
			SystemProperties.set("sys.actionlogcat.fullfilename",logFile.getAbsolutePath());
			SystemProperties.set("sys.actionlogcat.timeformat","time");
			SystemProperties.set("sys.actionlogcat.maxsize", Integer.toString(getSuggestSize()));
			Log.d(TAG, "propget"+SystemProperties.get("sys.actionlogcat.fullfilename"));
			Log.d(TAG, "begin logstart service");
			SystemProperties.set("ctl.start", "actionslogcat");
		}
		else{
			
		}
	}
	
	public void stopLogcatService(){
		if(serviceMode == SERVICE_MODE){
			SystemProperties.set("ctl.stop", "actionslogcat");
		}
		else {
				killProcessByName(logcatBinLocation);
		}
		if(logFile!=null){
			appendFileEndInfo(logFile);
		}
	}

	public long getTotalMemSize(){
		ActivityManager am;
		ActivityManager.MemoryInfo outInfo = new ActivityManager.MemoryInfo();
		am = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);
		am.getMemoryInfo(outInfo);
		return (outInfo.totalMem>>10)>>10; //bytes convert to Mbytes
	}
		
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
	            Log.e("mydebug","IO Exception when getting kernel version",e);
	            return "Unavailable";
	        }
	    }
	
	public static String formatKernelVersion(String rawKernelVersion) {
		final String PROC_VERSION_REGEX =
				"Linux version (\\S+) " + /* group 1: "3.0.31-g6fb96c9" */
	            "\\((\\S+?)\\) " +        /* group 2: "x@y.com" (kernel builder) */
	            "(?:\\(gcc.+? \\)) " +    /* ignore: GCC version information */
	            "(#\\d+) " +              /* group 3: "#1" */
	            "(?:.*?)?" +              /* ignore: optional SMP, PREEMPT, and any CONFIG_FLAGS */
	            "((Sun|Mon|Tue|Wed|Thu|Fri|Sat).+)"; /* group 4: "Thu Jun 28 11:02:39 PDT 2012" */

		Matcher m = Pattern.compile(PROC_VERSION_REGEX).matcher(rawKernelVersion);
	    if (!m.matches()) {
	    	Log.e("mydebug", "Regex did not match on /proc/version: " + rawKernelVersion);
	    	return "Unavailable";
	    } else if (m.groupCount() < 4) {
	        Log.e("mydebug", "Regex match on /proc/version only returned " + m.groupCount()
	              + " groups");
	        return "Unavailable";
	    }
	    return m.group(1) + "\n" +                 // 3.0.31-g6fb96c9
	           m.group(2) + " " + m.group(3) + "\n" + // x@y.com #1
	           m.group(4);                            // Thu Jun 28 11:02:39 PDT 2012
	}

	public File creatLogFile(String filename)	{
		boolean ret = false;
		logFile = new File(LogDir + File.separator + filename +".log");
		if(logFile.exists())
			return logFile;
		
		try {
				ret=logFile.createNewFile();
			}
		catch (Exception e) {
					// TODO: handle exception
			e.printStackTrace();
		}
		if(ret)
			return logFile;
		
		return null;
	}

	private void appendFileHeaderInfo(File file)
	{
		String str;
		FileWriter fw = null;
		
		if(!file.canWrite())
			return;
		
		try {
			fw = new FileWriter(file);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return;
		}
		StringBuilder sb = new StringBuilder();
		str = mContext.getString(R.string.memorysize)+":"+getTotalMemSize()+"M\n";
		sb.append(str);
		str = mContext.getString(R.string.model)+":"+Build.MODEL+"\n";
		sb.append(str);
		str = mContext.getString(R.string.kernelversion)+":"+getFormattedKernelVersion()+"\n";
		sb.append(str);
		str = mContext.getString(R.string.buildversion)+":"+Build.DISPLAY+"\n";
		sb.append(str);
		str=("\n===========actionslogcat: start at " + getTimeNow() +"================\n");
		sb.append(str);	
	
		try {
			fw.write(sb.toString());
			fw.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}	
	}
	    
	private void appendFileEndInfo(File file){
	    FileWriter fw;
	    
	    if(!file.canWrite())
			return;
	    
		try {
			fw = new FileWriter(file, true);
			fw.append("\n===========actionslogcat: done at " + getTimeNow() +"================\n");
//			fw.flush();
			fw.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private String getTimeNow() {
		Date date = new Date(System.currentTimeMillis());
		SimpleDateFormat sdf = new SimpleDateFormat("yy-MM-dd hh:mm:ss");
		return sdf.format(date);
	}
		
	public String getDefaultName() {
	    Date date = new Date(System.currentTimeMillis());
	    SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd_kk-mm-ss");
	    String s = format.format(date);
	    return s;
	}
		
	private boolean canWrite(String path) {
	    File file = new File(path);
	    if(!file.exists()) 
	    	return false;
	    if(file.canWrite()) 
	    	return true;
		
	    return false;
	}
}
