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

package com.android.tv.settings.name;

import android.app.Fragment;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;

import com.android.tv.settings.R;
import com.android.tv.settings.widget.ScrollAdapterView;
import com.android.tv.settings.widget.ScrollArrayAdapter;

public class SetDeviceNameFragment extends Fragment implements AdapterView.OnItemClickListener {

    private static final String ARG_NAME_LIST = "suggested_names";
    private static final String ARG_SHOW_CUSTOM_OPTION = "allow_custom";
    private static final int LAYOUT_MAIN = R.layout.setup_content_area;
    private static final int LAYOUT_DESCRIPTION = R.layout.setup_text_and_description;
    private static final int LAYOUT_ACTION = R.layout.setup_scroll_adapter_view;
    private static final int LAYOUT_LIST_ITEM = R.layout.setup_list_item_text_only;
    private static final int LAYOUT_ITEM_TEXT = R.id.list_item_text;

    public static SetDeviceNameFragment createInstance(ArrayList<String> names,
                                                    boolean allowCustom) {
        SetDeviceNameFragment frag = new SetDeviceNameFragment();
        Bundle args = new Bundle();
        args.putStringArrayList(SetDeviceNameFragment.ARG_NAME_LIST, names);
        args.putBoolean(SetDeviceNameFragment.ARG_SHOW_CUSTOM_OPTION, allowCustom);
        frag.setArguments(args);
        return frag;
    }

    private SetDeviceNameListener mEventListener;
    private ArrayList<String> mOptions;
    private boolean mShowCustom;
    private View mContent;
    private FrameLayout mDescription;
    private FrameLayout mAction;
    private ScrollAdapterView mList;
    private ScrollArrayAdapter<String> mListAdapter;
    private String mCustomRoomString = "";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Bundle args = getArguments();
        if (args.containsKey(ARG_NAME_LIST)) {
            mOptions = args.getStringArrayList(ARG_NAME_LIST);
        } else {
            mOptions = new ArrayList<String>();
        }

        mShowCustom = args.getBoolean(ARG_SHOW_CUSTOM_OPTION, false);

        if (mShowCustom) {
            mCustomRoomString = getResources().getString(R.string.custom_room);
            mOptions.add(mCustomRoomString);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        mContent = inflater.inflate(LAYOUT_MAIN, null);
        mAction = (FrameLayout) mContent.findViewById(R.id.action);
        mDescription = (FrameLayout) mContent.findViewById(R.id.description);
        mDescription.addView(inflater.inflate(LAYOUT_DESCRIPTION, null));
        String title = getResources().getString(R.string.select_title);
        ((TextView) mDescription.findViewById(R.id.title_text))
                .setText(TextUtils.expandTemplate(title, Build.MODEL));
        String description = getResources().getString(R.string.select_description);
        ((TextView) mDescription.findViewById(R.id.description_text))
                .setText(TextUtils.expandTemplate(description, Build.MODEL));
        mList = (ScrollAdapterView) inflater.inflate(LAYOUT_ACTION, null);
        mAction.addView(mList);
        setupList();
        return mContent;
    }

    public void setListener(SetDeviceNameListener listener) {
        mEventListener = listener;
    }

    @Override
    public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
        if (isCustomListItem(position)) {
            mEventListener.onCustomNameRequested();
        } else {
            mEventListener.onDeviceNameSelected(mOptions.get(position));
        }
    }

    private void setupList() {
        mListAdapter = new ScrollArrayAdapter<String>(getActivity(), LAYOUT_LIST_ITEM,
                LAYOUT_ITEM_TEXT, mOptions) {
            private static final int VIEW_TYPE_TEXT = 0;
            private static final int VIEW_TYPE_TEXT_AND_ICON = 1;
            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                // for a "standard" item, return standard view
                if (!isCustomListItem(position)) {
                    return super.getView(position, convertView, parent);
                }

                // for the "other option" draw a custom view that includes an icon
                if (convertView == null) {
                    LayoutInflater helium = (LayoutInflater) getActivity().getSystemService(
                            Context.LAYOUT_INFLATER_SERVICE);
                    convertView = helium.inflate(R.layout.setup_list_item, null);

                    // our image view is always going to be the same, so set that here
                    ImageView plusIcon = new ImageView(getActivity());
                    plusIcon.setImageResource(R.drawable.ic_menu_add);

                    ((FrameLayout) convertView.findViewById(R.id.list_item_icon)).addView(plusIcon);
                }

                TextView itemLabel = (TextView) convertView.findViewById(R.id.list_item_text);
                itemLabel.setText(mOptions.get(position));
                return convertView;
            }

            @Override
            public int getViewTypeCount() {
                return 2;
            }

            @Override
            public int getItemViewType(int position) {
                return mOptions.get(position).equals(mCustomRoomString) ?
                        VIEW_TYPE_TEXT_AND_ICON : VIEW_TYPE_TEXT;
            }
        };
        mList.setAdapter(mListAdapter);
        mList.setOnItemClickListener(this);
    }

    private boolean isCustomListItem(int position) {
        return mOptions.get(position).equals(mCustomRoomString);
    }
}
