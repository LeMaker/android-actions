package com.actions.tests;


import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.util.HashMap;
import java.util.Map;

import android.util.Log;



public class Md5Class {

public static boolean checkDirMd5Valid(File dir, Map<String, String>maps, boolean listChild){
		boolean ret = true;
		File files[]=dir.listFiles();
		if(files == null) return true;
	    for(int i=0;i<files.length;i++){
	      File f=files[i];
	      if(f.isDirectory()&&listChild){
	         if( checkDirMd5Valid(f, maps, listChild) == false){
	        	 ret = false;
	         }
	      } else {
	         ret = checkFileMd5InMaps(f, maps);
	      }
	    }
	  return ret;
	}
	public static boolean checkFileMd5InMaps(File file, Map<String, String>maps){
		String calcMd5 = getFileMD5(file);
	
		String mapsMd5 = maps.get(file.getAbsolutePath());
		
		if (mapsMd5 ==null || calcMd5==null) {
			 //we have no permission, so just skip the check ....	
		    return true;
		}
		
		if(mapsMd5.equals(calcMd5)){
			//Log.v("Md5", "File="+file.getAbsolutePath()+":"+calcMd5+":"+mapsMd5);
			return true;
		}
		
		Log.e("Md5", "Error, checkMd5 fails: mapsMd5="+mapsMd5
				+", calcMd5="+calcMd5
				+", File="+file.getAbsolutePath());
		dumpFile(file);
		return false;
	}
	
	private static void dumpFile(File file){
		String filename = file.getName();
		Log.d("Md5", "Save to /sdcard/"+filename+".dump");
		File dest = new File("/sdcard/"+filename+".dump");
	
		try{
			FileInputStream in = new FileInputStream(file);
			FileOutputStream out =  new FileOutputStream(dest);
			
			int counts = in.available();
			byte[] contents = new byte[counts]; 
			
			in.read(contents);   
			out.write(contents);   

	        in.close();
	        out.flush(); 
	        out.close(); 

	        
		}catch (Exception e) {
		      e.printStackTrace();
		      return;
		}
	}
	
	public static String getFileMD5(File file) {
	    if (!file.isFile()){
	      return null;
	    }
	    MessageDigest digest = null;
	    FileInputStream in=null;
	    byte buffer[] = new byte[1024];
	    int len;
	    try {
	      digest = MessageDigest.getInstance("MD5");
	      in = new FileInputStream(file);
	      while ((len = in.read(buffer, 0, 1024)) != -1) {
	        digest.update(buffer, 0, len);
	      }
	      in.close();
	    } catch (Exception e) {
	      e.printStackTrace();
	      return null;
	    }
	    BigInteger bigInt = new BigInteger(1, digest.digest());
	    return bigInt.toString(16);
	  }
	  
	  /**
	   * 获取文件夹中文件的MD5值
	   * @param file
	   * @param listChild ;true递归子目录中的文件
	   * @return
	   */
	  public static Map<String, String> getDirMD5(File file,boolean listChild) {
	    if(!file.isDirectory()){
	      return null;
	    }
	    //<filepath,md5>
	    Map<String, String> map=new HashMap<String, String>();
	    String md5;
	    File files[]=file.listFiles();
	    if(files == null) return null;
	    for(int i=0;i<files.length;i++){
	      File f=files[i];
	      if(f.isDirectory()&&listChild){
	    	  Map<String, String> maps = getDirMD5(f, listChild);
	    	  if(maps != null){
	    		  map.putAll(maps);
	    	  }
	      } else {
	        md5=getFileMD5(f);
	        if(md5!=null){
	          map.put(f.getAbsolutePath(), md5);
	//          Log.d("Md5", "check md5   "+f.getAbsolutePath()+":"+md5);
	        }
	      }
	    }
	    return map;
	  }
}