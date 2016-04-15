/*
 * Copyright (C) 2010 The Android-x86 Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Yi Sun <beyounn@gmail.com>
 */

package com.android.settings.ethernet;


import java.util.List;

import com.android.settings.R;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.NetworkInfo;
import android.net.EthernetManager;
import android.net.IpConfiguration;
import android.net.StaticIpConfiguration;
import android.net.LinkAddress;
import android.net.NetworkUtils;
import android.preference.Preference;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Slog;
import android.util.Log;
import android.os.SystemProperties;
import java.util.ArrayList;
import java.net.InetAddress;
import java.net.UnknownHostException;


public class EthernetConfigDialog extends AlertDialog implements
        DialogInterface.OnClickListener, AdapterView.OnItemSelectedListener, View.OnClickListener {
    private final String TAG = "EthernetConfigDialog";
    private final int ERROR_EMP_IP =1;
    private final int ERROR_EMP_MASK =2;
    private final int ERROR_EMP_GW =3; 
    private final int ERROR_EMP_DNS =4;

    private final int ADDR_LENGTH_MAX =16;    //xxx.xxx.xxx.xxx 
    private final int ADDR_LENGTH_MIN =7;     //x.x.x.x
                   
    private View mView;
    private RadioButton mConTypeDhcp;
    private RadioButton mConTypeManual;
    private EditText mIpaddr;
    private EditText mDns;
    private EditText mGw;
    private EditText mMask;
    
    
    private boolean mEdit;
    private Context mcontext = null;
    private final DialogInterface.OnClickListener mListener;

    private EthernetManager mEthManager;
    //private EthernetDevInfo mEthInfo;
    private boolean mEnablePending;
	
	SharedPreferences mPreferences;
	SharedPreferences.Editor mEditor;


    public EthernetConfigDialog(Context context, DialogInterface.OnClickListener listener,
             boolean edit) {
        super(context);
        mEdit = edit;
        mListener = listener;
        mcontext=context;
	    mEthManager = (EthernetManager) context.getSystemService(Context.ETHERNET_SERVICE);
		mPreferences = context.getSharedPreferences("ethernetConfig", Context.MODE_PRIVATE);
		mEditor = mPreferences.edit();
        buildDialogContent(context);
        setButton(DialogInterface.BUTTON_POSITIVE,context.getString(R.string.wifi_connect),listener);
        setButton(DialogInterface.BUTTON_NEGATIVE,context.getString(R.string.wifi_cancel),listener);
        //hide keyboard 
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
        Log.d(TAG,"EthernetConfigDialog construct ending");
    }

    public int buildDialogContent(Context context) {
        this.setTitle(R.string.ethernet_conf_title);
        this.setView(mView = getLayoutInflater().inflate(R.layout.ethernet_configure, null));
        mConTypeDhcp = (RadioButton) mView.findViewById(R.id.dhcp_radio);
        mConTypeManual = (RadioButton) mView.findViewById(R.id.manual_radio);
        mIpaddr = (EditText)mView.findViewById(R.id.ipaddr_edit);
        mMask = (EditText)mView.findViewById(R.id.netmask_edit);
        mDns = (EditText)mView.findViewById(R.id.eth_dns_edit);
        mGw = (EditText)mView.findViewById(R.id.eth_gw_edit);

        mConTypeDhcp.setChecked(true);
        mConTypeManual.setChecked(false);
        setStaticIpInfoState(false);
        mConTypeManual.setOnClickListener(new RadioButton.OnClickListener() {
            public void onClick(View v) {
				mEditor.putString("mode", "static");
				mEditor.commit();
				setStaticIpInfoState(true);
            }
        });

        mConTypeDhcp.setOnClickListener(new RadioButton.OnClickListener() {
            public void onClick(View v) {
				mEditor.putString("mode", "dhcp");
				mEditor.commit();
				setStaticIpInfoState(false);
            }
        });

        this.setInverseBackgroundForced(true);
            

        String mode = mPreferences.getString("mode", "dhcp");
		

        if (mode.equals("dhcp")) {
           setStaticIpInfoState(false);
//           setContent();
        } else if(mode.equals("static")) {
            mConTypeDhcp.setChecked(false);
            mConTypeManual.setChecked(true);
            setStaticIpInfoState(true);
//				    setContent();
        }
			setContent();
        return 0;
    }


  /*public void refresh(){
   Log.d(TAG,"refresh()");
   if (mEthManager.hasSavedConf()) {
    mEthInfo = mEthManager.getSavedConfig();
    if(mEthInfo.getConnectMode().equals(EthernetDevInfo.ETHERNET_CONN_MODE_DHCP)) {
        setStaticIpInfoState(false);
        setContent();
    }
   }
    }*/

  public void onClick(DialogInterface dialogInterface, int button) {
  }

    public void onNothingSelected(android.widget.AdapterView av) {
     Log.d(TAG,"onNothingSelected");
    }
    public void onItemSelected(android.widget.AdapterView<?> av,android.view.View v,int i,long l){
     Log.d(TAG,"onItemSelected");
    }
    public void onClick(android.view.View v){
    }


    private void setStaticIpInfoState(boolean enable){
     mIpaddr.setEnabled(enable);
     mDns.setEnabled(enable);
     mGw.setEnabled(enable);
     mMask.setEnabled(enable);
    }
	
	
	public int setIpConfiguration() {
		
		if(mConTypeDhcp.isChecked()) {			
			IpConfiguration.IpAssignment ipAssignment =IpConfiguration.IpAssignment.DHCP;
            IpConfiguration.ProxySettings proxySettings =IpConfiguration.ProxySettings.NONE;
			IpConfiguration ipConfig = new IpConfiguration(ipAssignment, proxySettings, null, null);
			
			mEthManager.setConfiguration(ipConfig);
			return 1;
	    }else{	        
			String ipAddr = mIpaddr.getText().toString();
			String routeAddr = mGw.getText().toString();
			String dnsAddr = mDns.getText().toString();
			String maskAddr = mMask.getText().toString();
			
			if(ipAddr==null||(ipAddr.length()<ADDR_LENGTH_MIN)||(ipAddr.length()>=ADDR_LENGTH_MAX)){
				Toast.makeText(mcontext,R.string.eth_ipaddr_err_summary, Toast.LENGTH_SHORT).show();	
				return ERROR_EMP_IP;
			}else if(routeAddr==null||(routeAddr.length()<ADDR_LENGTH_MIN)||(routeAddr.length()>=ADDR_LENGTH_MAX)){	
				Toast.makeText(mcontext,R.string.eth_gw_err_summary, Toast.LENGTH_SHORT).show();	
		  					return ERROR_EMP_GW;
		  }else if(dnsAddr==null||(dnsAddr.length()<ADDR_LENGTH_MIN)||(dnsAddr.length()>=ADDR_LENGTH_MAX)){
				Toast.makeText(mcontext,R.string.eth_dns_err_summary, Toast.LENGTH_SHORT).show();	
		  		return ERROR_EMP_DNS;
		  }else if(maskAddr==null||(maskAddr.length()<ADDR_LENGTH_MIN)||(maskAddr.length()>=ADDR_LENGTH_MAX)){
				Toast.makeText(mcontext,R.string.eth_mask_err_summary, Toast.LENGTH_SHORT).show();	
						  		return ERROR_EMP_MASK;
		  }
			
			StaticIpConfiguration staticIpConfiguration = new StaticIpConfiguration();
			
			int validMaskCount = netmaskIntToPrefixLength(maskAddr);
			LinkAddress linkAddr = new LinkAddress(
                                   NetworkUtils.numericToInetAddress(ipAddr), validMaskCount);
            staticIpConfiguration.ipAddress= linkAddr;
			
			InetAddress gateway = NetworkUtils.numericToInetAddress(routeAddr);
            staticIpConfiguration.gateway= gateway;
			
			staticIpConfiguration.dnsServers.add(NetworkUtils.numericToInetAddress(dnsAddr));
			
			IpConfiguration.IpAssignment ipAssignment =IpConfiguration.IpAssignment.STATIC;
            IpConfiguration.ProxySettings proxySettings =IpConfiguration.ProxySettings.NONE;
			
			IpConfiguration ipConfig = new IpConfiguration(ipAssignment, proxySettings, staticIpConfiguration, null);
			
			mEthManager.setConfiguration(ipConfig);
			
			saveConfiguration(ipAddr, routeAddr, dnsAddr, maskAddr);
			return 2;
	    }
	    //return 0;
	}
	
	 private int netmaskIntToPrefixLength(String netmask) {
		int allMaksCount = 0;
		String [] maskArray = netmask.split("\\.");
		for (int i = 0; i < maskArray.length; i++) {
    		int perCount = Integer.parseInt(maskArray[i]);
    		int perMaskCount = Integer.bitCount(perCount);
			allMaksCount += perMaskCount;
    	}
        return allMaksCount;
    }

    private void setContent(){
	 String ipAddr = mPreferences.getString("ipAddr", null);
	 String routeAddr = mPreferences.getString("routeAddr", null);
	 String dnsAddr = mPreferences.getString("dnsAddr", null);
	 String maskAddr = mPreferences.getString("maskAddr", null);
//	 Log.d(TAG,"dhb EthernetConfigDialog mode:"+mode+"  ipAddr:"+ipAddr+"  route:"+routeAddr+"  dns:"+dnsAddr+"  mask:"+maskAddr);
	 mMask.setText(maskAddr);	
   mDns.setText(dnsAddr);	    	
   mGw.setText(routeAddr);
 	 mIpaddr.setText(ipAddr);  
/*
	 if (mode.equals("static")) {
	    mMask.setText(maskAddr);	
        mDns.setText(dnsAddr);	    	
        mGw.setText(routeAddr);
 	    	mIpaddr.setText(ipAddr);       
	 } else if(mode.equals("dhcp"))
        mIpaddr.setText("");
        mGw.setText("");
        mDns.setText("");
	    mMask.setText("");
*/	    
    }
	
	private void saveConfiguration(String ipAddr, String routeAddr, String dnsAddr, String maskAddr) {
		mEditor.putString("ipAddr", ipAddr);
		mEditor.putString("routeAddr", routeAddr);
		mEditor.putString("dnsAddr", dnsAddr);
		mEditor.putString("maskAddr", maskAddr);
		mEditor.commit();
	}

    /*public void enableAfterConfig() {
        mEnablePending = true;
    }

    public EthernetDevInfo getConf(){
        EthernetDevInfo info = new EthernetDevInfo();
        info.setIfName("eth0");
        if(mConTypeDhcp.isChecked()) {
         info.setConnectMode(EthernetDevInfo.ETHERNET_CONN_MODE_DHCP);
         info.setIpAddress("");
         info.setRouteAddr("");
         info.setDnsAddr("");
         info.setNetMask("");
        }else{
         info.setConnectMode(EthernetDevInfo.ETHERNET_CONN_MODE_MANUAL);
         info.setIpAddress(mIpaddr.getText().toString());
         info.setRouteAddr(mGw.getText().toString());
         info.setDnsAddr(mDns.getText().toString());
         info.setNetMask(mMask.getText().toString());
        }
        return info;
    }*/
}
