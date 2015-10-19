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

package com.android.tv.settings.system;

import com.android.tv.settings.ActionBehavior;
import com.android.tv.settings.ActionKey;
import com.android.tv.settings.BaseSettingsActivity;
import com.android.tv.settings.R;
import com.android.tv.settings.util.SettingsHelper;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionAdapter;
import com.android.tv.settings.dialog.old.ActionFragment;
import com.android.tv.settings.dialog.old.ContentFragment;
import com.android.tv.settings.widget.picker.DatePicker;
import com.android.tv.settings.widget.picker.TimePicker;
import com.android.tv.settings.widget.picker.Picker;
import com.android.tv.settings.widget.picker.PickerConstant;

import org.xmlpull.v1.XmlPullParserException;

import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.XmlResourceParser;
import android.os.Bundle;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.text.format.DateFormat;
import android.util.Log;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;

public class DateTimeActivity extends BaseSettingsActivity implements ActionAdapter.Listener {

    private static final String TAG = "DateTimeActivity";
    private static final boolean DEBUG = false;

    private static final String HOURS_12 = "12";
    private static final String HOURS_24 = "24";

    private static final int HOURS_IN_HALF_DAY = 12;

    private static final String XMLTAG_TIMEZONE = "timezone";

    private Calendar mDummyDate;
    private boolean mIsResumed;

    private IntentFilter mIntentFilter;

    private String mNowDate;
    private String mNowTime;

    private ArrayList<Action> mTimeZoneActions;

    private SettingsHelper mHelper;

    /**
     * Flag indicating whether this UpdateView call is from onCreate.
     */
    private boolean mOnCreateUpdateView = false;

    /**
     * Flag indicating whether this UpdateView call is from onResume.
     */
    private boolean mOnResumeUpdateView = false;

    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mIsResumed) {
                switch ((ActionType) mState) {
                    case DATE_TIME_OVERVIEW:
                    case DATE:
                    case TIME:
                        updateTimeAndDateDisplay();
                }
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mDummyDate = Calendar.getInstance();
        mIntentFilter = new IntentFilter();
        mIntentFilter.addAction(Intent.ACTION_TIME_TICK);
        mIntentFilter.addAction(Intent.ACTION_TIME_CHANGED);
        mIntentFilter.addAction(Intent.ACTION_TIMEZONE_CHANGED);
        mIntentFilter.addAction(Intent.ACTION_DATE_CHANGED);
        registerReceiver(mIntentReceiver, mIntentFilter);

        mHelper = new SettingsHelper(getApplicationContext());

        setSampleDate();

        updateTimeAndDateStrings();
        mOnCreateUpdateView = true;

        super.onCreate(savedInstanceState);
    }

    private void setSampleDate() {
        Calendar now = Calendar.getInstance();
        mDummyDate.setTimeZone(now.getTimeZone());
        // We use December 31st because it's unambiguous when demonstrating the date format.
        // We use 15:14 so we can demonstrate the 12/24 hour options.
        mDummyDate.set(now.get(Calendar.YEAR), 11, 31, 15, 14, 0);
    }

    private boolean getAutoState(String name) {
        try {
            return Settings.Global.getInt(getContentResolver(), name) > 0;
        } catch (SettingNotFoundException snfe) {
            return false;
        }
    }

    static void setDate(Context context, int year, int month, int day) {
        Calendar c = Calendar.getInstance();

        c.set(Calendar.YEAR, year);
        c.set(Calendar.MONTH, month);
        c.set(Calendar.DAY_OF_MONTH, day);
        long when = c.getTimeInMillis();

        if (when / 1000 < Integer.MAX_VALUE) {
            ((AlarmManager) context.getSystemService(Context.ALARM_SERVICE)).setTime(when);
        }
    }

    static void setTime(Context context, int hourOfDay, int minute) {
        Calendar c = Calendar.getInstance();

        c.set(Calendar.HOUR_OF_DAY, hourOfDay);
        c.set(Calendar.MINUTE, minute);
        c.set(Calendar.SECOND, 0);
        c.set(Calendar.MILLISECOND, 0);
        long when = c.getTimeInMillis();

        if (when / 1000 < Integer.MAX_VALUE) {
            ((AlarmManager) context.getSystemService(Context.ALARM_SERVICE)).setTime(when);
        }
    }

    private boolean isTimeFormat24h() {
        return DateFormat.is24HourFormat(this);
    }

    private void setTime24Hour(boolean is24Hour) {
        Settings.System.putString(getContentResolver(),
                Settings.System.TIME_12_24,
                is24Hour ? HOURS_24 : HOURS_12);
    }

    private void setAutoDateTime(boolean on) {
        Settings.Global.putInt(getContentResolver(), Settings.Global.AUTO_TIME, on ? 1 : 0);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mIsResumed = true;
        registerReceiver(mIntentReceiver, mIntentFilter);

        mOnResumeUpdateView = true;

        updateTimeAndDateDisplay();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mIsResumed = false;
        unregisterReceiver(mIntentReceiver);
    }

    // Updates the member strings to reflect the current date and time.
    private void updateTimeAndDateStrings() {
        final Calendar now = Calendar.getInstance();
        java.text.DateFormat dateFormat = DateFormat.getDateFormat(this);
        mNowDate = dateFormat.format(now.getTime());
        java.text.DateFormat timeFormat = DateFormat.getTimeFormat(this);
        mNowTime = timeFormat.format(now.getTime());
    }

    @Override
    public void onActionClicked(Action action) {
        /*
         * For list preferences
         */
        final String key = action.getKey();
        switch ((ActionType) mState) {
            case TIME_SET_TIME_ZONE:
                setTimeZone(key);
                updateTimeAndDateStrings();
                goBack();
                return;
        }
        /*
         * For other preferences
         */
        ActionKey<ActionType, ActionBehavior> actionKey = new ActionKey<ActionType, ActionBehavior>(
                ActionType.class, ActionBehavior.class, key);
        final ActionType type = actionKey.getType();
        final ActionBehavior behavior = actionKey.getBehavior();
        if (type == null || behavior == null) {
            // Possible race condition manifested by monkey test.
            Log.e(TAG, "type or behavior is null - exiting  b/17404946");
            return;
        }
        switch (type) {
            case DATE:
            case TIME:
            case TIME_CHOOSE_FORMAT:
            case DATE_SET_DATE:
            case TIME_SET_TIME:
            case TIME_SET_TIME_ZONE:
            case AUTO_DATE_TIME:
                if (behavior == ActionBehavior.INIT) {
                    setState(type, true);
                }
                break;
        }
        switch (behavior) {
            case ON:
                if (mState == ActionType.TIME_CHOOSE_FORMAT) {
                    setTime24Hour(true);
                    updateTimeAndDateStrings();
                } else if (mState == ActionType.AUTO_DATE_TIME) {
                    setAutoDateTime(true);
                }
                goBack();
                break;
            case OFF:
                if (mState == ActionType.TIME_CHOOSE_FORMAT) {
                    setTime24Hour(false);
                    updateTimeAndDateStrings();
                } else if (mState == ActionType.AUTO_DATE_TIME) {
                    setAutoDateTime(false);
                }
                goBack();
                break;
        }
    }

    @Override
    protected Object getInitialState() {
        return ActionType.DATE_TIME_OVERVIEW;
    }

    // Updates the Date and Time entries in the current view, without resetting the
    // Action fragment, so we don't trigger an animation.
    protected void updateTimeAndDateDisplay() {
        updateTimeAndDateStrings();

        if (mActionFragment instanceof ActionFragment) {
            ActionAdapter adapter = (ActionAdapter) ((ActionFragment)mActionFragment).getAdapter();

            if (adapter != null) {
                mActions.clear();

                switch ((ActionType) mState) {
                    case DATE_TIME_OVERVIEW:
                        mActions.add(ActionType.AUTO_DATE_TIME.toAction(mResources,
                                mHelper.getStatusStringFromBoolean(getAutoState(
                                        Settings.Global.AUTO_TIME))));
                        mActions.add(ActionType.DATE.toAction(mResources, mNowDate));
                        mActions.add(ActionType.TIME.toAction(mResources, mNowTime));
                        break;
                    case DATE:
                        mActions.add(ActionType.DATE_SET_DATE.toAction(mResources, mNowDate));
                        break;
                    case TIME:
                        mActions.add(ActionType.TIME_SET_TIME.toAction(mResources, mNowTime));
                        mActions.add(ActionType.TIME_SET_TIME_ZONE.toAction(mResources,
                                getCurrentTimeZoneName()));
                        mActions.add(ActionType.TIME_CHOOSE_FORMAT.toAction(
                                mResources, getTimeFormatDescription()));
                        break;
                }
                adapter.setActions(mActions);
                return;
            }
        }

        // If we don't have an ActionFragment or adapter, fall back to the regular updateView
        updateView();
    }

    @Override
    protected void refreshActionList() {
        mActions.clear();
        boolean autoTime = getAutoState(Settings.Global.AUTO_TIME);
        switch ((ActionType)mState) {
            case DATE_TIME_OVERVIEW:
                mActions.add(ActionType.AUTO_DATE_TIME.toAction(mResources,
                        mHelper.getStatusStringFromBoolean(autoTime)));
                mActions.add(ActionType.DATE.toAction(mResources, mNowDate));
                mActions.add(ActionType.TIME.toAction(mResources, mNowTime));
                break;
            case DATE:
                mActions.add(ActionType.DATE_SET_DATE.toAction(mResources, mNowDate, !autoTime));
                break;
            case TIME:
                mActions.add(ActionType.TIME_SET_TIME.toAction(mResources, mNowTime, !autoTime));
                mActions.add(ActionType.TIME_SET_TIME_ZONE.toAction(
                        mResources, getCurrentTimeZoneName()));
                mActions.add(ActionType.TIME_CHOOSE_FORMAT.toAction(
                        mResources, getTimeFormatDescription()));
                break;
            case TIME_CHOOSE_FORMAT:
                mActions.add(ActionBehavior.ON.toAction(ActionBehavior.getOnKey(
                        ActionType.TIME_CHOOSE_FORMAT.name()), mResources, isTimeFormat24h()));
                mActions.add(ActionBehavior.OFF.toAction(ActionBehavior.getOffKey(
                        ActionType.TIME_CHOOSE_FORMAT.name()), mResources, !isTimeFormat24h()));
                break;
            case TIME_SET_TIME_ZONE:
                mActions = getZoneActions(this);
                break;
            case AUTO_DATE_TIME:
                mActions.add(ActionBehavior.ON.toAction(ActionBehavior.getOnKey(
                        ActionType.AUTO_DATE_TIME.name()), mResources, autoTime));
                mActions.add(ActionBehavior.OFF.toAction(ActionBehavior.getOffKey(
                        ActionType.AUTO_DATE_TIME.name()), mResources, !autoTime));
                break;
            default:
                break;
        }
    }

    private String getTimeFormatDescription() {
        String status = mHelper.getStatusStringFromBoolean(isTimeFormat24h());
        String desc = String.format("%s (%s)", status,
                DateFormat.getTimeFormat(this).format(mDummyDate.getTime()));
        return desc;
    }

    @Override
    protected void updateView() {
        refreshActionList();

        switch ((ActionType) mState) {
            case DATE_TIME_OVERVIEW:
                if (mOnCreateUpdateView && mOnResumeUpdateView) {
                    // If current updateView call is due to onResume following onCreate,
                    // avoid duplicate setView, which will lead to broken animation.
                    mOnCreateUpdateView = false;
                    mOnResumeUpdateView = false;

                    return;
                } else {
                    mOnResumeUpdateView = false;
                }
                mActionFragment = ActionFragment.newInstance(mActions);
                break;
            case DATE_SET_DATE:
                DatePicker datePicker =
                        DatePicker.newInstance(new String(DateFormat.getDateFormatOrder(this)));
                datePicker.setResultListener(new Picker.ResultListener() {

                    @Override
                    public void onCommitResult(List<String> result) {
                        String formatOrder = new String(
                                DateFormat.getDateFormatOrder(DateTimeActivity.this)).toUpperCase();
                        int yIndex = formatOrder.indexOf('Y');
                        int mIndex = formatOrder.indexOf('M');
                        int dIndex = formatOrder.indexOf('D');
                        if (yIndex < 0 || mIndex < 0 || dIndex < 0 ||
                                yIndex > 2 || mIndex > 2 || dIndex > 2) {
                            // Badly formatted input. Use default order.
                            mIndex = 0;
                            dIndex = 1;
                            yIndex = 2;
                        }
                        String month = result.get(mIndex);
                        int day = Integer.parseInt(result.get(dIndex));
                        int year = Integer.parseInt(result.get(yIndex));
                        int monthInt = 0;
                        String[] months = PickerConstant.getInstance(mResources).months;
                        int totalMonths = months.length;
                        for (int i = 0; i < totalMonths; i++) {
                            if (months[i].equals(month)) {
                                monthInt = i;
                            }
                        }

                        // turn off Auto date/time
                        setAutoDateTime(false);

                        setDate(DateTimeActivity.this, year, monthInt, day);
                        goBack();
                    }
                });
                mActionFragment = datePicker;
                break;
            case TIME_SET_TIME:
                Picker timePicker = TimePicker.newInstance(isTimeFormat24h(), true);
                timePicker.setResultListener(new Picker.ResultListener() {

                    @Override
                    public void onCommitResult(List<String> result) {
                        boolean is24hFormat = isTimeFormat24h();
                        int hour = Integer.parseInt(result.get(0));
                        int minute = Integer.parseInt(result.get(1));
                        if (!is24hFormat) {
                            String ampm = result.get(2);
                            if (ampm.equals(getResources().getStringArray(R.array.ampm)[1])) {
                                // PM case, valid hours: 12-23
                                hour = (hour % HOURS_IN_HALF_DAY) + HOURS_IN_HALF_DAY;
                            } else {
                                // AM case, valid hours: 0-11
                                hour = hour % HOURS_IN_HALF_DAY;
                            }
                        }

                        // turn off Auto date/time
                        setAutoDateTime(false);

                        setTime(DateTimeActivity.this, hour, minute);
                        goBack();
                    }
                });
                mActionFragment = timePicker;
                break;
            default:
                mActionFragment = ActionFragment.newInstance(mActions);
                break;
        }

        setViewWithActionFragment(
                ((ActionType) mState).getTitle(mResources), getPrevState() != null ?
                        ((ActionType) getPrevState()).getTitle(mResources)
                        : getString(R.string.settings_app_name),
                ((ActionType) mState).getDescription(mResources), R.drawable.ic_settings_datetime);
    }

    protected void setViewWithActionFragment(String title, String breadcrumb, String description,
            int iconResId) {
        mContentFragment = ContentFragment.newInstance(title, breadcrumb, description, iconResId,
                getResources().getColor(R.color.icon_background));
        setContentAndActionFragments(mContentFragment, mActionFragment);
    }

    @Override
    protected void setProperty(boolean enable) {
    }

    /**
     * Returns a string representing the current time zone set in the system.
     */
    private String getCurrentTimeZoneName() {
        final Calendar now = Calendar.getInstance();
        TimeZone tz = now.getTimeZone();

        Date date = new Date();
        return formatOffset(new StringBuilder(), tz.getOffset(date.getTime())).
                append(", ").
                append(tz.getDisplayName(tz.inDaylightTime(date), TimeZone.LONG)).toString();
    }

    /**
     * Formats the provided timezone offset into a string of the form GMT+XX:XX
     */
    private static StringBuilder formatOffset(StringBuilder sb, long offset) {
        long off = offset / 1000 / 60;

        sb.append("GMT");
        if (off < 0) {
            sb.append('-');
            off = -off;
        } else {
            sb.append('+');
        }

        int hours = (int) (off / 60);
        int minutes = (int) (off % 60);

        sb.append((char) ('0' + hours / 10));
        sb.append((char) ('0' + hours % 10));

        sb.append(':');

        sb.append((char) ('0' + minutes / 10));
        sb.append((char) ('0' + minutes % 10));

        return sb;
    }

    /**
     * Helper class to hold the time zone data parsed from the Time Zones XML
     * file.
     */
    private class TimeZoneInfo implements Comparable<TimeZoneInfo> {
        public String tzId;
        public String tzName;
        public long tzOffset;

        public TimeZoneInfo(String id, String name, long offset) {
            tzId = id;
            tzName = name;
            tzOffset = offset;
        }

        @Override
        public int compareTo(TimeZoneInfo another) {
            return (int) (tzOffset - another.tzOffset);
        }
    }

    /**
     * Parse the Time Zones information from the XML file and creates Action
     * objects for each time zone.
     */
    private ArrayList<Action> getZoneActions(Context context) {
        if (mTimeZoneActions != null && mTimeZoneActions.size() != 0) {
            return mTimeZoneActions;
        }

        ArrayList<TimeZoneInfo> timeZones = getTimeZones(context);

        mTimeZoneActions = new ArrayList<Action>();

        // Sort the Time Zones list in ascending offset order
        Collections.sort(timeZones);

        TimeZone currentTz = TimeZone.getDefault();

        for (TimeZoneInfo tz : timeZones) {
            StringBuilder name = new StringBuilder();
            boolean checked = currentTz.getID().equals(tz.tzId);
            mTimeZoneActions.add(getTimeZoneAction(tz.tzId, tz.tzName,
                    formatOffset(name, tz.tzOffset).toString(), checked));
        }

        return mTimeZoneActions;
    }

    /**
     * Parses the XML time zone information into an array of TimeZoneInfo
     * objects.
     */
    private ArrayList<TimeZoneInfo> getTimeZones(Context context) {
        ArrayList<TimeZoneInfo> timeZones = new ArrayList<TimeZoneInfo>();
        final long date = Calendar.getInstance().getTimeInMillis();
        try {
            XmlResourceParser xrp = context.getResources().getXml(R.xml.timezones);
            while (xrp.next() != XmlResourceParser.START_TAG)
                continue;
            xrp.next();
            while (xrp.getEventType() != XmlResourceParser.END_TAG) {
                while (xrp.getEventType() != XmlResourceParser.START_TAG &&
                        xrp.getEventType() != XmlResourceParser.END_DOCUMENT) {
                    xrp.next();
                }

                if (xrp.getEventType() == XmlResourceParser.END_DOCUMENT) {
                    break;
                }

                if (xrp.getName().equals(XMLTAG_TIMEZONE)) {
                    String id = xrp.getAttributeValue(0);
                    String displayName = xrp.nextText();
                    TimeZone tz = TimeZone.getTimeZone(id);
                    long offset;
                    if (tz != null) {
                        offset = tz.getOffset(date);
                        timeZones.add(new TimeZoneInfo(id, displayName, offset));
                    } else {
                        continue;
                    }
                }
                while (xrp.getEventType() != XmlResourceParser.END_TAG) {
                    xrp.next();
                }
                xrp.next();
            }
            xrp.close();
        } catch (XmlPullParserException xppe) {
            Log.e(TAG, "Ill-formatted timezones.xml file");
        } catch (java.io.IOException ioe) {
            Log.e(TAG, "Unable to read timezones.xml file");
        }
        return timeZones;
    }

    private static Action getTimeZoneAction(String tzId, String displayName, String gmt,
            boolean setChecked) {
        return new Action.Builder().key(tzId).title(displayName).description(gmt).
                checked(setChecked).build();
    }

    private void setTimeZone(String tzId) {
        // Update the system timezone value
        final AlarmManager alarm = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
        alarm.setTimeZone(tzId);

        setSampleDate();
    }
}
