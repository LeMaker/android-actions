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

import android.text.TextUtils;
import android.util.Log;

import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.DmtException;
import com.android.omadm.plugin.DmtPathUtils;
import com.android.omadm.plugin.DmtPluginNode;
import com.android.omadm.plugin.ErrorCodes;

import java.util.HashMap;
import java.util.Map;

public class DmtMoBuilder {
    protected static final String TAG = DmtPluginFacade.TAG;

    protected String mPath;

    protected Map<String, DmtPluginNode> mNodes = new HashMap<String, DmtPluginNode>();

    public DmtMoBuilder(String path, Map<String, DmtPluginNode> nodes) {
        Log.d(TAG, "--ENTER-- DmtMoBuilder constructor()");
        mPath = path;
        mNodes = nodes;

    }

    /**
     * Checks if a node by given path exists in the subtree.
     *
     * @param nodePath path to the node.
     * @return true if the node is exist otherwise false.
     */
    public boolean isNodeExist(String nodePath) {
        Log.d(TAG, "--ENTER-- DmtMoBuilder.isNodeExist()");
        return (mNodes.get(nodePath) != null);
    }

    /**
     * Creates new leaf node in the subtree. The parent node is updated automatically.
     *
     * @param nodePath path to the new node.
     * @param nodeValue new node value.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} in success case otherwise an error.
     */
    public int createLeafNode(String nodePath, DmtData nodeValue) {

        if (nodeValue == null) {
            return ErrorCodes.SYNCML_DM_INVALID_PARAMETER;
        }

        Log.d(TAG, "--ENTER-- DmtMoBuilder.createLeafNode(" + nodePath + "," + nodeValue.getString() + ")");

        String[] data = DmtPathUtils.splitPath(nodePath);

        String rootNodePath = data[0];
        String nodeName     = data[1];

        if (TextUtils.isEmpty(rootNodePath) || TextUtils.isEmpty(nodeName)) {
            return ErrorCodes.SYNCML_DM_INVALID_URI;
        }

        if (isNodeExist(nodePath)) {
            return updateLeafNode(nodePath, nodeValue);
        }

        int retcode = createInteriorNode(rootNodePath);
        if (retcode != ErrorCodes.SYNCML_DM_SUCCESS) {
            return retcode;
        }

        retcode = addNameToInteriorNode(rootNodePath, nodeName);
        if (retcode != ErrorCodes.SYNCML_DM_SUCCESS) {
            return retcode;
        }

        mNodes.put(nodePath, new DmtPluginNode(nodePath, nodeValue));

        return ErrorCodes.SYNCML_DM_SUCCESS;
    }

    /**
     * Utility wrapper to create DmtData, before calling createLeafNode().
     *
     * @param nodePath path to the new node.
     * @param nodeValue new node value.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success; otherwise, an error code.
     */
    public int createLeafNode(String nodePath, boolean nodeValue) {
        Log.d(TAG, String.format("DmtMoBuilder.createLeafNode(%s, %b)", nodePath, nodeValue));
        return createLeafNode(nodePath, new DmtData(nodeValue));
    }

    /**
     * Utility wrapper to create DmtData, before calling createLeafNode().
     *
     * @param nodePath path to the new node.
     * @param nodeValue new node value.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success; otherwise, an error code.
     */
    public int createLeafNode(String nodePath, int nodeValue) {
        Log.d(TAG, String.format("DmtMoBuilder.createLeafNode(%s, %d)", nodePath, nodeValue));
        return createLeafNode(nodePath, new DmtData(nodeValue));
    }

    /**
     * Utility wrapper to create DmtData, before calling createLeafNode().
     *
     * @param nodePath path to the new node.
     * @param nodeValue new node value.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success; otherwise, an error code.
     */
    public int createLeafNode(String nodePath, String nodeValue) {
        Log.d(TAG, String.format("DmtMoBuilder.createLeafNode(%s, %s)", nodePath, nodeValue));
        return createLeafNode(nodePath, new DmtData(nodeValue));
    }

    /**
     * Creates a new interior node in the subtree. The parent node is updated automatically.
     *
     * @param nodePath path to the new node.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success; otherwise, an error code.
     */
    public int createInteriorNode(String nodePath) {
        Log.d(TAG, "--ENTER-- DmtMoBuilder.createInteriorNode(" + nodePath + ")");
        String[] data = DmtPathUtils.splitPath(nodePath);

        String rootNodePath = data[0];
        String nodeName     = data[1];

        if (TextUtils.isEmpty(rootNodePath) || TextUtils.isEmpty(nodeName)) {
            return ErrorCodes.SYNCML_DM_INVALID_URI;
        }

        if (!DmtPathUtils.isSubPath(mPath, nodePath)) {
            return ErrorCodes.SYNCML_DM_INVALID_URI;
        }

        /* Check that exist node is an interior node and engine tries to set interior node too */
        DmtPluginNode node = mNodes.get(nodePath);
        if (node != null) {
            return node.isLeaf() ? ErrorCodes.SYNCML_DM_ENTRY_EXIST : ErrorCodes.SYNCML_DM_SUCCESS;
        }

        int retcode = createInteriorNode(rootNodePath);
        if (retcode != ErrorCodes.SYNCML_DM_SUCCESS) {
            return retcode;
        }

        retcode = addNameToInteriorNode(rootNodePath, nodeName);
        if (retcode != ErrorCodes.SYNCML_DM_SUCCESS) {
            return retcode;
        }

        mNodes.put(nodePath, new DmtPluginNode(nodePath, new DmtData(null, DmtData.NODE)));

        return ErrorCodes.SYNCML_DM_SUCCESS;
    }


    public int addNameToInteriorNode(String rootNodePath, String nodeName) {
        Log.d(TAG, "--ENTER-- DmtMoBuilder.addNameToInteriorNode()");
        DmtPluginNode rootNode = mNodes.get(rootNodePath);

        if (rootNode == null || rootNode.isLeaf()) {
            return ErrorCodes.SYNCML_DM_INVALID_URI;
        }

        try {
            rootNode.getValue().addChildNode(nodeName, new DmtData());
        } catch (DmtException e) {
            return e.getCode();
        }

        return ErrorCodes.SYNCML_DM_SUCCESS;
    }

    /**
     * Updates a leaf node value.
     *
     * @param nodePath full path to the node.
     * @param nodeValue new node value.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} on success; otherwise, an error code.
     */
    public int updateLeafNode(String nodePath, DmtData nodeValue) {
        Log.d(TAG, "--ENTER-- DmtMoBuilder.updateLeafNode()");
        if (nodeValue == null) {
            return ErrorCodes.SYNCML_DM_INVALID_PARAMETER;
        }

        DmtPluginNode node = mNodes.get(nodePath);
        if (node == null) {
            return ErrorCodes.SYNCML_DM_ENTRY_NOT_FOUND;
        }

        if (node.getType() == DmtData.NODE || nodeValue.getType() == DmtData.NODE) {
            return ErrorCodes.SYNCML_DM_COMMAND_NOT_ALLOWED;
        }

        node.setValue(nodeValue);

        return ErrorCodes.SYNCML_DM_SUCCESS;
    }
}