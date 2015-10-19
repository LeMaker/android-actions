/*
 * Copyright (C) 2014 The Android Open Source Project
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
 */

package com.android.example.testplugin;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.DmtException;
import com.android.omadm.plugin.IDMClientService;

import java.util.Arrays;
import java.util.Map;

public class GetDMTreeActivity extends Activity {
    static final String TAG = "GetDMTreeActivity";

    // Binder interface to client service
    IDMClientService mDMClientService;

    // Service connector object to attach/detach binder interface
    private final DMClientServiceConnector mDMServiceConnector = new DMClientServiceConnector();

    private class DMClientServiceConnector implements ServiceConnection {
        // package local constructor (called by outer class)
        DMClientServiceConnector() {}

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(TAG, "onServiceConnected for " + name + " service " + service);
            mDMClientService = IDMClientService.Stub.asInterface(service);

            try {
                Log.d(TAG, "getting DM tree and dumping to log");

                DmtData swVersion = mDMClientService.getDMTree("./DevDetail/SwV", false);
                Log.d(TAG, "node \"./DevDetail/SwV\" type: " + swVersion.getType() + " value: "
                        + swVersion.getString());

                DmtData treeRoot = mDMClientService.getDMTree(".", true);
                Log.d(TAG, "tree root: type " + treeRoot.getType() + ": " + treeRoot.getString());
                if (treeRoot.getType() == DmtData.NODE) {
                    printChildren(treeRoot, "    ");
                }

                DmtData wifiNode = mDMClientService.getDMTree("./Wi-Fi", true);
                Log.d(TAG, "HotSpot wifi node root is " + wifiNode);
                if (wifiNode.getType() == DmtData.NODE) {
                    printChildren(wifiNode, "    ");
                }
            } catch (RemoteException e) {
                Log.e(TAG, "remote exception", e);
            }

            Log.d(TAG, "finishing activity");
            finish();
        }

        private void printChildren(DmtData node, String indent) {
            try {
                Map<String, DmtData> childNodeMap = node.getChildNodeMap();
                for (Map.Entry<String, DmtData> entry : childNodeMap.entrySet()) {
                    DmtData value = entry.getValue();
                    Log.d(TAG, indent + " name: " + entry.getKey() + " type: " + value.getType()
                            + ": " + value.getString());
                    if (value.getType() == DmtData.NODE) {
                        printChildren(value, indent + "    ");
                    }
                }
            } catch (DmtException e) {
                Log.e(TAG, "DmtException", e);
            }
        }

        @Override
        public void onServiceDisconnected(ComponentName name) {
            Log.d(TAG, "onServiceDisconnected for " + name);
            mDMClientService = null;
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.get_dm_tree_activity);
    }

    @Override
    protected void onResume() {
        super.onResume();
        // bind to the OMA DM client service
        Intent intent = new Intent();
        intent.setClassName("com.android.omadm.service", "com.android.omadm.service.DMClientService");
        bindService(intent, mDMServiceConnector,
                Context.BIND_AUTO_CREATE | Context.BIND_IMPORTANT);
    }

    @Override
    protected void onPause() {
        // unbind the OMA DM client service
        unbindService(mDMServiceConnector);
        super.onPause();
    }
}
