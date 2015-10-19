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

public interface ErrorCodes {

    /** Success. */
    int SYNCML_DM_SUCCESS = 0;

    /** Unknown error (EXT). */
    int SYNCML_DM_FAIL = 1;

    /** Entry for tree mount point or transport already exists in the registry (EXT). */
    int SYNCML_DM_ENTRY_EXIST = 2;

    /** Entry for tree mount point or transport not found in the registry (EXT). */
    int SYNCML_DM_ENTRY_NOT_FOUND = 3;

    /** Another session is in progress (EXT). */
    int SYNCML_DM_SESSION_BUSY = 4;

    /** URI is invalid (EXT). */
    int SYNCML_DM_INVALID_URI = 5;

    /** Session authentication failed. */
    int SYNCML_DM_SESSION_AUTH_FAIL = 6;

    /** Second connection is not available (EXT). */
    int SYNCML_DM_SESSION_NW_NOT_AVAILABLE = 7;

    /** Session not connected. */
    int SYNCML_DM_SESSION_NOT_CONNECTED = 8;

    /** Session canceled. */
    int SYNCML_DM_SESSION_CANCELED = 9;

    /** File operation failure (EXT). */
    int SYNCML_DM_IO_FAILURE = 10;

    /** File not found (EXT). */
    int SYNCML_DM_FILE_NOT_FOUND = 11;

    /** URI is too long (EXT). */
    int SYNCML_DM_URI_CONFLICT = 12;

    /** Tree file is corrupted (EXT). */
    int SYNCML_DM_TREE_CORRUPT = 13;

    /** Internal error (EXT). */
    int SYNCML_DM_SKIP_SUBTREE = 14;

    /** Result is too large (EXT). */
    int SYNCML_DM_RESULT_TOO_LARGE = 15;

    /** Lock context is invalid (EXT). */
    int SYNCML_DM_LOCK_CONTEXT_NOT_FOUND = 16;

    /** Unable to create thread (EXT). */
    int SYNCML_DM_UNABLE_START_THREAD = 17;

    /** Too many data files: invalid configuration (EXT). */
    int SYNCML_DM_TOO_MANY_DATA_FILES = 18;

    /** File is locked; try again later (EXT). */
    int SYNCML_DM_LOCK_TRY_AGAIN = 19;

    /** Constraint failed (EXT). */
    int SYNCML_DM_CONSTRAINT_FAILED = 20;

    /** Unable to load Access Control List file (EXT). */
    int SYNCML_DM_LOAD_ACL_FILE_FAILED = 21;

    /** Tree is read-only (EXT). */
    int SYNCML_DM_READ_ONLY_TREE = 22;

    /** At least one of the parameters is invalid (EXT). */
    int SYNCML_DM_INVALID_PARAMETER = 23;

    /** LAWMO exec status code to the server. */
    int SYNCML_DM_ACCEPTED_FOR_PROCESSING = 202;

    /** Authentication accepted. */
    int SYNCML_DM_AUTHENTICATION_ACCEPTED = 212;

    /** Chunked item accepted and buffered. */
    int SYNCML_DM_CHUNK_BUFFERED = 213;

    /** Operation canceled. */
    int SYNCML_DM_OPERATION_CANCELED = 214;

    /** DM engine not executed. TODO: remove if unused. */
    int SYNCML_DM_NOT_EXECUTED = 215;

    /** Atomic rollback success. TODO: remove if unused. */
    int SYNCML_DM_ATOMIC_ROLLBACK_SUCCESS = 216;

    /** DM tree not modified. */
    int SYNCML_DM_TREE_NOT_MODIFIED = 304;

    /** Bad request. */
    int SYNCML_DM_BAD_REQUEST = 400;

    /** Unauthorized access. */
    int SYNCML_DM_UNAUTHORIZED = 401;

    /** Not found. */
    int SYNCML_DM_NOT_FOUND = 404;

    /** Command not allowed. */
    int SYNCML_DM_COMMAND_NOT_ALLOWED = 405;

    /** Requested operation not supported. */
    int SYNCML_DM_UNSUPPORTED_OPERATION = 406;

    /** Authentication required. */
    int SYNCML_DM_AUTHENTICATION_REQUIRED = 407;

    /** Request timeout. */
    int SYNCML_DM_REQUEST_TIMEOUT = 408;

    /** Incomplete command. */
    int SYNCML_DM_INCOMPLETE_COMMAND = 412;

    /** Request entity too large. */
    int SYNCML_DM_REQUEST_ENTITY_TOO_LARGE = 413;

    /** URI too long. */
    int SYNCML_DM_URI_TOO_LONG = 414;

    /** Unsupported media type format. TODO: remove if unused. */
    int SYNCML_DM_UNSUPPORTED_MEDIA_TYPE_FORMAT = 415;

    /** Request range not found. TODO: remove if unused. */
    int SYNCML_DM_REQUESTED_RANGE_NOT_FOUND = 416;

    /** The target already exists. */
    int SYNCML_DM_TARGET_ALREADY_EXISTS = 418;

    /** The device storage is full. */
    int SYNCML_DM_DEVICE_FULL = 420;

    /** Size mismatch. */
    int SYNCML_DM_SIZE_MISMATCH = 424;

    /** Permission verification failed. */
    int SYNCML_DM_PERMISSION_FAILED = 425;

    /** Command failed. */
    int SYNCML_DM_COMMAND_FAILED = 500;

    /** Command not implemented. */
    int SYNCML_DM_COMMAND_NOT_IMPLEMENTED = 501;

    /** Processing error. */
    int SYNCML_DM_PROCESSING_ERROR = 506;

    /** Atomic failed. TODO: remove if unused. */
    int SYNCML_DM_ATOMIC_FAILED = 507;

    /** Data store failure. */
    int SYNCML_DM_DATA_STORE_FAILURE = 510;

    /** Operation canceled failure. TODO: remove if unused. */
    int SYNCML_DM_OPERATION_CANCELED_FAILURE = 514;

    /** Atomic rollback failed. */
    int SYNCML_DM_ATOMIC_ROLLBACK_FAILED = 516;

    /** Atomic response too large. */
    int SYNCML_DM_ATOMIC_RESPONSE_TOO_LARGE = 517;

    /** Data set not complete for an ESN. TODO: remove if unused. */
    int SYNCML_DM_ESN_SET_NOT_COMPLETE = 518;

    /** Socket timeout error. TODO: remove if unused. */
    int SYNCML_DM_SOCKET_TIMEOUT = 700;

    /** Socket connect error. TODO: remove if unused. */
    int SYNCML_DM_SOCKET_CONNECT_ERROR = 701;

    /** Factory Data Reset result code. TODO: remove if unused. */
    int SYNCML_DM_PROCESS_ACCEPTED = 1200;
}
