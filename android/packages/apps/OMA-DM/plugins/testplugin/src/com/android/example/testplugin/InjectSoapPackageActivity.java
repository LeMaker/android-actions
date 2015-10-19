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
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.IDMClientService;

public class InjectSoapPackageActivity extends Activity {
    static final String TAG = "InjectSoapPackageActivity";

    // example serialized MO from SOAP-XML SPP package
    private static final String SAMPLE_PAYLOAD
            = "<MgmtTree xmlns=\"syncml:dmddf1.2\">\n"
            + "    <VerDTD>1.2</VerDTD>\n"
            + "    <Node>\n"
            + "        <NodeName>PerProviderSubscription</NodeName>\n"
            + "        <RTProperties>\n"
            + "            <Type>\n"
            + "                <DDFName>urn:wfa:mo:hotspot2dot0-perprovidersubscription:1.0</DDFName>\n"
            + "            </Type>\n"
            + "        </RTProperties>\n"
            + "        <Node>\n"
            + "            <NodeName>x1</NodeName>\n"
            + "            <Node>\n"
            + "                <NodeName>CredentialPriority</NodeName>\n"
            + "                <Value>1</Value>\n"
            + "            </Node>\n"
            + "            <Node>\n"
            + "                <NodeName>SubscriptionRemediation</NodeName>\n"
            + "                <Node>\n"
            + "                    <NodeName>URI</NodeName>\n"
            + "                    <Value>remediation-server.R2-testbed.wi-fi.org</Value>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>certURL</NodeName>\n"
            + "                    <Value>http://remediation-server.R2-testbed.wi-fi.org/server.cer</Value>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>certSHA256Fingerprint</NodeName>\n"
            + "                    <Value>abcdef01234567899876543210fedcbaabcdef01234567899876543210fedcba</Value>\n"
            + "                </Node>\n"
            + "            </Node>\n"
            + "            <Node>\n"
            + "                <NodeName>SubscriptionUpdate</NodeName>\n"
            + "                <Node>\n"
            + "                    <NodeName>UpdateInterval</NodeName>\n"
            + "                    <Value>4294967295</Value>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>UpdateMethod</NodeName>\n"
            + "                    <Value>ClientInitiated</Value>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>Restriction</NodeName>\n"
            + "                    <Value>HomeSP</Value>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>URI</NodeName>\n"
            + "                    <Value>subscription-server.R2-testbed.wi-fi.org</Value>\n"
            + "                </Node>\n"
            + "            </Node>\n"
            + "            <Node>\n"
            + "                <NodeName>HomeSP</NodeName>\n"
            + "                <Node>\n"
            + "                    <NodeName>FriendlyName</NodeName>\n"
            + "                    <Value>Wi-Fi Alliance</Value>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>FQDN</NodeName>\n"
            + "                    <Value>wi-fi.org</Value>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>HomeOIList</NodeName>\n"
            + "                    <Node>\n"
            + "                        <NodeName>x1</NodeName>\n"
            + "                        <Node>\n"
            + "                            <NodeName>HomeOI</NodeName>\n"
            + "                            <Value>506f9a</Value>\n"
            + "                        </Node>\n"
            + "                        <Node>\n"
            + "                            <NodeName>HomeOIRequired</NodeName>\n"
            + "                            <Value>FALSE</Value>\n"
            + "                        </Node>\n"
            + "                    </Node>\n"
            + "                    <Node>\n"
            + "                        <NodeName>x2</NodeName>\n"
            + "                        <Node>\n"
            + "                            <NodeName>HomeOI</NodeName>\n"
            + "                            <Value>004096</Value>\n"
            + "                        </Node>\n"
            + "                        <Node>\n"
            + "                            <NodeName>HomeOIRequired</NodeName>\n"
            + "                            <Value>FALSE</Value>\n"
            + "                        </Node>\n"
            + "                    </Node>\n"
            + "                </Node>\n"
            + "            </Node>\n"
            + "            <Node>\n"
            + "                <NodeName>SubscriptionParameters</NodeName>\n"
            + "            </Node>\n"
            + "            <Node>\n"
            + "                <NodeName>Credential</NodeName>\n"
            + "                <Node>\n"
            + "                    <NodeName>CreationDate</NodeName>\n"
            + "                    <Value>2012-12-01T12:00:00Z</Value>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>UsernamePassword</NodeName>\n"
            + "                    <Node>\n"
            + "                        <NodeName>Username</NodeName>\n"
            + "                        <Value>test01</Value>\n"
            + "                    </Node>\n"
            + "                    <Node>\n"
            + "                        <NodeName>Password</NodeName>\n"
            + "                        <Value>Q2hhbmdlTWU=</Value>\n"
            + "                    </Node>\n"
            + "                    <Node>\n"
            + "                        <NodeName>MachineManaged</NodeName>\n"
            + "                        <Value>TRUE</Value>\n"
            + "                    </Node>\n"
            + "                    <Node>\n"
            + "                        <NodeName>EAPMethod</NodeName>\n"
            + "                        <Node>\n"
            + "                            <NodeName>EAPType</NodeName>\n"
            + "                            <Value>21</Value>\n"
            + "                        </Node>\n"
            + "                        <Node>\n"
            + "                            <NodeName>InnerMethod</NodeName>\n"
            + "                            <Value>MS-CHAP-V2</Value>\n"
            + "                        </Node>\n"
            + "                    </Node>\n"
            + "                </Node>\n"
            + "                <Node>\n"
            + "                    <NodeName>Realm</NodeName>\n"
            + "                    <Value>wi-fi.org</Value>\n"
            + "                </Node>\n"
            + "            </Node>\n"
            + "        </Node>\n"
            + "    </Node>\n"
            + "</MgmtTree>";

    // Binder interface to client service
    IDMClientService mDMClientService;

    class InjectPayloadTask extends AsyncTask<Void, Void, Void> {
          protected Void doInBackground(Void... ignored) {
              try {
                  Log.d(TAG, "injecting SOAP package");

                  // service will add the package synchronously then return result.
                  int ret = mDMClientService.injectSoapPackage("./Wi-Fi/R2-testbed.wi-fi.org",
                          "ADD", SAMPLE_PAYLOAD);

                  Log.d(TAG, "session result was " + ret + ", dumping tree");
                  // TODO

                  Log.d(TAG, "finishing activity");
                  finish();
              } catch (RemoteException e) {
                  Log.e(TAG, "remote exception", e);
              }
              return null;
          }

          protected void onPostExecute(Void result) {
              Log.d(TAG, "closing activity");
              finish();
          }
    }

    // Service connector object to attach/detach binder interface
    private final DMClientServiceConnector mDMServiceConnector = new DMClientServiceConnector();

    private class DMClientServiceConnector implements ServiceConnection {
        // package local constructor (called by outer class)
        DMClientServiceConnector() {}

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            Log.d(TAG, "onServiceConnected for " + name + " service " + service);
            mDMClientService = IDMClientService.Stub.asInterface(service);
            new InjectPayloadTask().execute();
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
        setContentView(R.layout.inject_soap_package_activity);
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
