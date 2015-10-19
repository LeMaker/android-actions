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

package com.android.compatibility.common.util;

import org.xmlpull.v1.XmlSerializer;

import java.io.IOException;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

/**
 * Utility class to add results to the report.
 */
public abstract class ReportLog implements Serializable {

    private Result mSummary;
    private final List<Result> mDetails = new ArrayList<Result>();

    class Result implements Serializable {
        private static final int CALLER_STACKTRACE_DEPTH = 5;
        private String mLocation;
        private String mMessage;
        private double[] mValues;
        private ResultType mType;
        private ResultUnit mUnit;

        /**
         * Creates a result object to be included in the report. Each object has a message
         * describing its values and enums to interpret them. In addition, each result also includes
         * class, method and line number information about the test which added this result which is
         * collected by looking at the stack trace.
         *
         * @param message A string describing the values
         * @param values An array of the values
         * @param type Represents how to interpret the values (eg. A lower score is better)
         * @param unit Represents the unit in which the values are (eg. Milliseconds)
         * @param depth A number used to increase the depth the stack is queried. This should only
         * be given in the case that the report is populated by a helper function, in which case it
         * would be 1, or else 0.
         */
        private Result(String message, double[] values, ResultType type,
                ResultUnit unit, int depth) {
            final StackTraceElement[] trace = Thread.currentThread().getStackTrace();
            final StackTraceElement e =
                    trace[Math.min(CALLER_STACKTRACE_DEPTH + depth, trace.length - 1)];
            mLocation = String.format(
                    "%s#%s:%d", e.getClassName(), e.getMethodName(), e.getLineNumber());
            mMessage = message;
            mValues = values;
            mType = type;
            mUnit = unit;
        }

        public String getLocation() {
            return mLocation;
        }

        public String getMessage() {
            return mMessage;
        }

        public double[] getValues() {
            return mValues;
        }

        public ResultType getType() {
            return mType;
        }

        public ResultUnit getUnit() {
            return mUnit;
        }
    }

    /**
     * Adds an array of values to the report.
     */
    public void addValues(String message, double[] values, ResultType type, ResultUnit unit) {
        mDetails.add(new Result(message, values, type, unit, 0));
    }

    /**
     * Adds an array of values to the report.
     */
    public void addValues(String message, double[] values, ResultType type,
            ResultUnit unit, int depth) {
        mDetails.add(new Result(message, values, type, unit, depth));
    }

    /**
     * Adds a value to the report.
     */
    public void addValue(String message, double value, ResultType type, ResultUnit unit) {
        mDetails.add(new Result(message, new double[] {value}, type, unit, 0));
    }

    /**
     * Adds a value to the report.
     */
    public void addValue(String message, double value, ResultType type,
            ResultUnit unit, int depth) {
        mDetails.add(new Result(message, new double[] {value}, type, unit, depth));
    }

    /**
     * Sets the summary of the report.
     */
    public void setSummary(String message, double value, ResultType type, ResultUnit unit) {
        mSummary = new Result(message, new double[] {value}, type, unit, 0);
    }

    /**
     * Sets the summary of the report.
     */
    public void setSummary(String message, double value, ResultType type,
            ResultUnit unit, int depth) {
        mSummary = new Result(message, new double[] {value}, type, unit, depth);
    }

    public Result getSummary() {
        return mSummary;
    }

    public List<Result> getDetailedMetrics() {
        return new ArrayList<Result>(mDetails);
    }
}
