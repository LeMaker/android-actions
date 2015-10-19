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

#ifndef DMT_ERROR_H
#define DMT_ERROR_H

/**
 \file dmtError.h
 \brief  The dmtError.h header file contains DM engine error type definition.
                  There are two error types:
                  Standard SyncML DM Error codes
                  Extension ("EXT", use with device management tree API)
*/

#include "xpl_Types.h"

/**Enumeration defines Sync ML return status codes */
enum
{
/** Success */
  SYNCML_DM_SUCCESS                  = 0,
/** Unknown error (EXT) */
  SYNCML_DM_FAIL                     = 1,
/** Entry for tree mount point or transport already exist in the registry (EXT) */
  SYNCML_DM_ENTRY_EXIST              = 2,
/** Entry for tree mount point or transport not found in the registry (EXT) */
  SYNCML_DM_ENTRY_NOT_EXIST          = 3,
/** Other Session is in progress (EXT) */
  SYNCML_DM_SESSION_BUSY             = 4,
/** URI is invalid (EXT) */
  SYNCML_DM_INVALID_URI              = 5,
/**  Session authentication failed */
  SYNCML_DM_SESSION_AUTH_FAIL        =  6,
/** Second connection is not available (EXT) */
  SYNCML_DM_SESSION_NW_NOT_AVAILABLE =  7,
/** Session not connected*/
  SYNCML_DM_SESSION_NO_CONNECT       =  8,
/** Session canceled*/
  SYNCML_DM_SESSION_CANCELED         = 9,
/** File operation failure (EXT) */
  SYNCML_DM_IO_FAILURE               = 10,
/** File not found (EXT) */
  SYNCML_DM_FILE_NOT_FOUND           = 11,
/** URI is too long (EXT) */
  SYNCML_DM_URI_CONFLICT             =  12,
/** Tree file is corrupted (EXT) */
  SYNCML_DM_TREE_CORRUPT             =  13,
/** Internal error (EXT) */
  SYNCML_DM_SKIP_SUBTREE             =  14,
/** Result is too large (EXT) */
  SYNCML_DM_RESULTS_TOO_LARGE        =  15,
/** Lock context is invalid (EXT) */
  SYNCML_DM_LOCKCTX_NOTFOUND         =  16,
/** Unable to create thread (EXT) */
  SYNCML_DM_UNABLE_START_THREAD      =  17,
/** Too many data files - invalid configuration (EXT) */
  SYNCML_DM_TOO_MANY_DATA_FILES      =  18,
/** File is locked, try again later (EXT) */
  SYNCML_DM_LOCK_TRY_AGAIN           =  19,
/** Constraint failed (EXT) */
  SYNCML_DM_CONSTRAINT_FAIL          =  20,
/** Unable to load Access Control List file (EXT) */
  SYNCML_DM_LOAD_ACL_FAIL            =  21,
/** Tree is read only (EXT) */
  SYNCML_DM_TREE_READONLY            =  22,
/** At least one of the parameters is invalid (EXT) */
  SYNCML_DM_INVALID_PARAMETER        =  23,
/** Authentication accepted */
  SYNCML_DM_AUTHENTICATION_ACCEPTED  =  212,
/** Chunked item accepted and buffered      213*/
  SYNCML_DM_CHUNK_BUFFERED            = 213,     
/** Operation canceled */
  SYNCML_DM_OPERATION_CANCELLED      =  214,
/** DM engine not executed */
  SYNCML_DM_NOT_EXECUTED             =  215,
/** Atomic rollback */
  SYNCML_DM_ATOMIC_ROLLBACK_OK       =  216,
/** DM tree not modified */
  SYNCML_DM_NOT_MODIFIED             =  304,
/** Bad request */
  SYNCML_DM_BAD_REQUEST              =  400,
/** Unauthorized access */
  SYNCML_DM_UNAUTHORIZED             =  401,
/** Not found */
  SYNCML_DM_NOT_FOUND                =  404,
/** The command not allowed */
  SYNCML_DM_COMMAND_NOT_ALLOWED      =  405,
/** The asking feature not supported */
  SYNCML_DM_FEATURE_NOT_SUPPORTED    =  406,
/** Authentication required */
  SYNCML_DM_AUTHENTICATION_REQUIRED  =  407,
/** Request timeout */
  SYNCML_DM_REQUEST_TIMEOUT          =  408,
/** Incomplete command */
  SYNCML_DM_INCOMPLETE_COMMAND       =  412,
/** Request entity too large */
  SYNCML_DM_REQUEST_ENTITY_TOO_LARGE =  413,
/** URI too long */
  SYNCML_DM_URI_TOO_LONG             =  414,
/** Unsupported media type format */
  SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT     =  415,
/** Request range not satisfied */
  SYNCML_DM_REQUESTED_RANGE_NOT_SATISFIABLE  =  416,
/** The target already exists */
  SYNCML_DM_TARGET_ALREADY_EXISTS            =  418,
/** Device is full */
  SYNCML_DM_DEVICE_FULL                      =  420,
 /**Size Mismatch */
SYNCML_DM_SIZE_MISMATCH              =424,
/** Permission verification failed */
  SYNCML_DM_PERMISSION_FAILED                =  425,
/** Command failed */
  SYNCML_DM_COMMAND_FAILED                   =  500,
/** The command not implemented */
  SYNCML_DM_COMMAND_NOT_IMPLEMENTED          =  501,
/** Service currently not available */
  SYNCML_DM_SERVICE_UNAVAILABLE              =  503,
/** Gateway timeout */
  SYNCML_DM_GATEWAY_TIMEOUT                  =  504,
/** Processing error */
  SYNCML_DM_PROCESSING_ERROR                 =  506,
/** Atomic failed */
  SYNCML_DM_ATOMIC_FAILED                    =  507,
/** Data store failure */
  SYNCML_DM_DATA_STORE_FAILURE               =  510,
/** Operation cancelled failure */
  SYNCML_DM_OPERATION_CANCELLED_FAILURE      =  514,
/** Atomic rollback failed */
  SYNCML_DM_ATOMIC_ROLLBACK_FAILED           =  516,
/** Atomic response too large */
  SYNCML_DM_ATOMIC_RESPONSE_TOO_LARGE        =  517,
/** Data set not complete for an ESN */
  SYNCML_DM_ESN_SET_NOT_COMPLETE         =     518,

/** Http execute exceptions; keep in sync with DmResult.java **/
  SYNCML_DM_SOCKET_TIMEOUT                   = 700,
  SYNCML_DM_SOCKET_CONNECT_ERR               = 701,
  SYNCML_DM_NO_HTTP_RESPONSE                 = 702,
  SYNCML_DM_UNKNOWN_HOST                     = 703,
  SYNCML_DM_INTERRUPTED                      = 704,
   /**Factory Data Reset result code */
  SYNCML_DM_PROCESS_ACCEPTED                 = 1200

};

/** Return codes definition */
typedef UINT32 SYNCML_DM_RET_STATUS_T;

#endif
