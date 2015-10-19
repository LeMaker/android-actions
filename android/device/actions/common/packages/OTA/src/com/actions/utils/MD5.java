package com.actions.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class MD5 {
	
	private static final char HEX_DIGITS[] = {'0','1','2','3','4',
		'5','6','7','8','9','a','b','c','d','e','f'};
	
	// check md5 message
	public static boolean checkMD5(String mMD5Msg, String mPathFile){
		String msg = createMD5(mPathFile);
		System.out.println("---->create MD5 number is: " + msg);
		if((msg != null) && (mMD5Msg.equalsIgnoreCase(msg))){
			return true;
		}
		return false;
	}
	
	public static String toHexString(byte[] b) {
        StringBuilder sb = new StringBuilder(b.length * 2);
        for (int i = 0; i < b.length; i++) {
            sb.append(HEX_DIGITS[(b[i] & 0xf0) >>> 4]);
            sb.append(HEX_DIGITS[b[i] & 0x0f]);
        }
        return sb.toString();
    }
	
	// create md5 message from the /mnt/sdcard/update.zip(mPathFile)
	private static String createMD5(String mPathFile){
		File mFile = new File(mPathFile);
		if(!mFile.exists()) {
		    Debug.e("File not found:" + mPathFile);
		    return null;
		}
		FileInputStream fis;
        byte[] buffer = new byte[1024000];
        int numRead = 0;
        MessageDigest md5;
        
        try {
			fis = new FileInputStream(mFile);
			md5 = MessageDigest.getInstance("MD5");
			while((numRead=fis.read(buffer)) > 0) {
                md5.update(buffer,0,numRead);
            }
			fis.close();
            return toHexString(md5.digest()); 
		} catch (IOException e) {
			System.out.println("check md5 fail");
			e.printStackTrace();
		} catch (NoSuchAlgorithmException e) {
			System.out.println("check md5 fail");
			e.printStackTrace();
		}
        return null;
	}
	
}
