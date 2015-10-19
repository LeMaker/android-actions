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

package com.android.tv.settings.base;

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.text.InputType;
import android.text.method.PasswordTransformationMethod;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.TextView;

import com.android.tv.settings.R;
import com.android.tv.settings.util.AccessibilityHelper;

/**
 * Displays a UI for text input in the "wizard" style.
 * TODO: Merge with EditTextFragment
 */
public class TextInputWizardFragment extends Fragment {

    public interface Listener {
        /**
         * Called when text input is complete.
         *
         * @param text the text that was input.
         * @return true if the text is acceptable; false if not.
         */
        boolean onTextInputComplete(String text);
    }

    private static final String EXTRA_TITLE = "title";
    private static final String EXTRA_IS_PASSWORD = "is_password";
    private static final String EXTRA_PREFILL = "prefill";
    private static final String EXTRA_IS_NUMERIC = "is_numeric";

    public static TextInputWizardFragment newInstance(String title, boolean isPassword,
            String prefill) {
        return newInstance(title, isPassword, prefill, false);
    }

    public static TextInputWizardFragment newInstance(String title, boolean isPassword,
            String prefill, boolean isNumeric) {
        TextInputWizardFragment fragment = new TextInputWizardFragment();
        Bundle args = new Bundle();
        args.putString(EXTRA_TITLE, title);
        args.putBoolean(EXTRA_IS_PASSWORD, isPassword);
        args.putString(EXTRA_PREFILL, prefill);
        args.putBoolean(EXTRA_IS_NUMERIC, isNumeric);
        fragment.setArguments(args);
        return fragment;
    }

    private Handler mHandler;
    private EditText mTextInput;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle icicle) {
        mHandler = new Handler();
        View view = inflater.inflate(R.layout.setup_content_area, container, false);
        View content = inflater.inflate(R.layout.wifi_content, null);
        View action = inflater.inflate(R.layout.wifi_text_input, null);
        ((ViewGroup) view.findViewById(R.id.description)).addView(content);
        ((ViewGroup) view.findViewById(R.id.action)).addView(action);

        TextView titleText = (TextView) content.findViewById(R.id.title_text);
        mTextInput = (EditText) action.findViewById(R.id.text_input);

        Bundle args = getArguments();
        String title = args.getString(EXTRA_TITLE);
        boolean isPassword = args.getBoolean(EXTRA_IS_PASSWORD);
        String prefill = args.getString(EXTRA_PREFILL);
        boolean isNumeric = args.getBoolean(EXTRA_IS_NUMERIC);

        if (title != null) {
            titleText.setText(title);
            titleText.setVisibility(View.VISIBLE);
            if (AccessibilityHelper.forceFocusableViews(getActivity())) {
                titleText.setFocusable(true);
                titleText.setFocusableInTouchMode(true);
            }
        } else {
            titleText.setVisibility(View.GONE);
        }

        if (isNumeric) {
            mTextInput.setInputType(InputType.TYPE_CLASS_NUMBER);
        }

        if (isPassword) {
            mTextInput.setTransformationMethod(new PasswordTransformationMethod());
        }

        if (prefill != null) {
            mTextInput.setText(prefill);
            mTextInput.setSelection(mTextInput.getText().length(), mTextInput.getText().length());
        }

        mTextInput.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (event == null || event.getAction() == KeyEvent.ACTION_UP) {
                    Activity a = getActivity();
                    if (a instanceof Listener) {
                        return ((Listener) a).onTextInputComplete(v.getText().toString());
                    }
                    return false;
                }
                return true;  // If we don't return true on ACTION_DOWN, we don't get the ACTION_UP.
            }
        });

        return view;
    }

    @Override
    public void onResume() {
        super.onResume();
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                InputMethodManager inputMethodManager = (InputMethodManager) getActivity()
                    .getSystemService(Context.INPUT_METHOD_SERVICE);
                inputMethodManager.showSoftInput(mTextInput, 0);
                mTextInput.requestFocus();
            }
        });
    }
}
