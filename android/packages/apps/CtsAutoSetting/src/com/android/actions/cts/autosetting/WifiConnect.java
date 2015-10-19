package com.android.actions.cts.autosetting;

import java.util.List;  

import android.net.wifi.WifiConfiguration;  
import android.net.wifi.WifiManager;  
import android.util.Log; 

public class WifiConnect {

	private final String TAG = "ctsAutoSetting";
	
	private WifiManager.ActionListener mConnectListener;
	
    WifiManager wifiManager;
        
	//WifiCipherType:WEP,WPA,noPassword
	public enum WifiCipherType
	{
	 WIFICIPHER_WEP,WIFICIPHER_WPA, WIFICIPHER_NOPASS, WIFICIPHER_INVALID
	}
		
    public WifiConnect(WifiManager wifiManager)
    {   
    	Log.i(TAG, "WifiConnect..." );
        this.wifiManager = wifiManager;
        mConnectListener = new WifiManager.ActionListener() {
            @Override
            public void onSuccess() {
            	Log.i(TAG, "wifi connect successfully..." );
            }
            @Override
            public void onFailure(int reason) {
            	Log.e(TAG, "wifi failed to connect..." );
            }
        };
	}
		
	//OpenWifi
    private boolean OpenWifi()
    {
	    boolean bRet = true;
	    if (!wifiManager.isWifiEnabled())
	    {
	        bRet = wifiManager.setWifiEnabled(true);  
	    }
	    return bRet;
	}
	    
  //connect wifi
	public boolean Connect(String SSID, String Password, WifiCipherType Type)
	{
		Log.i(TAG, "Connect..." );
	    if(!this.OpenWifi())
	    {
	    	Log.e(TAG, "OpenWifi failed..." );
	        return false;
	    }
	    Log.i(TAG, "wifiState:" +wifiManager.getWifiState());
	    //wait until the wifi state is WIFI_STATE_ENABLED(enable wifi will cost 0--3s)
	    //while(wifiManager.getWifiState() == WifiManager.WIFI_STATE_ENABLING )
	    while(wifiManager.getWifiState() != WifiManager.WIFI_STATE_ENABLED)
	    {
	    	Log.i(TAG, "wifiState:WIFI_STATE_ENABLING" );
	        try{
	        //sleep 200ms,and then detect
	            Thread.currentThread();
				Thread.sleep(200);
	        }
	        catch(InterruptedException ie){
	        }
	    }
	    Log.i(TAG, "wifiState:" +wifiManager.getWifiState());  
	    WifiConfiguration wifiConfig = this.CreateWifiInfo(SSID, Password, Type);
		//
	    if(wifiConfig == null)
		{
	        return false;
		}
	    //Log.i(TAG, "wifiConfig.preSharedKey:" +wifiConfig.preSharedKey);
		   	
	    WifiConfiguration tempConfig = this.IsExsits(SSID);
	        
	    if(tempConfig != null)
	    {
	        wifiManager.removeNetwork(tempConfig.networkId);
	    }
	    boolean bRet =true;
	    //int netID = wifiManager.addNetwork(wifiConfig);
	    //boolean bRet = wifiManager.enableNetwork(netID, false);  
	    wifiManager.connect(wifiConfig, mConnectListener);
		return bRet;
    }
	     
	//wifi_neiwork IsExsits or not
	private WifiConfiguration IsExsits(String SSID)
	{
	    List<WifiConfiguration> existingConfigs = wifiManager.getConfiguredNetworks();
	    for (WifiConfiguration existingConfig : existingConfigs) 
	    {
	    	Log.i(TAG, "existingConfig.SSID:" +existingConfig.SSID);  
	        if (existingConfig.SSID.equals("\""+SSID+"\""))
	    	{
	    	    return existingConfig;
	    	}
	    }
	    return null; 
	}
	     
	private WifiConfiguration CreateWifiInfo(String SSID, String Password, WifiCipherType Type)
	{
	    WifiConfiguration config = new WifiConfiguration();  
	    config.allowedAuthAlgorithms.clear();
	    config.allowedGroupCiphers.clear();
	    config.allowedKeyManagement.clear();
	    config.allowedPairwiseCiphers.clear();
	    config.allowedProtocols.clear();
	    config.SSID = "\"" + SSID + "\"";  
     	if(Type == WifiCipherType.WIFICIPHER_NOPASS)
     	{
     		 config.wepKeys[0] = "";
     		 config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
     		 config.wepTxKeyIndex = 0;
     	}
     	if(Type == WifiCipherType.WIFICIPHER_WEP)
     	{
     		config.preSharedKey = "\""+Password+"\""; 
     		config.hiddenSSID = true;  
     	    config.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.SHARED);
     	    config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.CCMP);
     	    config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);
     	    config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.WEP40);
     	    config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.WEP104);
     	    config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.NONE);
     	    config.wepTxKeyIndex = 0;
     	}
     	if(Type == WifiCipherType.WIFICIPHER_WPA)
     	{
     	config.preSharedKey = "\""+Password+"\"";
 		config.priority = 1;
     	//config.hiddenSSID = true;  
     	//config.allowedAuthAlgorithms.set(WifiConfiguration.AuthAlgorithm.OPEN);  
     	//config.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);                        
     	config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);                        
     	//config.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.TKIP);                   
     	//config.allowedProtocols.set(WifiConfiguration.Protocol.WPA);                     
     	config.status = WifiConfiguration.Status.ENABLED;  
     	}
     	else
     	{
     		return null;
     	}
     	return config;
     }	
}
