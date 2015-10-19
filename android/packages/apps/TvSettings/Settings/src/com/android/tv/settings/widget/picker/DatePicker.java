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

import android.os.Bundle;
import android.view.View;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import android.widget.TextView;

public class DatePicker extends Picker {

    private static final String EXTRA_START_YEAR = "start_year";
    private static final String EXTRA_YEAR_RANGE = "year_range";
    private static final String EXTRA_DEFAULT_TO_CURRENT = "default_to_current";
    private static final String EXTRA_FORMAT = "date_format";
    private static final int DEFAULT_YEAR_RANGE = 24;
    private static final int DEFAULT_START_YEAR = Calendar.getInstance().get(Calendar.YEAR);
    private String[] mYears;
    private int mStartYear;
    private int mYearRange;
    private String[] mDayString = null;
    private int mColMonthIndex = 0;
    private int mColDayIndex = 1;
    private int mColYearIndex = 2;

    private boolean mPendingDate = false;
    private int mInitYear;
    private int mInitMonth;
    private int mInitDay;


    private int mSelectedYear = DEFAULT_START_YEAR;
    private String mSelectedMonth;

    public static DatePicker newInstance() {
        return newInstance("");
    }

    /**
     * Creates a new instance of DatePicker
     *
     * @param format         String containing a permutation of Y, M and D, indicating the order
     *                       of the fields Year, Month and Day to be displayed in the DatePicker.
     */
    public static DatePicker newInstance(String format) {
        return newInstance(format, DEFAULT_START_YEAR);
    }

    /**
     * Creates a new instance of DatePicker
     *
     * @param format         String containing a permutation of Y, M and D, indicating the order
     *                       of the fields Year, Month and Day to be displayed in the DatePicker.
     * @param startYear      The lowest number to be displayed in the Year selector.
     */
    public static DatePicker newInstance(String format, int startYear) {
        return newInstance(format, startYear, DEFAULT_YEAR_RANGE, true);
    }

    /**
     * Creates a new instance of DatePicker
     *
     * @param format         String containing a permutation of Y, M and D, indicating the order
     *                       of the fields Year, Month and Day to be displayed in the DatePicker.
     * @param startYear      The lowest number to be displayed in the Year selector.
     * @param yearRange      Number of entries to be displayed in the Year selector.
     * @param startOnToday   Indicates if the date should be set to the current date by default.
     */
    public static DatePicker newInstance(String format, int startYear, int yearRange,
            boolean startOnToday) {
        DatePicker datePicker = new DatePicker();
        if (startYear <= 0) {
            throw new IllegalArgumentException("The start year must be > 0. Got " + startYear);
        }
        if (yearRange <= 0) {
            throw new IllegalArgumentException("The year range must be > 0. Got " + yearRange);
        }
        Bundle args = new Bundle();
        args.putString(EXTRA_FORMAT, format);
        args.putInt(EXTRA_START_YEAR, startYear);
        args.putInt(EXTRA_YEAR_RANGE, yearRange);
        args.putBoolean(EXTRA_DEFAULT_TO_CURRENT, startOnToday);
        datePicker.setArguments(args);
        return datePicker;
    }

    private void initYearsArray(int startYear, int yearRange) {
        mYears = new String[yearRange];
        for (int i = 0; i < yearRange; i++) {
            mYears[i] = String.format("%d", startYear + i);
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mStartYear = getArguments().getInt(EXTRA_START_YEAR, DEFAULT_START_YEAR);
        mYearRange = getArguments().getInt(EXTRA_YEAR_RANGE, DEFAULT_YEAR_RANGE);
        boolean startOnToday = getArguments().getBoolean(EXTRA_DEFAULT_TO_CURRENT, false);
        mSelectedMonth = mConstant.months[0];
        initYearsArray(mStartYear, mYearRange);

        mDayString = mConstant.days30;

        String format = getArguments().getString(EXTRA_FORMAT);
        if (format != null && !format.isEmpty()) {
            format = format.toUpperCase();

            int yIndex = format.indexOf('Y');
            int mIndex = format.indexOf('M');
            int dIndex = format.indexOf('D');
            if (yIndex < 0 || mIndex < 0 || dIndex < 0 || yIndex > 2 || mIndex > 2 || dIndex > 2) {
                // Badly formatted input. Use default order.
                mColMonthIndex = 0;
                mColDayIndex = 1;
                mColYearIndex = 2;
            } else {
                mColMonthIndex = mIndex;
                mColDayIndex = dIndex;
                mColYearIndex = yIndex;
            }
        }

        if (startOnToday) {
            mPendingDate = true;
            Calendar cal = Calendar.getInstance();
            mInitYear = cal.get(Calendar.YEAR);
            mInitMonth = cal.get(Calendar.MONTH);
            mInitDay = cal.get(Calendar.DATE);
        }
    }

    @Override
    public void onResume() {
        if (mPendingDate) {
            mPendingDate = false;
            setDate(mInitYear, mInitMonth, mInitDay);
        }
        super.onResume();
    }

    @Override
    protected ArrayList<PickerColumn> getColumns() {
        ArrayList<PickerColumn> ret = new ArrayList<PickerColumn>();
        // TODO orders of these columns might need to be localized
        PickerColumn months = new PickerColumn(mConstant.months);
        PickerColumn days = new PickerColumn(mDayString);
        PickerColumn years = new PickerColumn(mYears);

        for (int i = 0; i < 3; i++) {
            if (i == mColYearIndex) {
                ret.add(years);
            } else if (i == mColMonthIndex) {
                ret.add(months);
            } else if (i == mColDayIndex) {
                ret.add(days);
            }
        }

        return ret;
    }

    @Override
    protected String getSeparator() {
        return mConstant.dateSeparator;
    }

    protected boolean setDate(int year, int month, int day) {
        boolean isLeapYear = false;

        if (year < mStartYear || year > (mStartYear + mYearRange)) {
            return false;
        }

        // Test to see if this is a valid date
        try {
            GregorianCalendar cal = new GregorianCalendar(year, month, day);
            cal.setLenient(false);
            Date test = cal.getTime();
        } catch (IllegalArgumentException e) {
            return false;
        }

        mSelectedYear = year;
        mSelectedMonth = mConstant.months[month];

        updateSelection(mColYearIndex, year - mStartYear);
        updateSelection(mColMonthIndex, month);

        String[] dayString = null;
        // This is according to http://en.wikipedia.org/wiki/Leap_year#Algorithm
        if (year % 400 == 0) {
            isLeapYear = true;
        } else if (year % 100 == 0) {
            isLeapYear = false;
        } else if (year % 4 == 0) {
            isLeapYear = true;
        }

        if (month == 1) {
            if (isLeapYear) {
                dayString = mConstant.days29;
            } else {
                dayString = mConstant.days28;
            }
        } else if ((month == 3) || (month == 5) || (month == 8) || (month == 10)) {
            dayString = mConstant.days30;
        } else {
            dayString = mConstant.days31;
        }

        if (mDayString != dayString) {
            mDayString = dayString;
            updateAdapter(mColDayIndex, new PickerColumn(mDayString));
        }

        updateSelection(mColDayIndex, day - 1);
        return true;
    }

    @Override
    public void onScroll(View v) {
        int column = (Integer) v.getTag();
        String text = ((TextView) v).getText().toString();
        if (column == mColMonthIndex) {
            mSelectedMonth = text;
        } else if (column == mColYearIndex) {
            mSelectedYear = Integer.parseInt(text);
        } else {
            return;
        }

        String[] dayString = null;

        boolean isLeapYear = false;
        // This is according to http://en.wikipedia.org/wiki/Leap_year#Algorithm
        if (mSelectedYear % 400 == 0) {
            isLeapYear = true;
        } else if (mSelectedYear % 100 == 0) {
            isLeapYear = false;
        } else if (mSelectedYear % 4 == 0) {
            isLeapYear = true;
        }
        if (mSelectedMonth.equals(mConstant.months[1])) {
            if (isLeapYear) {
                dayString = mConstant.days29;
            } else {
                dayString = mConstant.days28;
            }
        } else if (mSelectedMonth.equals(mConstant.months[3])
                || mSelectedMonth.equals(mConstant.months[5])
                || mSelectedMonth.equals(mConstant.months[8])
                || mSelectedMonth.equals(mConstant.months[10])) {
            dayString = mConstant.days30;
        } else {
            dayString = mConstant.days31;
        }
        if (!mDayString.equals(dayString)) {
            mDayString = dayString;
            updateAdapter(mColDayIndex, new PickerColumn(mDayString));
        }
    }
}
