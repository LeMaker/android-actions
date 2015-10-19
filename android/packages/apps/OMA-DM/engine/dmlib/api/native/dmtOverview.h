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
\mainpage DMT API Documentation 

\section Overview 2.0 Overview 

 The Device Management implementation is based on the OMA DM (Formerly called SyncML DM)
 Standard. A core part of implementation is the Device Management Tree (DMT). Each device that 
 supports OMA DM must contain a management tree. This document describes API required for
 access and control DMT. \n
 Nodes are the entities which can be manipulated by management actions carried over the OMA DM protocol.
 A node can be as small as an integer or as large and complex as a background picture or screen saver.

\section IndustryStandard 3.0 Industry Standard

 OMA DM (formerly SyncML DM)


\section ExternalApiDocuments 4.0 External API Documents
"None"

\section ClassAndStructureDefinitions 5.0 C++ Class and Structure Definitions

\subsection Classes 5.1 Classes
-# DMFirmAlertVector <em>Helper class; due to gcc limitations, declaring separate class instead of typedef produces smaller binary </em>
-# DMMap <em>Simple Collection Class without overhead of STL or QT DMMap template class is similar to Java's Hashmap </em>
-# DMString <em>A simple String class similar to Java String and STL string with limited functionality</em>
-# DMStringVector <em>Helper class; due to gcc limitations, declaring separate class instead of typedef produces smaller binary </em>
-# DmtAcl <em>DMT Acl models the standard ACL attribute Acl composes of Principals associated with accessrights Principals are server identifications  </em>
-# DmtAttributes <em>DmtAttributes encapsulates all standard DMT attributes</em>
-# DmtData <em>Encapsulates various DMT leaf node data formats </em>
-# DmtDataChunk <em>encapsulates various methods to access External Storage Node (ESN) data</em>
-# DmtEventData <em> represents actual updates performed on DM node</em>
-# DmtEventSubscription <em>represents subscription on DMT update event</em>
-# DmtFirmAlert <em>Helper class for sending repair status with ALERT 1226</em>
-# DMFirmAlertVector <em>Helper class; due to gcc limitations, declaring separate class instead of typedef produces smaller binary</em>
-# DmtNode <em>DMT Represents tree nodes as they are created and added to the tree. </em>
-# DmtNotification <em>This class processes and parses "package0" information that is coming from a server </em>
-# DmtPrincipal <em>Represents actors from the security viewpoint  </em>
-# DmtSessionProp <em>Structure with parameters for server session  </em>
-# DmtTree <em>Represents the object that carrying atomicity (when supported) and authentication functionality associated with DMT access, as well as basic node access  </em>
-# DmtTreeFactory <em>Represents a tree factory</em>
-# DMVector <em>Simple Collection Class without overhead of STL or QT DMVector template class is similar to Java's ArrayList </em>
-# JemBaseObject <em> Base object for any ref-counted object </em>
-# JemSmartPtr <em>Smart pointer; works with classes derived from JemBaseObject </em>

\subsection Structures 5.2 Structures

Details of the structures are available from the source header files directly:
-# DM_WlanEncoder.h


\section CFunctionsAndDataTypes 6.0 "C" Functions and Data Types

"None"

\section HeaderFilesAndLibraries 7.0 Header Files And Libraries


-# dmstring.h
-# dmt.hpp
-# dmtAcl.hpp
-# dmtAttributes.hpp
-# dmtData.hpp
-# dmtDataChunk.hpp
-# dmtDefs.h
-# dmtError.h
-# dmtErrorDescription.hpp
-# dmtEvent.hpp
-# dmtEventData.hpp
-# dmtFirmAlert.hpp
-# dmtNode.hpp
-# dmtNotification.hpp
-# dmtPrincipal.hpp
-# dmtSessionProp.hpp
-# dmtTree.hpp
-# dmtTreeFactory.hpp
-# dmvector.h
-# dmVersion.h
-# DM_WlanEncoder.h
-# jem_defs.hpp
-# omacp.h
-# xpl_StringUtil.h
-# xpl_Types.h


\section PluginExtensionPointInterface 8.0 Plug-in Extension Point Interface

\section OtherInterfaces 9.0 Other Interfaces

"None"

\section OtherNotes 10.0 Other Notes

    - All APIs' functions are synchronous.

    - All APIs are in the "Final" state and guaranteed backward compatibility.

    - All APIs are not secured but protection enforced on the DMT subtree level.

*/
