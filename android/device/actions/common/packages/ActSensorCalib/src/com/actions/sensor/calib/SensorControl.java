package com.actions.sensor.calib;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;

import android.content.Context;
import android.util.Log;

public class SensorControl {

	public final String TAG = "SensorControl";
	
	private final String SENSORNAME = "bma220,bma222,bma223, bma250, mir3da, mma7660,mma8452,dmard06,dmard10,bma250t"
									+ "mc3210,mc3230,mc3236, mc6420,bmc050,lis3dh,lis3dh_acc,kionix_accel,"
									+ "stk8312,stk8313,l3g4200d,mpu3050c,mpu6515";
	
	private final String CALIBFILE = "gsensor_calib.txt";
	private final String INPUT_DIR = "/sys/class/input";
	private final String IIO_DIR = "/sys/bus/iio/devices";

	public final String CALIB_RESET_ATTR = "calibration_reset";
	public final String CALIB_RUN_ATTR = "calibration_run";
	public final String CALIB_VAL_ATTR = "calibration_value";
	public final String SENSOR_NAME_ATTR = "name";

	private String classPath = null;
	private String dataPath = null;
	
	
	public final int CALI_TYPE_INPUT = 1;
	public final int CALI_TYPE_IIO = 2; 
	public int calibration_type = 0;

	public SensorControl(Context ctx) {
		dataPath = ctx.getFilesDir().getAbsolutePath();
		getClassPath();
	}
	
	public void resetCalib() {
		String path = getDevPath(CALIB_RESET_ATTR);
		writeFile(path, "1");
	}

	public void runCalib() {
		String path = getDevPath(CALIB_RUN_ATTR);
		writeFile(path, "1");
	}

	public String getCalibValue() {
		String path = getDevPath(CALIB_VAL_ATTR);
		String data = readFile(path);
		return data.trim();
	}
	
	public void setCalibValue(String value) {
		String path = getDevPath(CALIB_RUN_ATTR);
		writeFile(path, value);
	}
	
	public String readCalibFile() {
		String path = getDataPath(CALIBFILE);
		return readFile(path);
	}

	public void writeCalibFile(String calib) {
		String path = getDataPath(CALIBFILE);
		writeFile(path, calib);
	}

	
	private String getClassPath() {
		String name = null;
		// get sensor class path
		if (classPath == null) {
			File file = new File(INPUT_DIR);
			File[] files = file.listFiles();
			
			for(int i = 0; i < files.length; i++){
				if(files[i].isDirectory() && files[i].getName().contains("input")){
					name = readFile(files[i].getAbsolutePath() + "/name");
					if((name != null) && SENSORNAME.contains(name.trim())){
						classPath = files[i].getAbsolutePath();
						Log.i(TAG, "classPath: " + classPath);
						calibration_type = CALI_TYPE_INPUT;
						//break;
						return classPath;
					}
				}
			}
			
			File file_iio = new File(IIO_DIR);
			File[] iio_files = file_iio.listFiles();
			
			for(int i = 0; i < iio_files.length; i++){
				if(iio_files[i].isDirectory() && iio_files[i].getName().contains("iio:device")){
					name = readFile(iio_files[i].getAbsolutePath() + "/name");
					if((name != null) && SENSORNAME.contains(name.trim())){
						classPath = iio_files[i].getAbsolutePath();
						Log.i(TAG, "classPath: " + classPath);
						calibration_type = CALI_TYPE_IIO;
						break;
					}
				}
			}	
		}
		
		return classPath;
	}
	
	private String getDevPath(String name) {
		return getClassPath() + "/" + name;
	}

	private String getDataPath(String name) {
		return dataPath + "/" + name;
	}
	
	private String readFile(String path) {
		byte[] buffer = null;
		try {
			FileInputStream in = new FileInputStream(path);
			buffer = new byte[in.available()];
			in.read(buffer);
			in.close();
		} catch (Exception e) {
			Log.e(TAG, "read " + path +" error!");
			e.printStackTrace();
		}

		return new String(buffer);
	}

	private void writeFile(String path, String content) {
		try {
			FileOutputStream out = new FileOutputStream(path);
			out.write(content.getBytes());
			out.close();
		} catch (Exception e) {
			Log.e(TAG, "write " + path +" error!");
			e.printStackTrace();
		}
	}
}
