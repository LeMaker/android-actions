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

package com.android.omadm.pluginhelper;

import android.util.Log;

import com.android.omadm.plugin.DmtBasePlugin;
import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.DmtException;
import com.android.omadm.plugin.DmtManagementObject;
import com.android.omadm.plugin.DmtPluginNode;

import java.util.Map;

public class DmtPluginFacade extends DmtBasePlugin {

    protected static final String TAG = "DmtPluginFacade";

    static class GenericDmtManagementObject extends DmtManagementObject {

        /**
         * @param path path to the base of the management object
         */
        GenericDmtManagementObject(String path) throws DmtException {
            super(path, null);
        }

        public Map<String, DmtPluginNode> getData() {
            return mNodes;
        }

        @Override
        protected String resolve(String tag, boolean getSemantics) throws DmtException {
            if (tag != null) {
                return tag;
            }
            throw new DmtException("Invalid tag was provided");
        }
    }

    private GenericDmtManagementObject mManagementObject;

    private IDmtPluginAdaptor mAdaptor = null;

    /**
     * Plug-in facade constructor.
     *
     * @param adaptor the plugin adaptor object
     */
    public DmtPluginFacade(IDmtPluginAdaptor adaptor) {
        Log.d(TAG, "DmtPluginFacade.ctor()");
        mAdaptor = adaptor;
        try {
            mManagementObject = new GenericDmtManagementObject(adaptor.getPath());
        } catch (DmtException e) {
            Log.e(TAG, "DmtPluginFacade.ctor() - EXCEPTION: " + e.getMessage());
            setOperationResult(e.getCode());
        }
        Log.d(TAG, "DmtPluginFacade.ctor() " + mAdaptor.toString());
    }

    @SuppressWarnings("unchecked")
    public boolean init(String rootPath, Map parameters) {
        Log.d(TAG, String.format("DmtPluginFacade.init(%s, ...) - enter", rootPath));
        for (Object key : parameters.keySet()) {
            Log.d(TAG, String.format("parameters(%s) = %s", key.toString(),
                    parameters.get(key).toString()));
        }
        boolean status;
        try {
            Map<String, DmtPluginNode> nodes = mManagementObject.getData();
            Log.d(TAG, "DmtPluginFacade.init() - before load nodes.size() = " + nodes.size());
            mAdaptor.load(nodes);
            status = true;
            Log.d(TAG, "DmtPluginFacade.init() - after load nodes.size() = " + nodes.size());
        } catch (Exception e) {
            Log.e(TAG, "DmtPluginFacade.init(): EXCEPTION = " + e.getMessage());
            status = false;
        }
        Log.d(TAG, "DmtPluginFacade.init() - return  " + status);
        return status;
    }

    public boolean release() {
        Log.d(TAG, "DmtPluginFacade.release() - enter");
        mAdaptor.release();
        return true;
    }

    public int createInteriorNode(String path) {
        int status;
        Log.d(TAG, "DmtPluginFacade.createInteriorNode() - enter");
        status = setOperationResult(mManagementObject.createInteriorNode(path));
        Log.d(TAG, "DmtPluginFacade.createInteriorNode() - return " + status);
        return status;
    }

    public int createLeafNode(String path, DmtData value) {
        Log.d(TAG, String.format("DmtPluginFacade.createLeafNode(%s)", path));
        int status;
        status = setOperationResult(mManagementObject.createLeafNode(path, value));
        return status;
    }

    public int updateLeafNode(String path, DmtData newValue) {
        Log.d(TAG, String.format("DmtPluginFacade.updateLeafNode(%s)", path));
        int status;
        status = setOperationResult(mManagementObject.updateLeafNode(path, newValue));
        return status;
    }

    public int renameNode(String path, String newName) {
        Log.d(TAG, String.format("DmtPluginFacade.renameNode(%s, %s)", path, newName));
        int status;
        status = setOperationResult(mManagementObject.renameNode(path, newName));
        return status;
    }

    public int deleteNode(String path) {
        Log.d(TAG, String.format("DmtPluginFacade.deleteNode(%s)", path));
        int status;
        status = setOperationResult(mManagementObject.deleteNode(path));
        return status;
    }

    public DmtPluginNode getNode(String path) {
        Log.d(TAG, String.format("DmtPluginFacade.getNode(%s)", path));
        DmtPluginNode node = null;
        try {
            node = mManagementObject.getNode(path);
        } catch (DmtException de) {
            Log.e(TAG, "DmtPluginNode.getNode(): " + path + " e = " + de);
            setOperationResult(de.getCode());
        }
        return node;
    }

    public Map<String, DmtPluginNode> getNodes(String path) {
        Log.d(TAG, String.format("DmtPluginFacade.getNodes(%s)", path));
        Map<String, DmtPluginNode> nodes;
        try {
            nodes = mManagementObject.getNodes(path);
        } catch (DmtException de) {
            Log.e(TAG, "DmtPluginNode.getNodes(): " + path + " e = " + de);
            setOperationResult(de.getCode());
            nodes = null;
        }
        return nodes;
    }

    public DmtData getNodeValue(String path) {
        Log.d(TAG, String.format("DmtPluginFacade.getNodeValue(%s)", path));
        DmtData datum = null;
        try {
            datum = mManagementObject.getNodeValue(path);
        } catch (DmtException de) {
            Log.e(TAG, "getNodeValue(): " + path + " e = " + de);
            setOperationResult(de.getCode());
        }
        return datum;
    }

    public int commit() {
        Log.d(TAG, String.format("DmtPluginFacade.commit()"));
        if (getOperationResult() == 0) {
            int status;
            status = setOperationResult(mAdaptor.commit(mManagementObject.getData()));
            return status;
        } else {
            return getOperationResult();
        }
    }
}
