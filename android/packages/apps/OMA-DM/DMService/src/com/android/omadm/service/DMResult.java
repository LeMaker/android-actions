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

package com.android.omadm.service;

public interface DMResult {

    // The DMEngine return code, which must be consistent with
    // engine/dmlib/api/common/dmtError.h

    int SYNCML_DM_SUCCESS = 0;
    int SYNCML_DM_FAIL = 1;
    int SYNCML_DM_ENTRY_EXIST = 2;
    int SYNCML_DM_ENTRY_NOT_EXIST = 3;
    int SYNCML_DM_SESSION_BUSY = 4;
    int SYNCML_DM_INVALID_URI = 5;
    int SYNCML_DM_SESSION_AUTH_FAIL = 6;
    int SYNCML_DM_SESSION_NW_NOT_AVAILABLE = 7;
    int SYNCML_DM_SESSION_NO_CONNECT = 8;
    int SYNCML_DM_SESSION_CANCELED = 9;
    int SYNCML_DM_IO_FAILURE = 10;
    int SYNCML_DM_FILE_NOT_FOUND = 11;
    int SYNCML_DM_URI_CONFLICT = 12;
    int SYNCML_DM_TREE_CORRUPT = 13;
    int SYNCML_DM_SKIP_SUBTREE = 14;
    int SYNCML_DM_RESULTS_TOO_LARGE = 15;
    int SYNCML_DM_LOCK_CTX_NOT_FOUND = 16;
    int SYNCML_DM_UNABLE_START_THREAD = 17;
    int SYNCML_DM_TOO_MANY_DATA_FILES = 18;
    int SYNCML_DM_LOCK_TRY_AGAIN = 19;
    int SYNCML_DM_CONSTRAINT_FAIL = 20;
    int SYNCML_DM_LOAD_ACL_FAIL = 21;
    int SYNCML_DM_TREE_READONLY = 22;
    int SYNCML_DM_INVALID_PARAMETER = 23;
    int SYNCML_DM_AUTHENTICATION_ACCEPTED = 212;
    int SYNCML_DM_CHUNK_BUFFERED = 213;
    int SYNCML_DM_OPERATION_CANCELLED = 214;
    int SYNCML_DM_NOT_EXECUTED = 215;
    int SYNCML_DM_ATOMIC_ROLLBACK_OK = 216;
    int SYNCML_DM_NOT_MODIFIED = 304;
    int SYNCML_DM_BAD_REQUEST = 400;
    int SYNCML_DM_UNAUTHORIZED = 401;
    int SYNCML_DM_NOT_FOUND = 404;
    int SYNCML_DM_COMMAND_NOT_ALLOWED = 405;
    int SYNCML_DM_FEATURE_NOT_SUPPORTED = 406;
    int SYNCML_DM_AUTHENTICATION_REQUIRED = 407;
    int SYNCML_DM_REQUEST_TIMEOUT = 408;
    int SYNCML_DM_INCOMPLETE_COMMAND = 412;
    int SYNCML_DM_REQUEST_ENTITY_TOO_LARGE = 413;
    int SYNCML_DM_URI_TOO_LONG = 414;
    int SYNCML_DM_UNSUPPORTED_MEDIATYPE_FORMAT = 415;
    int SYNCML_DM_REQUESTED_RANGE_NOT_SATISFIABLE = 416;
    int SYNCML_DM_TARGET_ALREADY_EXISTS = 418;
    int SYNCML_DM_DEVICE_FULL = 420;
    int SYNCML_DM_SIZE_MISMATCH = 424;
    int SYNCML_DM_PERMISSION_FAILED = 425;
    int SYNCML_DM_COMMAND_FAILED = 500;
    int SYNCML_DM_COMMAND_NOT_IMPLEMENTED = 501;
    int SYNCML_DM_SERVICE_UNAVAILABLE = 503;
    int SYNCML_DM_GATEWAY_TIMEOUT = 504;
    int SYNCML_DM_PROCESSING_ERROR = 506;
    int SYNCML_DM_ATOMIC_FAILED = 507;
    int SYNCML_DM_DATA_STORE_FAILURE = 510;
    int SYNCML_DM_OPERATION_CANCELLED_FAILURE = 514;
    int SYNCML_DM_ATOMIC_ROLLBACK_FAILED = 516;
    int SYNCML_DM_ATOMIC_RESPONSE_TOO_LARGE = 517;
    int SYNCML_DM_ESN_SET_NOT_COMPLETE = 518;

    int SYNCML_DM_SOCKET_TIMEOUT = 700;
    int SYNCML_DM_SOCKET_CONNECT_ERR = 701;
    int SYNCML_DM_NO_HTTP_RESPONSE = 702;
    int SYNCML_DM_UNKNOWN_HOST = 703;
    int SYNCML_DM_INTERRUPTED = 704;

    int SYNCML_DM_SESSION_PARAM_ERR = 800;
    int SYNCML_DM_SESSION_USER_REJECT = 801;
    int SYNCML_DM_SESSION_NO_FOTA_SERVER_ID = 802;
}
