
package com.actions.explore;

import java.io.File;
import java.util.ArrayList;

import android.content.Context;
import android.os.Environment;
import android.os.storage.StorageManager;
//import android.util.Log;
/*
 * @author lizihao@actions-semi.com
 */
public class DevicePath{
	
	private ArrayList<String> totalDevicesList;
	private static final String TAG = DevicePath.class.getSimpleName();
	
	public DevicePath(Context context)
	{
		totalDevicesList = new ArrayList<String>();
		StorageManager stmg = (StorageManager) context.getSystemService(context.STORAGE_SERVICE);
		String[] list = stmg.getVolumePaths();
		for(int i = 0; i < list.length; i++)
		{
			totalDevicesList.add(list[i]);
		}
	}
	
	public String getSdStoragePath()
	{
		String path = "none";
		for(int i = 0; i < totalDevicesList.size(); i++)
		{
			if(!totalDevicesList.get(i).equals(Environment.getExternalStorageDirectory().getPath()))
			{
				if(totalDevicesList.get(i).contains("sd"))
				{
					path = totalDevicesList.get(i);
					return path;
				}
			}
		}
		return path;
	}
	
	public String getInterStoragePath()
	{
		return Environment.getExternalStorageDirectory().getPath();
	}
	
	public String getUsbStoragePath()
	{
		String path = "none";
		for(int i = 0; i < totalDevicesList.size(); i++)
		{
			if(!totalDevicesList.get(i).equals(Environment.getExternalStorageDirectory().getPath()))
			{
				if(totalDevicesList.get(i).contains("host"))
				{
					path = totalDevicesList.get(i);
					return path;
				}
			}
		}
		return path;
	}
	
	public boolean hasMultiplePartition(String dPath)
	{
		try
		{
			File file = new File(dPath);
			String minor = null;
			String major = null;
			for(int i = 0; i < totalDevicesList.size(); i++)
			{
				if(dPath.equals(totalDevicesList.get(i)))
				{
					String[] list = file.list();
					for(int j = 0; j < list.length; j++)
					{
						int lst = list[j].lastIndexOf("_");
						if(lst != -1 && lst != (list[j].length() -1))
						{
							major = list[j].substring(0, lst);
							minor = list[j].substring(lst + 1, list[j].length());
							try
							{
							
								Integer.valueOf(major);
								Integer.valueOf(minor);
							}
							catch(NumberFormatException e)
							{
								return false;
							}
						}
						else 
						{
							return false;
						}
					}
					return true;
				}
			}
			return false;
		}
		catch(Exception e)
		{
			//Log.e(TAG, "hasMultiplePartition() exception e");
			return false;
		}
	}
}


