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

/**
 * DmtException is the base for all DMT exceptions.
 */
public class DmtException extends Exception {

    private final int code;

    /**
     * Default Constructor for unspecified message. Not recommended to use.
     */
    public DmtException() {
        code = ErrorCodes.SYNCML_DM_FAIL;
    }

    /**
     * Exception with only message without error code, for backwards
     * compatibility, not recommended to use.
     *
     * @param message the detail message. null means unspecified.
     */
    public DmtException(String message) {
        super(message);
        code = ErrorCodes.SYNCML_DM_FAIL;
    }

    /**
     * Recommended DmtException constructor.
     *
     * @param code error code specified in <A
     *            href="ErrorCodes.html">ErrorCodes</A>. 0 means unspecified.
     * @param message the detail message. null means unspecified.
     */
    public DmtException(int code, String message) {
        super(message);
        this.code = code;
    }

    /**
     * Return error code. if it is 0 it means has not been specified.
     *
     * @return Error code,
     */
    public int getCode() {
        return code;
    }

    /**
     * Returns the error message string. null means unspecified message
     *
     * @return Error message string.
     */
    @Override
    public String getMessage() {
        String s = super.getMessage();
        if (s == null) {
            s = "Unspecified Message";
        }
        return s;
    }
}
