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

interface IDMClientService
{
    /**
     * Gets DM tree node value by specified path.
     *
     * If DM tree doesn't contain the value, null value shall be returned by
     * the method and DM engine will use the getOperationResult() method
     * to get result of the operation.
     *
     * @param path full path to the node.
     * @param recursive whether to return the child nodes recursively.
     * @return node value object in success case, otherwise null.
     */
    DmtData getDMTree(String path, boolean recursive);

    /**
     * Starts a client session for the specified server ID.
     *
     * @param path full path to the node.
     * @param clientCert URI of the client cert to use, or null
     * @param privateKey private key to use, or null
     * @param alertType the alert 1226 command
     * @param redirectURI the redirect URI
     * @param username the username for digest authentication, or null
     * @param password the password for digest authentication, or null
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int startClientSession(String path, String clientCert, String privateKey,
            String alertType, String redirectURI, String username, String password);

    /**
     * Notifies that the exec callback operation completed.
     * @param path the path for the exec call
     */
    int notifyExecFinished(String path);

    /**
     * Inject the specified SOAP-XML package into the DM tree.
     *
     * @param path node path
     * @param command the command to pass
     * @param payload the SyncML payload to process
     * @return ErrorCodes.SYNCML_DM_SUCCESS in success case, otherwise an error.
     */
    int injectSoapPackage(String path, String command, String payload);
}
