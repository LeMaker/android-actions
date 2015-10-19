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

import com.android.omadm.plugin.DmtData;
import com.android.omadm.plugin.DmtPluginNode;

interface IDmtPlugin
{
    /**
     * Initializes the plug-in.
     *
     * @param rootPath the root path of the plug-in.
     * @param parameters initial parameters of the plug-in. Type of the parameter is Map<String, String> type.
     * @return true in success case, otherwise false.
     */
    boolean init(String rootPath, in Map parameters);

    /**
     * This method returns result of the last operation which has requested by DM engine.
     * It is required because each plug-in is a content provider and it is impossible to
     * throw the DmtException with an error code to the DM engine.
     *
     * @return operation result of the last operation.
     */
    int getOperationResult();

    /**
     * Sets Server ID of the plug-in.
     *
     * @param serverID service ID
     */
    void setServerID(String serverID);

    /**
     * Returns Server ID.
     *
     * @return Server ID which has been set by DM engine for the plug-in.
     */
    String getServerID();

    /**
     * The method is called to release plug-in resource when the plug-in is no longer used.
     *
     * @return true in success case, otherwise false.
     */
    boolean release();

    /**
     * Gets node by given path.
     *
     * If plug-in cannot return the node null value shall be returned by
     * the method and DM engine will use the getOperationResult() method
     * to get result of the operation.
     *
     * @param path full path to the node.
     * @return node object in success case, otherwise null.
     */
    DmtPluginNode getNode(String path);

    /**
     * Gets node value by given path.
     *
     * If plug-in cannot return the value null value shall be returned by
     * the method and DM engine will use the getOperationResult() method
     * to get result of the operation.
     *
     * @param path full path to the node.
     * @return node value object in success case, otherwise null.
     */
    DmtData getNodeValue(String path);

    /**
     * Gets a set of nodes by given path.
     *
     * If plug-in cannot return the set null value shall be returned by
     * the method and DM engine will use the getOperationResult() method
     * to get result of the operation.
     *
     * @param path full path to the node.
     * @return a set of nodes in success case otherwise null.
     *         Type of returned value shall be Map<String, DmtPluginNode>.
     *         Where the first parameter is full path to the node and the
     *         second is node object.
     */
    Map getNodes(String path);

    /**
     * Creates an interior node in the tree by given path.
     *
     * @param path full path to the node.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int createInteriorNode(String path);

    /**
     * Creates a leaf node in the tree by given path.
     *
     * @param path full path to the node.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int createLeafNode(String path, in DmtData value);

    /**
     * Creates a sibling of the node specified by its URI "path".
     * This new node's name is user-specified as "newNodename".
     * An error shall be returned in following cases:
     *   -  If either path or new node name are null
     *   -  If node to be copied does not exist
     *   -  If a sibling node with the user-specified name already exists
     *   -  If the new node cannot be created for some other reason
     *
     * NOTE: This is optional command of OMA DM protocol.
     *       Currently java plug-ins do not support the command.
     *
     * @param path full path to node to be cloned.
     * @param newNodename New node name as specified by user.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int clone(String path, String newNodename);

    /**
     * Renames a node.
     *
     * NOTE: This is optional command of OMA DM protocol.
     *       Currently java plug-ins do not support the command.
     *
     * @param path full path to node to be renamed.
     * @param newName new node name.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int renameNode(String path, String newName);

    /**
     * Updates a leaf node value by given path.
     *
     * @param path full path to the leaf node.
     * @param newValue new value of the leaf node.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int updateLeafNode(String path, in DmtData newValue);

    /**
     * Deletes a node by given path.
     * @param path full path to the leaf node.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int deleteNode(String path);

    /**
     * Performs "Execute" command on a plug-in node.
     *
  	 * @param path node path
     * @param args exec plug-in arguments
     * @param correlator correlator
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int exec(String path, String args, String correlator);

    /**
     * Performs commit operation under current DMT.
     *
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int commit();

    /**
     * Returns DM account server PW. This is not lookup from the DM tree; this function generates it.
     *
     * @return String. null if plugin is not carrier specific
     */
    String getServerPW(String aiServerPW);

    /**
     * Returns DM account client PW. This is not lookup from the DM tree; this function generates it.
     *
     * @return String. null if plugin is not carrier specific
     */
    String getClientPW(String aiClientPW);

    /**
     * Returns DM account user name. This is not lookup from the DM tree; this function generates it.
     *
     * @return String. null if plugin is not carrier specific
     */
    String getUsername(String aiUsername);
}
