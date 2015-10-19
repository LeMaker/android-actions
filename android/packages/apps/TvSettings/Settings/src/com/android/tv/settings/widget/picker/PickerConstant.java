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

package com.android.tv.settings.widget.picker;

import android.content.res.Resources;

import com.android.tv.settings.R;

import java.text.DateFormatSymbols;

/**
 * Picker related constants
 */
public class PickerConstant {
    private static PickerConstant sInst;
    private static Object sInstLock = new Object();

    public final String[] months;
    public final String[] days31;
    public final String[] days30;
    public final String[] days29;
    public final String[] days28;
    public final String[] hours12;
    public final String[] hours24;
    public final String[] minutes;
    public final String[] ampm;
    public final String dateSeparator;
    public final String timeSeparator;

    private PickerConstant(Resources resources) {
        // TODO re-init months and ampm if the locale changes
        months = new DateFormatSymbols().getShortMonths();
        days28 = createStringIntArrays(28, false, 2);
        days29 = createStringIntArrays(29, false, 2);
        days30 = createStringIntArrays(30, false, 2);
        days31 = createStringIntArrays(31, false, 2);
        hours12 = createStringIntArrays(12, false, 2);
        hours24 = createStringIntArrays(23, true, 2);
        minutes = createStringIntArrays(59, true, 2);
        ampm = resources.getStringArray(R.array.ampm);
        dateSeparator = resources.getString(R.string.date_separator);
        timeSeparator = resources.getString(R.string.time_separator);
    }


    private String[] createStringIntArrays(int lastNumber, boolean startAtZero, int minLen) {
        int range = startAtZero ? (lastNumber + 1) : lastNumber;
        String format = "%0" + minLen + "d";
        String[] array = new String[range];
        for (int i = 0; i < range; i++) {
            if (minLen > 0) {
                array[i] = String.format(format, startAtZero ? i : (i + 1));
            } else {
                array[i] = String.valueOf(startAtZero ? i : (i + 1));
            }
        }
        return array;
    }

    static public PickerConstant getInstance(Resources resources) {
        synchronized (sInstLock) {
            if (sInst == null) {
                sInst = new PickerConstant(resources);
            }
        }
        return sInst;
    }
}
