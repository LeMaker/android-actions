package com.actions.userconfig;

import com.actions.utils.Utilities;

public class UserConfig {
	/**
	 * We support downloading firmware by both HTTP and FTP protocols.
	 * The only thing that you have to do is to modify this file followed:
	 * 
	 */
    
    /**
     * when use HTTP protocol
     */
	public static String mServerIP; //= android.os.SystemProperties.get("ro.ota.server", "http://**.**.***/");
	
	
	/**
	 * when FTP protocol
	 */
	public static String ftpHost = "";
	//public static String ftpHost = "58.254.217.101";
    public static int ftpPort = 21;
    public static String ftpUserName = "";
    public static String ftpPassword = "";
    public static String ftpRootDir = "";
    public static String remoteXmlPath = "";
}
