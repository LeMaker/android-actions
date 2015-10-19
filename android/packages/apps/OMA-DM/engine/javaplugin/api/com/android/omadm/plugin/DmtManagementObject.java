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

package com.android.omadm.plugin;

import android.text.TextUtils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public abstract class DmtManagementObject implements IDmtSubTree {

    private final String mPath;

    private boolean mIsUpdated;

    protected final Map<String, DmtPluginNode> mNodes = new HashMap<String, DmtPluginNode>();

    private final IDmtRootPlugin mRootPlugin;     // FIXME: assigned but never accessed

    public DmtManagementObject(String path, IDmtRootPlugin rootPlugin) throws DmtException {
        if (!DmtPathUtils.isValidPath(path)) {
            throw new DmtException("Invalid URI");
        }

        mPath = path;
        mRootPlugin = rootPlugin;

        mNodes.put(path, new DmtPluginNode(mPath, new DmtData(null, DmtData.NODE)));
    }

    /**
     * Checks if a node by given path exists in the subtree.
     *
     * @param nodePath path to the node.
     * @return true if the node is exist otherwise false.
     */
    protected boolean isNodeExist(String nodePath) {
        return (mNodes.get(nodePath) != null);
    }

    /**
     * Creates new leaf node in the subtree. The parent node is updated automatically.
     *
     * @param nodePath path to the new node.
     * @param nodeValue new node value.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} in success case otherwise an error.
     */
    protected final int createLeafNode_(String nodePath, DmtData nodeValue) {
        if (nodeValue == null) {
            return ErrorCodes.SYNCML_DM_INVALID_PARAMETER;
        }

        String[] data = DmtPathUtils.splitPath(nodePath);

        String rootNodePath = data[0];
        String nodeName     = data[1];

        if (TextUtils.isEmpty(rootNodePath) || TextUtils.isEmpty(nodeName)) {
            return ErrorCodes.SYNCML_DM_INVALID_URI;
        }

        if (isNodeExist(nodePath)) {
            return updateLeafNode_(nodePath, nodeValue);
        }

        int retcode = createInteriorNode_(rootNodePath);
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
     * Creates a new interior node in the subtree. The parent node is updated automatically.
     *
     * @param nodePath path to the new node.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} in success case otherwise an error.
     */
    protected final int createInteriorNode_(String nodePath) {
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
            return node.isLeaf() ? ErrorCodes.SYNCML_DM_ENTRY_EXIST
                    : ErrorCodes.SYNCML_DM_SUCCESS;
        }

        int retcode = createInteriorNode_(rootNodePath);
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


    private int addNameToInteriorNode(String rootNodePath, String nodeName) {
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
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} in success case otherwise an error.
     */
    protected final int updateLeafNode_(String nodePath, DmtData nodeValue) {
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

    /**
     * Removes a node. If the node is an inner node, the root node of it is
     * updated automatically.
     *
     * @param nodePath path to the node.
     * @return {@link ErrorCodes#SYNCML_DM_SUCCESS} in success case otherwise an error.
     */
    protected final int deleteNode_(String nodePath) {
        DmtPluginNode node = mNodes.get(nodePath);
        if (node == null) {
            return ErrorCodes.SYNCML_DM_ENTRY_NOT_FOUND;
        }

        if (mPath.equals(nodePath)) {
            mNodes.clear();
            return ErrorCodes.SYNCML_DM_SUCCESS;
        }

        String[] data = DmtPathUtils.splitPath(nodePath);
        if (TextUtils.isEmpty(data[0]) || TextUtils.isEmpty(data[1])) {
            return ErrorCodes.SYNCML_DM_FAIL;
        }

        DmtPluginNode rootNode = mNodes.get(data[0]);
        if (rootNode == null) {
            return ErrorCodes.SYNCML_DM_TREE_CORRUPT;
        }

        /* Remove the node name from root node's list */
        Map<String, DmtData> children;
        try {
            children = rootNode.getValue().getChildNodeMap();
        } catch (DmtException e) {
            return e.getCode();
        }

        if (children == null) {
            return ErrorCodes.SYNCML_DM_TREE_CORRUPT;
        }

        children.remove(data[1]);

        /*
         * Since getNodeValue() returns reference to
         * its own object we can change it without creating
         * of new DmtData object
         */
        // rootNode.setValue(new DmtData(children));

        /* Remove the node from the sub tree */
        mNodes.remove(nodePath);

        /* Remove all sub nodes for the interior node */
        if (!node.isLeaf()) {
            String subPath = nodePath + '/';

            String[] paths = mNodes.keySet().toArray(new String[mNodes.size()]);
            for (String path : paths) {
                if (path.startsWith(subPath)) {
                    mNodes.remove(path);
                }
            }
        }

        return ErrorCodes.SYNCML_DM_SUCCESS;
    }

    /**
     * Performs mapping of a given node path tag to relative node path.
     *
     * @param tag arbitrary tag that defines the node.
     * @param getSemantics true if GET semantics; false if SET semantics.
     * @return DMT node path.
     */
    protected abstract String resolve(String tag, boolean getSemantics) throws DmtException;

    /**
     * Returns a node value by given path. The method uses
     * resolve() method which should be implemented on successor side
     * to get full path to the node by tag.
     *
     * @param tag arbitrary tag that defines the node.
     * @throws DmtException if the node is not present.
     * @return DmtData. It can be null if value is not set.
     */
    public final DmtData getLeafNodeDmtValue(String tag) throws DmtException {
        String nodePath;
        try {
            nodePath = resolve(tag, true);
        } catch (Exception e) {
            throw new DmtException("Resolver throws an exception");
        }

        if (TextUtils.isEmpty(nodePath)) {
            throw new DmtException("Resolver returns empty string");
        }
        nodePath = mPath + '/' + nodePath;

        return getNodeValue(nodePath);
    }

    /**
     * Returns a string node value by given tag. The method uses
     * resolve() method which should be implemented on successor side
     * to get full path to the node by tag.
     *
     * @param tag arbitrary tag that defines the node.
     * @throws DmtException if the node is not present or node type is not String.
     * @return string. It can be null if value is not set.
     */
    public final String getLeafNodeStringValue(String tag) throws DmtException {
        DmtData data = getLeafNodeDmtValue(tag);
        if (data == null) {
            throw new DmtException("Data is not set for the node. Tag = " + tag);
        }

        if (data.getType() == DmtData.NULL) {
            return "";
        }

        if (data.getType() != DmtData.STRING) {
            throw new DmtException("Node type is not STRING");
        }

        return data.getString();
    }

    /**
     * Returns a integer node value by given tag. The method uses
     * resolve() method which should be implemented on successor side
     * to get full path to the node by tag.
     *
     * @param tag arbitrary tag that defines the node.
     * @throws DmtException if the node is not present or node type is not String.
     * @return value of integer node.
     */
    public final int getLeafNodeIntegerValue(String tag) throws DmtException {
        DmtData data = getLeafNodeDmtValue(tag);
        if (data == null) {
            throw new DmtException("Data is not set for the node. Tag = " + tag);
        }

        if (data.getType() != DmtData.INT) {
            throw new DmtException("Node type is not INT");
        }

        return data.getInt();
    }

    /**
     * Sets value of node specified by given tag. Creates the node and any
     * necessary intermediate nodes that lie within the node path if necessary.
     *
     * @param tag MO-defined string that describes the node.
     * @param value node value.
     * @throws DmtException if an error occurred.
     */
    public final void setLeafNodeValue(String tag, DmtData value) throws DmtException {
        String nodePath;
        try {
            nodePath = resolve(tag, false);
        } catch (Exception e) {
            throw new DmtException("Resolver throws an exception");
        }

        if (TextUtils.isEmpty(nodePath)) {
            throw new DmtException("Resolver returns empty string");
        }
        nodePath = mPath + '/' + nodePath;

        if (isNodeExist(nodePath)) {
            if (updateLeafNode_(nodePath, value) != ErrorCodes.SYNCML_DM_SUCCESS) {
                throw new DmtException("Cannot update node");
            }
            return;
        }

        if (createLeafNode_(nodePath, value) != ErrorCodes.SYNCML_DM_SUCCESS) {
            throw new DmtException("Cannot create node");
        }
    }

    /**
     * Sets the string value of node specified by given tag. Creates the node
     * and any necessary intermediate nodes that lie within the node path if
     * necessary.
     *
     * @param tag MO-defined string that describes the node.
     * @param value node value.
     * @throws DmtException if an error occurred.
     */
    public final void setLeafNodeValue(String tag, String value) throws DmtException {
        setLeafNodeValue(tag, new DmtData(value));
    }

    /**
     * Marks the management object as updated.
     */
    public void setNodeUpdatedState() {
        mIsUpdated = true;
    }

    @Override
    public int createInteriorNode(String path) {
        String[] data = DmtPathUtils.splitPath(path);
        if (TextUtils.isEmpty(data[0]) || !isNodeExist(data[0])) {
            return ErrorCodes.SYNCML_DM_NOT_FOUND;
        }

        int retcode = createInteriorNode_(path);
        if (retcode == ErrorCodes.SYNCML_DM_SUCCESS) {
            mIsUpdated = true;
        }
        return retcode;
    }

    @Override
    public int createLeafNode(String path, DmtData value) {
        String[] data = DmtPathUtils.splitPath(path);
        if (TextUtils.isEmpty(data[0]) || !isNodeExist(data[0])) {
            return ErrorCodes.SYNCML_DM_NOT_FOUND;
        }

        int retcode = createLeafNode_(path, value);
        if (retcode == ErrorCodes.SYNCML_DM_SUCCESS) {
            mIsUpdated = true;
        }
        return retcode;
    }

    @Override
    public int updateLeafNode(String path, DmtData newValue) {
        int retcode = updateLeafNode_(path, newValue);
        if (retcode == ErrorCodes.SYNCML_DM_SUCCESS) {
            mIsUpdated = true;
        }
        return retcode;
    }

    @Override
    public int renameNode(String path, String newName) {
        return ErrorCodes.SYNCML_DM_UNSUPPORTED_OPERATION;
    }

    @Override
    public int deleteNode(String path) {
        int retcode = deleteNode_(path);
        if (retcode == ErrorCodes.SYNCML_DM_SUCCESS) {
            mIsUpdated = true;
        }
        return retcode;
    }

    @Override
    public boolean isNodeUpdated() {
        return mIsUpdated;
    }

    @Override
    public void resetUpdated() {
        mIsUpdated = false;
    }

    @Override
    public Map<String, DmtPluginNode> getNodes(String nodePath) throws DmtException {
        if (!DmtPathUtils.isSubPath(mPath, nodePath)) {
            throw new DmtException(ErrorCodes.SYNCML_DM_NOT_FOUND,
                                   "Cannot get nodes for given path");
        }

        if (nodePath.equals(mPath)) {
            return mNodes;
        }

        Map<String, DmtPluginNode> nodes = new HashMap<String, DmtPluginNode>();

        for (Map.Entry<String, DmtPluginNode> nodeEntry : mNodes.entrySet()) {
            if (DmtPathUtils.isSubPath(nodePath, nodeEntry.getKey())) {
                nodes.put(nodeEntry.getKey(), nodeEntry.getValue());
            }
        }
        return nodes;
    }

    @Override
    public DmtPluginNode getNode(String nodePath) throws DmtException {
        DmtPluginNode node = mNodes.get(nodePath);
        if (node == null) {
            throw new DmtException(ErrorCodes.SYNCML_DM_NOT_FOUND,
                                   "The requested node doesn't exist");
        }
        return node;
    }

    @Override
    public DmtData getNodeValue(String nodePath) throws DmtException {
        DmtPluginNode node = mNodes.get(nodePath);
        if (node == null) {
            throw new DmtException(ErrorCodes.SYNCML_DM_NOT_FOUND,
                                   "The requested node doesn't exist");
        }
        return node.getValue();
    }

    @Override
    public String getNodePath() {
        return mPath;
    }

    /**
     * Returns the name of the root node.
     *
     * @return node name.
     */
    public String getNodeName() {
        return DmtPathUtils.getNodeName(mPath);
    }
}
