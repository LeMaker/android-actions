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
// Module Name: FNodeLoader
//
// General Description: Provides redirection to FNode (the default File 
//                      implementation).
//
//==================================================================================================

package com.mot.dm.io;

import java.io.FileNotFoundException;
import java.io.Reader;

import com.mot.dm.tool.Util;

/**
 * Default implementation
 * @author jwylder1
 */
public class FNodeLoader extends NodeLoader {
    
    /** Creates a new instance of FNodeLoader */
    public FNodeLoader() {
    }

    public Reader getReaderImpl(String name) throws FileNotFoundException {
        return Util.openUtf8FileReader(name);
    }
    
    public Reader getReaderImpl(Node node) throws FileNotFoundException {
    	FNode fnode = (FNode) node;
        return Util.openUtf8FileReader(fnode.getFile());
    }
    
    protected Node getInstanceImpl(String path) {
        return (Node) new FNode(path);
    }
}
