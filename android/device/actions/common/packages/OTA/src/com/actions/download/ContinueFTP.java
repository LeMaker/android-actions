package com.actions.download;

import java.io.IOException;
import android.util.Log;
import java.net.SocketException;
import org.apache.commons.net.ftp.FTPClient;

public class ContinueFTP {
	
	public FTPClient mFtpClient = new FTPClient();
	
	public boolean connect(String host, int port, String username, String password){
		try {
			Log.i("tag", "1111111111111111111") ;
			mFtpClient.connect(host, port);
			Log.i("tag", "2222222222222222222") ;
			mFtpClient.setControlEncoding("GBK");
			if((mFtpClient.login(username, password))){
				return true;
			} else{
				disconnect();
				return false;
			}
		} catch (SocketException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return false;
	}
	
	public void disconnect() throws IOException{
		if(mFtpClient.isConnected()){
			mFtpClient.disconnect();
		}
	}
	
}
