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

import java.util.Arrays;

public final class DmtPathUtils {

    public static final String ROOTNODE = "__ROOT__";

    private DmtPathUtils() {}

    /**
     * Checks whether the given path is valid or not.
     *
     * @param path full path.
     * @return true if this is valid path, otherwise false.
     */
    public static boolean isValidPath(String path) {
        return (!TextUtils.isEmpty(path)) && (".".equals(path) || path.startsWith("./"));
    }

    /**
     * Parses path and returns name of the end node.
     *
     * @param nodePath path to the node.
     * @return node name or null in bad case.
     */
    public static String getNodeName(String nodePath) {
        String[] data = splitPath(nodePath);
        return data[1];
    }

    /**
     * Parses passed path and returns name of the first node
     * after root path.
     *
     * @param rootPath root path.
     * @param nodePath full path to the node.
     * @return name of the node or null in bad case.
     */
    public static String getSubNodeName(String rootPath, String nodePath) {
        if (!isValidPath(rootPath) || !isValidPath(nodePath)) {
            return null;
        }

        String prefix = rootPath + '/';

        if (!nodePath.startsWith(prefix)) {
            return null;
        }

        int nameEnd = nodePath.indexOf('/', prefix.length());
        if (nameEnd == -1) {
            return nodePath.substring(prefix.length());
        }
        return nodePath.substring(prefix.length(), nameEnd);
    }

    /**
     * Parses path and returns name of the end node and its root path.
     *
     * @param nodePath path to the node.
     * @return root path (String[0]) and name node (String[1]).
     */
    public static String[] splitPath(String nodePath) {
        String[] data = new String[2];
        Arrays.fill(data, null);

        if (!isValidPath(nodePath)) {
            return data;
        }

        int index = nodePath.lastIndexOf('/');
        if (index == -1) {
            data[1] = nodePath;
            return data;
        }

        if (nodePath.length() == 1) {
            return data;
        }

        if (index == 0) {
            data[1] = nodePath.substring(1);
            return data;
        }

        if (index == nodePath.length() - 1) {
            data[0] = nodePath.substring(0, index);
            return data;
        }

        data[0] = nodePath.substring(0, index);
        data[1] = nodePath.substring(index + 1);

        return data;
    }

    /**
     * Checks whether the root path is a part of the node path.
     *
     * @param rootPath root path.
     * @param nodePath path to some node.
     * @return true if these paths are equal or nodePath starts with rootPath.
     */
    public static boolean isSubPath(String rootPath, String nodePath) {
        if (!isValidPath(rootPath) || !isValidPath(nodePath)) {
            return false;
        }

        if (nodePath.equals(rootPath)) {
            return true;
        }

        return nodePath.startsWith(rootPath + '/');
    }

    public static String toRelativePath(String rootPath, String path) {
        String r = path;

        // deal with the root path of plugin tree
        if (rootPath.equals(path) || path.isEmpty()) {
            r = ROOTNODE;
        }
        else if (path.startsWith(rootPath)) {
            r = path.substring(rootPath.length() + 1);
        }
//        Log.i(TAG, "'" + path + "' -> '" + r + "'");
        return r;
    }

    /**
     * convert to the absolute path
     */
    public static String toAbsolutePath(String rootPath, String path) {
        String a = path;
        if (path.isEmpty() || path.equals(ROOTNODE)) {
            a = rootPath;
        }
        else if (!path.startsWith(rootPath)) {
            a = rootPath + '/' + path;
        }
//        Log.i(TAG, "'" + path + "' -> '" + a + "'");
        return a;
    }
}
