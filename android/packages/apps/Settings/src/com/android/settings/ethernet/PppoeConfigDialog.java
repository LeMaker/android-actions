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

import com.android.settings.R;

import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.preference.Preference;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;
import android.util.Slog;
import android.util.Log;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.List;

public class PppoeConfigDialog extends AlertDialog implements
        DialogInterface.OnClickListener, AdapterView.OnItemSelectedListener, View.OnClickListener {
    private final String TAG = "PppoeConfigDialog";

    private static final String STRPPPOE1 = "pppd pty \"pppoe -I ";
    private static final String STRPPPOE2 = " -T 80 -U -m 1412\" noipdefault noauth default-asyncmap defaultroute hide-password nodetach usepeerdns mtu 1006 mru 1006 noaccomp nodeflate nopcomp novj novjccomp user ";
    private static final String STRPPPOE3 = " lcp-echo-interval 20 lcp-echo-failure 3 ";

    private View mView;
    private EditText mUsername;
    private EditText mPassword;

    
    private boolean mEdit;
    private final DialogInterface.OnClickListener mListener;


    public PppoeConfigDialog(Context context, DialogInterface.OnClickListener listener,
            boolean edit) {
        super(context);
        mEdit = edit;
        mListener = listener;
        buildDialogContent(context);
        setButton(DialogInterface.BUTTON_POSITIVE,context.getString(R.string.wifi_connect),listener);
        setButton(DialogInterface.BUTTON_NEGATIVE,context.getString(R.string.wifi_cancel),listener);
        //hide keyboard 
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
        Log.d(TAG,"EthernetConfigDialog construct ending");
    }

    public int buildDialogContent(Context context) {
        this.setTitle(R.string.pppoe_conf_title);
        this.setView(mView = getLayoutInflater().inflate(R.layout.pppoe_configure, null));


        mUsername = (EditText)mView.findViewById(R.id.username_edit);
        mPassword = (EditText)mView.findViewById(R.id.password_edit);        

        this.setInverseBackgroundForced(true);
	readConfig();

        return 0;
    }

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



	public boolean saveConfig() {
		String name = mUsername.getText().toString();
		String passwd = mPassword.getText().toString();

		if (name == null || name.trim().equals("")
			|| passwd == null || passwd.trim().equals("") ) {
			return false;
		}

	
		try {
			File lpap = new File("/data/data/com.android.settings/pap-secrets");
			if (lpap.exists()) {
				lpap.delete();
			}
			lpap.createNewFile();

			FileWriter lwriterpap = new FileWriter(lpap);
			lwriterpap.write( name + '\n' + passwd
					+ '\n');
			lwriterpap.close();

			File lfile = new File("/data/data/com.android.settings/ppp_eth0.sh");
			if (lfile.exists()) {
				lfile.delete();
			}
			lfile.createNewFile();
			lfile.setExecutable(true);
			FileWriter lwriter = new FileWriter(lfile);

			
			lwriter.write(STRPPPOE1 + "eth0" + STRPPPOE2 + name
					+ " password " + passwd + STRPPPOE3 + " interface "
					+ "eth0" + " & ");
			lwriter.close();

		} catch (Exception localException) {
			Slog.v(TAG, localException.toString());
			return false;
		}
		
		return true;

	}

	private void readConfig() {
		try {
			File lpap = new File("/data/data/com.android.settings/pap-secrets");
			if (!lpap.exists()) {
				return;
			}

			BufferedReader lbufReader = new BufferedReader(new FileReader(lpap));		
			String name = lbufReader.readLine();
			String passwd = lbufReader.readLine();
			mUsername.setText(name);
			mPassword.setText(passwd);
            lbufReader.close();

			Slog.i(TAG,  "Username:" + name + " Password:" + passwd);
		} catch (Exception localException) {
			Slog.v(TAG, localException.toString());
		}
	}



}
