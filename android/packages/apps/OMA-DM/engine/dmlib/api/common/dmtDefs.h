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

#ifndef DMT_DEFS_H
#define DMT_DEFS_H

/** 
 \file dmtDefs.h

 \brief   The dmtDefs.h header file contains constants and datatype definitions exported
          from the DM engine.   
*/

#include "dmtError.h"

/**
 * The enumeration defines lock types for the access device management tree
 */
enum {
    /**
     * Shared lock allows read-only access to the tree, but can be shared between multiple readers.
     */
    SYNCML_DM_LOCK_TYPE_SHARED = 0,

    /**
     * Exclusive lock allows full access to the tree, but can not be shared with any other locks.
     */
    SYNCML_DM_LOCK_TYPE_EXCLUSIVE = 1,

    /**
     * Automatic lock is treated as "shared" by engine until first "set" operation. 
     * During first "set" operation lock is upgraded to exclusive with potential wait operation.
     * Note: upgrade operation is performed via unlock, which makes it possible for the tree to change,
     * so after first "read" but before next "write" operations, this field potentially can be changed.
     * If this is crucial, explicit "Exclusive" lock should be specified.
     * Note: in case of any lock, if tree object is held for more than 10 sec without accessing,
     * lock is released automatically to prevent blocking other processes by misbehaving app.
     */
    SYNCML_DM_LOCK_TYPE_AUTOMATIC = 2
};

/**
 * Lock types definition for the accessing device management tree
 */
typedef INT32 SYNCML_DM_TREE_LOCK_TYPE_T;

/**
 * The enumeration defines data types for the device management tree
 */
enum {
     /** Undefined data type  */
    SYNCML_DM_DATAFORMAT_UNDEFINED = -1,
    /** Null data type  */
    SYNCML_DM_DATAFORMAT_NULL      = 0,
    /** String data type  */
    SYNCML_DM_DATAFORMAT_STRING    = 1,
    /** Integer data type  */
    SYNCML_DM_DATAFORMAT_INT       = 2,
    /** Boolean data type  */
    SYNCML_DM_DATAFORMAT_BOOL      = 3,
    /** Binary data type  */
    SYNCML_DM_DATAFORMAT_BIN       = 4,
    /** Node data type  */
    SYNCML_DM_DATAFORMAT_NODE      = 5,
    /** Base 64 data encoding type  */
    SYNCML_DM_DATAFORMAT_B64       = 6,
    /** XML data type  */
    SYNCML_DM_DATAFORMAT_XML       = 7,
    /** Date data type  */
    SYNCML_DM_DATAFORMAT_DATE      = 8,
    /** Time data type  */
    SYNCML_DM_DATAFORMAT_TIME      = 10,
    /** Float data type  */
    SYNCML_DM_DATAFORMAT_FLOAT     = 11
};

/**
 * Definition data types for the device management tree.
 */
typedef INT8 SYNCML_DM_DATAFORMAT_T;

/**
 * The enumeration defines operation types that can be performed on a plugin sub tree for
 * the device management tree
 */
enum
{
    /** No command */
    SYNCML_DM_PLUGIN_NO_COMMAND,
    /** Command "Add" */
    SYNCML_DM_PLUGIN_ADD,
    /** Command "Delete" */
    SYNCML_DM_PLUGIN_DELETE,
    /** Command "Replace" */
    SYNCML_DM_PLUGIN_REPLACE,
    /** Command "Add Child" */
    SYNCML_DM_PLUGIN_ADD_CHILD,
    /** Command "Get" */
    SYNCML_DM_PLUGIN_GET,
    /** Command "Rename" */
    SYNCML_DM_PLUGIN_RENAME
};

/**
 * Definition operation types that can be performed on a plugin sub tree for
 * the device management tree.
 */
typedef UINT8 SYNCML_DM_PLUGIN_COMMAND_T;

/**
 * The enumeration defines command properties for the  device management tree operations
 */
enum
{
    /** Execute command on node */
    SYNCML_DM_PLUGIN_COMMAND_ON_NODE,     
    /** Execute command on property name  */
    SYNCML_DM_PLUGIN_COMMAND_ON_NAME_PROPERTY,
    /** Execute command on property titles */
    SYNCML_DM_PLUGIN_COMMAND_ON_TITLE_PROPERTY,
    /** Execute command on property LOB */
    SYNCML_DM_PLUGIN_COMMAND_ON_LOB_PROPERTY
};

/**
 * Definition command properties for the  device management tree operations
 */
typedef UINT8 SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T;

/**
 * Access Control List (ACL) permissions - int "or'ed" of the following values 
 */
enum {
    /** Grants "Add" permissions */
    SYNCML_DM_ACL_ADD  = 0x01,
    /** Grants "Delete" permissions */
    SYNCML_DM_ACL_DELETE = 0x02,
    /** Grants "Get" permissions */
    SYNCML_DM_ACL_GET  = 0x04,
    /** Grants "Replace" permissions */
    SYNCML_DM_ACL_REPLACE = 0x08,
    /** Grants "Execute" permissions */
    SYNCML_DM_ACL_EXEC  = 0x10
}; 

/**
 * Definition Access Control List (ACL) permissions.
 */
typedef UINT8 SYNCML_DM_ACL_PERMISSIONS_T;

enum {
    /** No updates */	
    SYNCML_DM_EVENT_NONE = 0,
    /** Node added */	
    SYNCML_DM_EVENT_ADD = 0x01,
    /** Leaf node replaced */	
    SYNCML_DM_EVENT_REPLACE = 0x02,
    /** Node deleteds */	
    SYNCML_DM_EVENT_DELETE = 0x04,
    /** Interior node renamed */	
    SYNCML_DM_EVENT_RENAME = 0x08,
    /** Inderect update */	
    SYNCML_DM_EVENT_INDIRECT = 0x10,
};

/**
 * Definition of action performed on DM node.
 */
typedef UINT8 SYNCML_DM_EVENT_ACTION_T;

enum {
    /** Event constructed only for updates on specific node */		
    SYNCML_DM_EVENT_NODE = 0,
    /** Cumulative event constructed for sub tree */		
    SYNCML_DM_EVENT_CUMULATIVE = 1,
    /** Detail event constracted for node or sub tree */
    SYNCML_DM_EVENT_DETAIL = 2    
};

/**
 * Definition of depth of DM event.
 */
typedef UINT8 SYNCML_DM_EVENT_TYPE_T;

/**
 * The enumeration defines types for SyncML DM server session
 */
enum
{
    /** "Server Initiated" session type */
    SYNCML_DM_SERVER_INITIATED_SESSION = 1200,
    /** "Clien Initiated" session type */
    SYNCML_DM_CLIENT_INITIATED_SESSION = 1201
};

/**
 * Definition types for SyncML DM server session
 */
typedef UINT16 SYNCML_DM_SESSION_DIRECTION_T;

#endif
