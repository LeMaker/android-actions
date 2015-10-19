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

package com.android.tv.settings.device.apps;

import com.android.tv.settings.R;

import android.app.Fragment;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AppSecurityPermissions;

/**
 * Fragment that shows the app permissions.
 */
public class PermissionsFragment extends Fragment {

    private static final String PACKAGE_NAME_KEY = "packageName";

    static PermissionsFragment newInstance(String packageName) {

        PermissionsFragment f = new PermissionsFragment();
        Bundle args = new Bundle();
        args.putString(PACKAGE_NAME_KEY, packageName);
        f.setArguments(args);
        return f;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

        View content = inflater.inflate(R.layout.device_apps_app_management_permissions, null);

        AppSecurityPermissions asp = new AppSecurityPermissions(getActivity(), getPackageName());
        if (asp.getPermissionCount() > 0 && content instanceof ViewGroup) {
            ViewGroup vg = (ViewGroup) content;
            vg.removeAllViews();
            vg.addView(asp.getPermissionsView());
        }

        return content;
    }

    private String getPackageName() {
        return getArguments().getString(PACKAGE_NAME_KEY);
    }
}
