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

package com.android.tv.settings.device.storage;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;
import android.os.Parcelable;
import android.text.format.Formatter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.tv.settings.dialog.old.BaseContentFragment;
import com.android.tv.settings.dialog.old.ContentFragment;
import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Fragment that shows storage info.
 */
public class StorageContentFragment extends ContentFragment {
    private View mView;

    public static StorageContentFragment newInstance(String title,
            String breadcrumb, String description, int iconResourceId, int iconBackgroundColor) {
        StorageContentFragment fragment = new StorageContentFragment();
        fragment.setArguments(BaseContentFragment.buildArgs(title, breadcrumb, description,
                iconResourceId, iconBackgroundColor));
        return fragment;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return (mView = super.onCreateView(inflater, container, savedInstanceState));
    }

    public void updateEntries(ArrayList<PercentageBarChart.Entry> entries) {
        setIcon(buildPercentageBarChart(entries));
    }

    private PercentageBarChart buildPercentageBarChart(ArrayList<PercentageBarChart.Entry> entries)
    {
        return new PercentageBarChart(entries,
                getColor(R.color.storage_avail), getPixelSize(R.dimen.storage_bar_min_tick_width),
                getPixelSize(R.dimen.content_fragment_icon_width),
                getPixelSize(R.dimen.content_fragment_icon_width), mView.isLayoutRtl());
    }

    private int getColor(int resource) {
        return getActivity().getResources().getColor(resource);
    }

    private int getPixelSize(int resource) {
        return getActivity().getResources().getDimensionPixelSize(resource);
    }
}
