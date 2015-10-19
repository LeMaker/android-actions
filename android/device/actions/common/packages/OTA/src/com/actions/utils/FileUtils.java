package com.actions.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class FileUtils {
	
	private static final String SDCardRoot = "/mnt/sdcard";
	
	public FileUtils(){
		// /mnt/sdcard
		//SDCardRoot = "/mnt/sdcard";
	}
	
	// create a file in sdcard
	public File createFileInSDCard(String fileName) throws IOException{
		File file = new File(SDCardRoot + File.separator + fileName);
		file.createNewFile();
		return file;
	}
	
	// create a dir in sdcard(this create in /mnt/sdcard)
	public File createSDdir(String dir){
		File dirFile = new File(SDCardRoot + File.separator + dir);
		return dirFile;
	}
	
	// whether the dir is exist in sdcard
	public boolean isFileExist(String fileName){
		File file = new File(SDCardRoot + File.separator + fileName);
		return file.exists();
	}
	
	// return the size of file
	public int getFileSize(String fileName){
		File file = new File(SDCardRoot + File.separator + fileName);
		FileInputStream fis = null;
		int mFileSize = 0;
		try {
			fis = new FileInputStream(file);
			mFileSize = fis.available();
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally {
		    if(fis != null) {
		        try {
                    fis.close();
                } catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
		    }
		}
		
		System.out.println("--->exist update.zip,size is: " + mFileSize);
		return mFileSize;
	}
	
	// delete file
	public void deleteFile(String fileName){
		File file = new File(SDCardRoot + File.separator + fileName);
		if(file.exists()){
			file.delete();
		}
	}
	
	// write the data of inputstream to sdcard
	/**
	 * @return -1 failed, 0 successful
	 */
	public int write2SDfromInputStream(String fileName,InputStream inputstream) {
		File file = null;
		int ret = -1;
		OutputStream outputStream = null;
		
		try {
			file = createFileInSDCard(fileName);
			outputStream = new FileOutputStream(file);
			byte buffer[] = new byte[1024 * 4];
			int temp;
			while((temp = inputstream.read(buffer)) != -1){
				outputStream.write(buffer,0,temp);
			}
			outputStream.flush();
			ret = 1;
		} catch (Exception e) {
			// TODO: handle exception
		    e.printStackTrace();
		    ret = -1;
		} finally{
			try {
				outputStream.close();
			} catch (Exception e2) {
				// TODO: handle exception
			    e2.printStackTrace();
			    ret = -1;
			}
		}
		return ret;
	}
}
