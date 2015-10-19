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

/**
\mainpage DMT Plugin API Documentation 

\section Overview 2.0 Overview 

\warning This file is for internal usage only!!!

 The Device Management implementation is based on the OMA DM (Formerly called SyncML DM)
 Standard. A core part of implementation is the Device Management Tree (DMT). Each device that 
 supports OMA DM must contain a management tree. This document describes API required for
 access and control DMT. \n
 Nodes are the entities which can be manipulated by management actions carried over the OMA DM protocol.
 A node can be as small as an integer or as large and complex as a background picture or screen saver. \n

 The DMT constitutes a universal management data access mechanism. 
However, in the OMA DM standard, the DMT is only defined logically, 
that is, it does not define how the nodes are stored or where they come from; 
therefore, in our implementation, there are two kinds of tree nodes – physical 
and logical. Physical nodes are easy to understand – all nodes together with 
their configuration data are physically persisted and stored. 
This is similar to the MS Windows registry. However not all management information 
can be covered by such a data repository.  \n

The information either is too dynamic or already exists in the system and cannot 
be duplicated in the DMT. However, these nodes can still appear in the DMT, and 
are referred to as logical nodes. \n

For example, in the P2K DM engine, all of the nodes in the tree were logical – that is, 
configuration data was stored by each application on the phone, and the DMT was a logical 
representation of that data. \n

The DM Engine supports both physical nodes and logical nodes. Different platforms may 
choose different approaches. To access these logical nodes, the DM engine supports a 
plug-in architecture, which allows libraries to be added to the DM engine for accessing 
nodes that are not physically in the tree. By using the data plug-ins, the DM engine can 
support those logical nodes. \n

This plug-in architecture also allows for libraries that are used to enforce integrity 
constraints on the nodes in the tree, as well as ‘execute’ plug-ins which allow commands or 
pointers to applications to be stored in DMT nodes. \n

The DM engine plug-in mechanism is designed to allow uniform access to both the built-in 
configuration repository and other types of management data. \n
 
Data plug-ins can be written in both C++ and Java and their interface is an extension of 
the public DMT API. For details, refer to the DMT API Documentation.
Native data plug-ins are separate libraries (in an OS environment allowing for such),
deployed in a well-known directory along with their internal configuration files. 
The files describe the sub-tree(s) of the DMT claimed by the plug-in, as well as, 
optionally, its internal configuration information for the nodes it is responsible for.


\section IndustryStandard 3.0 Industry Standard

 OMA DM (formerly SyncML DM)

\section ClassAndStructureDefinitions 5.0 C++ Class and Structure Definitions

\subsection Classes 5.1 Classes
-# DMSubscriptionVector <em>DM Subscription Vector, inheritance from DMVector </em>
-# DmtAPIPluginTree <em> Device management plugin tree API class; inheritance from DmtTree </em>
-# DmtOPINodeData <em> Engine side support for Overlay plug-ins including meta node ID and PD retrieval  </em>
-# DmtOverlayPluginSyncData <em> Overlay plugin synchronization data  </em>
-# DmtPluginNode <em> Class DmtPluginNode for default read only plugin node implementation  </em>
-# DmtPluginTree <em> Has parent DmtAPIPluginTree class; handle operation with nodes and other tasks </em>
-# DmtRWPluginNode <em> Based plugin node class with read/write functionality, inheritance from DmtPluginNode  </em>
-# DmtRWPluginTree <em> Based plugin tree class with read/write functionality, inheritance from DmtPluginTree  </em>

\subsection Structures 5.2 Structures
Details of the structures are available from the source header files directly:
-# dmtCommitPlugin.hpp
-# dmtPlugin.hpp

\section CFunctionsAndDataTypes 6.0 "C" Functions and Data Types
-# const DmtOPINodeData* DmtGetCachedOPINodeData() <em>For the internal usage only!This function should be called only from the plug-in "getNode" function</em>
-# SYNCML_DM_RET_STATUS_T DmtSetOPINodeData( CPCHAR szURI, const DmtOverlayPluginData& oData ) <em>For the internal usage only! Allow to update plug-in data in DMT</em>

\section HeaderFilesAndLibraries 7.0 Header Files And Libraries

-# dmtDefs.h
-# dmtError.h
-# dmtCommitPlugin.hpp
-# dmtPlugin.hpp
-# dmtRWPlugin.hpp
-# xpl_Types.h
-# xpl_StringUtil.h

\section PluginExtensionPointInterface 8.0 Plug-in Extension Point Interface

 "None"
 
\section OtherInterfaces 9.0 Other Interfaces

"None"

\section OtherNotes 10.0 Other Notes

    - All APIs' functions are synchronous.

    - All APIs are in the "Final" state and guaranteed backward compatibility.

    - All APIs are not secured but protection enforced on the DMT subtree level.
*/
