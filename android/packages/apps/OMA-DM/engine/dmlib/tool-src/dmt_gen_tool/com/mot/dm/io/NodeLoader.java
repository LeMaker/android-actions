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
// Module Name: NodeLoader
//
// General Description: Provides basic factory for accessing a Node.  Nodes roughly approximate 
//                      a files functionality which by default is what actually happens.
//                      If special behavior is needed, then the ldr variable can be replaced 
//                      before any of the modules are called.
//==================================================================================================

package com.mot.dm.io;

import java.io.FileNotFoundException;
import java.io.Reader;

/**
 * NodeLoader provides access to a node and access to the associated reader.
 * @author jwylder1
 */
public abstract class NodeLoader {
    
    public static NodeLoader ldr = new FNodeLoader();  // default = use files

    protected abstract Node getInstanceImpl(String path) ;
    protected abstract Reader getReaderImpl(String path) throws FileNotFoundException;
    protected abstract Reader getReaderImpl(Node node) throws FileNotFoundException;

    /**
     * get a created node (use instead of File Create
     */
    public static Node getInstance(String path) {
            return ldr.getInstanceImpl(path);
    }
    
    /**
     * @return an associated Reader for this io type
     */
    public static Reader getReader(String path) throws FileNotFoundException {
        return ldr.getReaderImpl(path);
    }
    
    /**
     * @return an associated Reader for this io type
     */
    public static Reader getReader(Node node) throws FileNotFoundException {
        return ldr.getReaderImpl(node);
    }
    
}
