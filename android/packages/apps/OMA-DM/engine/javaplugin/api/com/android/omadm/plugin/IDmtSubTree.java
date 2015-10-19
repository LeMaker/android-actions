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

import java.util.Map;

public interface IDmtSubTree {

    interface Creator<T> {

        /**
         * This method shall be called when a new instance of
         * subtree is created by a component.
         * (FIXME comment: For example com.android.omadm.plugin.DmtMultiNode class)
         *
         * @param path path to the new node.
         * @param pluginRoot object of the plug-in.
         * @return subtree object.
         */
        T createFromPath(String path, IDmtRootPlugin pluginRoot);
    }

    /**
     * Returns a node specified by path.
     *
     * @param path full path to the node.
     * @throws DmtException if the node is not present.
     * @return the node object.
     */
    DmtPluginNode getNode(String path) throws DmtException;

    /**
     * Returns a node value specified by path.
     *
     * @param path full path to the node.
     * @throws DmtException if the node is not present.
     * @return the node value object.
     */
    DmtData getNodeValue(String path) throws DmtException;

    /**
     * Returns a set of nodes specified by path.
     *
     * @param rootPath full path to the node.
     * @throws DmtException if the node is not present.
     * @return the map of paths and node objects.
     */
    Map<String, DmtPluginNode> getNodes(String rootPath) throws DmtException;

    /**
     * Creates an interior node in the tree.
     *
     * @param path full path to the new node.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case otherwise an error.
     */
    int createInteriorNode(String path);

    /**
     * Creates a leaf node in the tree.
     *
     * @param path full path to the node.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case otherwise an error.
     */
    int createLeafNode(String path, DmtData value);

    /**
     * Renames a node which has PlaceHolder type.
     *
     * @param path full path to the node.
     * @param newName new node name.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case otherwise an error.
     */
    int renameNode(String path, String newName);

    /**
     * Updates a leaf node value.
     *
     * @param path full path to the leaf node.
     * @param newValue new value of the leaf node.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case otherwise an error.
     */
    int updateLeafNode(String path, DmtData newValue);

    /**
     * Deletes a node specified by path.
     *
     * @param path full path to the leaf node.
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case otherwise an error.
     */
    int deleteNode(String path);

    /**
     * Checks if the current node or its sub nodes were updated.
     *
     * @return true in updated case, otherwise false.
     */
    boolean isNodeUpdated();

    /**
     * Resets updated flag.
     */
    void resetUpdated();

    /**
     * Returns the node path.
     *
     * @return node path.
     */
    String getNodePath();
}
