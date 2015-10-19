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

package android.theme.app;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Canvas;
import android.os.AsyncTask;
import android.os.Environment;
import android.os.Bundle;
import android.os.Handler;
import android.theme.app.modifiers.DatePickerModifier;
import android.theme.app.modifiers.ProgressBarModifier;
import android.theme.app.modifiers.SearchViewModifier;
import android.theme.app.modifiers.TimePickerModifier;
import android.theme.app.modifiers.ViewCheckedModifier;
import android.theme.app.modifiers.ViewPressedModifier;
import android.theme.app.R;
import android.theme.app.ReferenceViewGroup;
import android.util.Log;
import android.view.View;
import android.widget.CheckBox;
import android.widget.LinearLayout;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.Override;

/**
 * A activity which display various UI elements with Holo theme.
 */
public class HoloDeviceActivity extends Activity {

    public static final String EXTRA_THEME = "holo_theme_extra";

    public static final String EXTRA_LAYOUT = "holo_layout_extra";

    public static final String EXTRA_TIMEOUT = "holo_timeout_extra";

    private static final String TAG = HoloDeviceActivity.class.getSimpleName();

    private static final int TIMEOUT = 1 * 1000;//1 sec

    private View mView;

    private String mName;

    private Bitmap mBitmap;

    private ReferenceViewGroup mViewGroup;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setUpUi(getIntent());
    }

    @Override
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setUpUi(intent);
    }

    /**
     * Configures the UI with the given intent
     */
    private void setUpUi(Intent intent) {
        final Theme theme = themes[intent.getIntExtra(EXTRA_THEME, 0)];
        final Layout layout = layouts[intent.getIntExtra(EXTRA_LAYOUT, 0)];
        final int timeout = intent.getIntExtra(EXTRA_TIMEOUT, TIMEOUT);

        setTheme(theme.mId);
        setContentView(R.layout.holo_test);

        mViewGroup = (ReferenceViewGroup) findViewById(R.id.reference_view_group);

        mView = getLayoutInflater().inflate(layout.mId, mViewGroup, false);
        mViewGroup.addView(mView);
        if (layout.mModifier != null) {
            layout.mModifier.modifyView(mView);
        }
        mViewGroup.measure(0, 0);
        mViewGroup.layout(0, 0, mViewGroup.getMeasuredWidth(), mViewGroup.getMeasuredHeight());
        mView.setFocusable(false);
        mName = String.format("%s_%s", theme.mName, layout.mName);

        final Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                new GenerateBitmapTask().execute();
            }
        }, timeout);
        setResult(RESULT_CANCELED);//On success will be changed to OK
    }

    /**
     * A task which gets the UI element to render to a bitmap and then saves that as a png
     * asynchronously
     */
    private class GenerateBitmapTask extends AsyncTask<Void, Void, Boolean> {

        @Override
        protected void onPreExecute() {
            final View v = mView;
            mBitmap = Bitmap.createBitmap(v.getWidth(), v.getHeight(), Bitmap.Config.ARGB_8888);
            final Canvas canvas = new Canvas(mBitmap);
            v.draw(canvas);
        }

        @Override
        protected Boolean doInBackground(Void... ignored) {
            if (!Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                Log.i(TAG, "External storage for saving bitmaps is not mounted");
                return false;
            }
            final File dir = new File(Environment.getExternalStorageDirectory(), "cts-holo-assets");
            dir.mkdirs();
            boolean success = false;
            try {
                final File file = new File(dir, mName + ".png");
                FileOutputStream stream = null;
                try {
                    stream = new FileOutputStream(file);
                    mBitmap.compress(CompressFormat.PNG, 100, stream);
                } finally {
                    if (stream != null) {
                        stream.close();
                    }
                }
                success = true;
            } catch (Exception e) {
                Log.e(TAG, e.getMessage());
            } finally {
                mBitmap.recycle();
                mBitmap = null;
            }
            return success;
        }

        @Override
        protected void onPostExecute(Boolean success) {
            Log.i(TAG, (success ? "OKAY" : "ERROR") + ":" + mName);
            setResult(RESULT_OK);
            finish();
        }
    }

    /**
     * A class to encapsulate information about a holo theme.
     */
    private static class Theme {

        public final int mId;

        public final String mName;

        private Theme(int id, String name) {
            mId = id;
            mName = name;
        }
    }

    private static final Theme[] themes = {
            new Theme(android.R.style.Theme_Holo,
                    "holo"),
            new Theme(android.R.style.Theme_Holo_Dialog,
                    "holo_dialog"),
            new Theme(android.R.style.Theme_Holo_Dialog_MinWidth,
                    "holo_dialog_minwidth"),
            new Theme(android.R.style.Theme_Holo_Dialog_NoActionBar,
                    "holo_dialog_noactionbar"),
            new Theme(android.R.style.Theme_Holo_Dialog_NoActionBar_MinWidth,
                    "holo_dialog_noactionbar_minwidth"),
            new Theme(android.R.style.Theme_Holo_DialogWhenLarge,
                    "holo_dialogwhenlarge"),
            new Theme(android.R.style.Theme_Holo_DialogWhenLarge_NoActionBar,
                    "holo_dialogwhenlarge_noactionbar"),
            new Theme(android.R.style.Theme_Holo_InputMethod,
                    "holo_inputmethod"),
            new Theme(android.R.style.Theme_Holo_Light,
                    "holo_light"),
            new Theme(android.R.style.Theme_Holo_Light_DarkActionBar,
                    "holo_light_darkactionbar"),
            new Theme(android.R.style.Theme_Holo_Light_Dialog,
                    "holo_light_dialog"),
            new Theme(android.R.style.Theme_Holo_Light_Dialog_MinWidth,
                    "holo_light_dialog_minwidth"),
            new Theme(android.R.style.Theme_Holo_Light_Dialog_NoActionBar,
                    "holo_light_dialog_noactionbar"),
            new Theme(android.R.style.Theme_Holo_Light_Dialog_NoActionBar_MinWidth,
                    "holo_light_dialog_noactionbar_minwidth"),
            new Theme(android.R.style.Theme_Holo_Light_DialogWhenLarge,
                    "holo_light_dialogwhenlarge"),
            new Theme(android.R.style.Theme_Holo_Light_DialogWhenLarge_NoActionBar,
                    "holo_light_dialogwhenlarge_noactionbar"),
            new Theme(android.R.style.Theme_Holo_Light_NoActionBar,
                    "holo_light_noactionbar"),
            new Theme(android.R.style.Theme_Holo_Light_NoActionBar_Fullscreen,
                    "holo_light_noactionbar_fullscreen"),
            new Theme(android.R.style.Theme_Holo_Light_Panel,
                    "holo_light_panel"),
            new Theme(android.R.style.Theme_Holo_NoActionBar,
                    "holo_noactionbar"),
            new Theme(android.R.style.Theme_Holo_NoActionBar_Fullscreen,
                    "holo_noactionbar_fullscreen"),
            new Theme(android.R.style.Theme_Holo_Panel,
                    "holo_panel"),
            new Theme(android.R.style.Theme_Holo_Wallpaper,
                    "holo_wallpaper"),
            new Theme(android.R.style.Theme_Holo_Wallpaper_NoTitleBar,
                    "holo_wallpaper_notitlebar")
    };

    /**
     * A class to encapsulate information about a holo layout.
     */
    private static class Layout {

        public final int mId;

        public final String mName;

        public final LayoutModifier mModifier;

        private Layout(int id, String name, LayoutModifier modifier) {
            mId = id;
            mName = name;
            mModifier = modifier;
        }
    }

    private static final Layout[] layouts = {
            new Layout(R.layout.button, "button", null),
            new Layout(R.layout.button, "button_pressed", new ViewPressedModifier()),
            new Layout(R.layout.checkbox, "checkbox", null),
            new Layout(R.layout.checkbox, "checkbox_checked", new ViewCheckedModifier()),
            new Layout(R.layout.chronometer, "chronometer", null),
            new Layout(R.layout.color_blue_bright, "color_blue_bright", null),
            new Layout(R.layout.color_blue_dark, "color_blue_dark", null),
            new Layout(R.layout.color_blue_light, "color_blue_light", null),
            new Layout(R.layout.color_green_dark, "color_green_dark", null),
            new Layout(R.layout.color_green_light, "color_green_light", null),
            new Layout(R.layout.color_orange_dark, "color_orange_dark", null),
            new Layout(R.layout.color_orange_light, "color_orange_light", null),
            new Layout(R.layout.color_purple, "color_purple", null),
            new Layout(R.layout.color_red_dark, "color_red_dark", null),
            new Layout(R.layout.color_red_light, "color_red_light", null),
            new Layout(R.layout.datepicker, "datepicker", new DatePickerModifier()),
            new Layout(R.layout.display_info, "display_info", null),
            new Layout(R.layout.edittext, "edittext", null),
            new Layout(R.layout.progressbar_horizontal_0, "progressbar_horizontal_0", null),
            new Layout(R.layout.progressbar_horizontal_100, "progressbar_horizontal_100", null),
            new Layout(R.layout.progressbar_horizontal_50, "progressbar_horizontal_50", null),
            new Layout(R.layout.progressbar_large, "progressbar_large", new ProgressBarModifier()),
            new Layout(R.layout.progressbar_small, "progressbar_small", new ProgressBarModifier()),
            new Layout(R.layout.progressbar, "progressbar", new ProgressBarModifier()),
            new Layout(R.layout.radiobutton_checked, "radiobutton_checked", null),
            new Layout(R.layout.radiobutton, "radiobutton", null),
            new Layout(R.layout.radiogroup_horizontal, "radiogroup_horizontal", null),
            new Layout(R.layout.radiogroup_vertical, "radiogroup_vertical", null),
            new Layout(R.layout.ratingbar_0, "ratingbar_0", null),
            new Layout(R.layout.ratingbar_2point5, "ratingbar_2point5", null),
            new Layout(R.layout.ratingbar_5, "ratingbar_5", null),
            new Layout(R.layout.ratingbar_0, "ratingbar_0_pressed", new ViewPressedModifier()),
            new Layout(R.layout.ratingbar_2point5, "ratingbar_2point5_pressed", new ViewPressedModifier()),
            new Layout(R.layout.ratingbar_5, "ratingbar_5_pressed", new ViewPressedModifier()),
            new Layout(R.layout.searchview, "searchview_query", new SearchViewModifier(SearchViewModifier.QUERY)),
            new Layout(R.layout.searchview, "searchview_query_hint", new SearchViewModifier(SearchViewModifier.QUERY_HINT)),
            new Layout(R.layout.seekbar_0, "seekbar_0", null),
            new Layout(R.layout.seekbar_100, "seekbar_100", null),
            new Layout(R.layout.seekbar_50, "seekbar_50", null),
            new Layout(R.layout.spinner, "spinner", null),
            new Layout(R.layout.switch_button_checked, "switch_button_checked", null),
            new Layout(R.layout.switch_button, "switch_button", null),
            new Layout(R.layout.textview, "textview", null),
            new Layout(R.layout.timepicker, "timepicker", new TimePickerModifier()),
            new Layout(R.layout.togglebutton_checked, "togglebutton_checked", null),
            new Layout(R.layout.togglebutton, "togglebutton", null),
            new Layout(R.layout.zoomcontrols, "zoomcontrols", null),
    };
}
