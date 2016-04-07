
package com.actions.utils;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.actions.actduinotest.R;

import android.content.Context;
import android.content.res.XmlResourceParser;
import android.os.Environment;
import android.util.Log;
import android.widget.Toast;

/**
 * Description:
 * 1.insmod and rmmod drivers
 * 2.read xml file 
 * 3.chmod 777  /dev/ttySx  i2c-x  spidevx.0
*********************************************   
 *ActionsCode(author:jiangjinzhang, new_code)
 * @version 1.0
 */
public class Utilities
{
	private static final String TAG = "ActduinoTest.Utilities";
    private static final boolean ACTDEBUG = true;
    private Context mContext; 
    
	public static String uart_port;
	public static String i2c_port;
	public static String spi_port;
	public static String adc_port;
	public static String pwm_port;
	public static ArrayList<String> gpio_pin;

	
	public Utilities(Context cxt) {
        this.mContext = cxt;
    }
	
	public boolean readConfigXml() {
		gpio_pin=new ArrayList<String>();
		XmlResourceParser xrp = this.mContext.getResources()
				.getXml(R.xml.actduinotest_config);
		if (xrp == null) {
			Log.w(TAG,"cannt find actduinotest_comfig.xml");
			return false;
		}
		try {
			while (xrp.getEventType() != XmlResourceParser.END_DOCUMENT) {
				if (xrp.getEventType() == XmlResourceParser.START_TAG) {
					String tagName = xrp.getName();
					if ("gpio".equals(tagName)) {
						String out =xrp.getAttributeValue(null,"out");
						gpio_pin.add(out);
						String in =xrp.getAttributeValue(null,"in");
						gpio_pin.add(in);
					} else if ("uart".equals(tagName)) {
						uart_port = xrp.nextText();
					} else if ("spi".equals(tagName)) {
						spi_port = xrp.nextText();
					} else if ("i2c".equals(tagName)) {
						i2c_port = xrp.nextText();
					} else if ("adc".equals(tagName)) {
						adc_port = xrp.nextText();
					}else if ("pwm".equals(tagName)) {
						pwm_port = xrp.nextText();
					}
				}
				xrp.next();
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
		return true;
    }
	//insmod drivers and change device file properties
	public boolean chmoddevices(){
		DataOutputStream localDataOutputStream;
		try {
			Process localProcess = Runtime.getRuntime().exec("su");
			OutputStream localOutputStream = localProcess.getOutputStream();
			localDataOutputStream = new DataOutputStream(localOutputStream);
			

			File mFile = new File("/dev/");
			File[] subFiles = mFile.listFiles();
			if (subFiles != null && subFiles.length > 0) {
				for (File f : subFiles) {
					String fileName = f.getName();
					// chmod 777 ttySx which exsits in catalogue /dev
					if (fileName.startsWith("ttyS")) {
						Log.d(TAG, "chmod 777  /dev/" + fileName + " \n");
						localDataOutputStream.writeBytes("chmod 777  /dev/"
								+ fileName + " \n");
						localDataOutputStream.flush();
					}
					// chmod 777 i2c-x which exsits in catalogue /dev
					if (fileName.startsWith("i2c-")) {
						Log.d(TAG, "chmod 777  /dev/" + fileName + " \n");
						localDataOutputStream.writeBytes("chmod 777  /dev/"
								+ fileName + " \n");
						localDataOutputStream.flush();
					}
					// chmod 777 spidevx.0 which exsits in catalogue /dev
					if (fileName.startsWith("spidev")) {
						Log.d(TAG, "chmod 777  /dev/" + fileName + " \n");
						localDataOutputStream.writeBytes("chmod 777  /dev/"
								+ fileName + " \n");
						localDataOutputStream.flush();
					}
					if (fileName.endsWith("owl_test")) {
						Log.d(TAG, "chmod 777  /dev/" + fileName + " \n");
						localDataOutputStream.writeBytes("chmod 777  /dev/"
								+ fileName + " \n");
						localDataOutputStream.flush();
					}
					
				}
			}

			localDataOutputStream.writeBytes("exit\n");// notice if there is no
														// "exit" the waitFor()
														// will wait forever
			localDataOutputStream.flush();
			localProcess.waitFor();
			Log.d(TAG, "excuting insmod down");
			localDataOutputStream.close();
		} catch (IOException e1) {			
			e1.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		return true;
	}
	
	
}
