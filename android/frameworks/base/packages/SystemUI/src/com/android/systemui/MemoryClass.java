
package com.android.systemui;
import android.os.SystemProperties;
import android.util.Log;


public class MemoryClass {
	private static int MemorySizeMB =-1;
	public static boolean isExtraLowMemDevice() {
	
		if(MemorySizeMB <0){
				//value system.ram.total is autodeteced int system/core/init
				MemorySizeMB=SystemProperties.getInt("system.ram.total", 0);
		 }
		 if(MemorySizeMB==0){
				Log.d("MemoryClass", "property not set , something wrong, force to 512M bytes");
				new Exception().printStackTrace();
				MemorySizeMB=512;
		 }
			Log.d("MemoryClass",  "memory is " + MemorySizeMB +"  Mbytes");
	    return MemorySizeMB<=256;
	}

}