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

//==================================================================================================
//
// Module Name: FNode
//
// General Description: Provides redirection to FNode (the default File 
//                      implementation).
//
//==================================================================================================

package com.mot.dm.io;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.Reader;


/**
 * Default implementation to redirect to File class
 * @author jwylder1
 */
public class FNode implements Node {
    
    private final File file;
    
    /** Creates a new instance of FNode */
    protected FNode(String path) {
        file = new File(path);
    }
    /** ctor */
    protected FNode(File file) {
        this.file = file;
    }
    
    protected File getFile() {
    	return this.file;
    }

    /**
     * equivalent to listFiles()
     */
    public Node[] listNodes() {
        Node[] nodes = null;
        File[] children = file.listFiles();
        if (children != null) {
            nodes = new FNode[children.length];
            for (int i = 0; i < children.length; i++) {
                nodes[i] = (Node) new FNode(children[i]);
            }
        }
        return nodes;
    }

    /**
     * redirects to File.isDirectory()
     */
    public boolean isDirectory() {
        return file.isDirectory();
    }

    /**
     * get the name of the Node/File
     */
    public String getName() {
        return file.getName();
    }

    /**
     * get the absolute path of the 
     * file
     */
    public String getAbsolutePath() {
        return file.getAbsolutePath();
    }

    /**
     * @return true if the file
     *  exists
     */
    public boolean exists() {
        return file.exists();
    }

    /**
     * @return true if this is a File
     */
    public boolean isFile() {
        return file.isFile();
    }
   
}
