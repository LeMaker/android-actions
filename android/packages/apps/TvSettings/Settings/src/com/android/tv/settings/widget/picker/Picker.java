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

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.TextView;

import com.android.tv.settings.widget.ScrollAdapterView;
import com.android.tv.settings.widget.ScrollArrayAdapter;
import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Picker class
 */
public class Picker extends Fragment {

    /**
     * Object listening for adapter events.
     */
    public interface ResultListener {
        void onCommitResult(List<String> result);
    }

    private Context mContext;
    private String mSeparator;
    private ViewGroup mRootView;
    private ViewGroup mPickerView;
    private List<ScrollAdapterView> mColumnViews;
    private ResultListener mResultListener;
    private ChangeTextColorOnFocus mColumnChangeListener;
    private ArrayList<PickerColumn> mColumns = new ArrayList<PickerColumn>();
    protected PickerConstant mConstant;

    private float mUnfocusedAlpha;
    private float mFocusedAlpha;
    private float mVisibleColumnAlpha;
    private float mInvisibleColumnAlpha;
    private int mAlphaAnimDuration;
    private Interpolator mDecelerateInterpolator;
    private Interpolator mAccelerateInterpolator;
    private boolean mKeyDown = false;
    private boolean mClicked = false;

    /**
     * selection result
     */
    private List<String> mResult;

    private OnItemClickListener mOnClickListener = new OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            if (mKeyDown) {
                mKeyDown = false;
                mClicked = true;
                updateAllColumnsForClick(true);
            }
        }
    };

    public static Picker newInstance() {
        return new Picker();
    }

    /**
     * Classes extending {@link Picker} should override this method to supply
     * the columns
     */
    protected ArrayList<PickerColumn> getColumns() {
        return null;
    }

    /**
     * Classes extending {@link Picker} can choose to override this method to
     * supply the separator string
     */
    protected String getSeparator() {
        return mSeparator;
    }

    /**
     * Classes extending {@link Picker} can choose to override this method to
     * supply the {@link Picker}'s root layout id
     */
    protected int getRootLayoutId() {
        return R.layout.picker;
    }

    /**
     * Classes extending {@link Picker} can choose to override this method to
     * supply the {@link Picker}'s id from within the layout provided by
     * {@link Picker#getRootLayoutId()}
     */
    protected int getPickerId() {
        return R.id.picker;
    }

    /**
     * Classes extending {@link Picker} can choose to override this method to
     * supply the {@link Picker}'s separator's layout id
     */
    protected int getPickerSeparatorLayoutId() {
        return R.layout.picker_separator;
    }

    /**
     * Classes extending {@link Picker} can choose to override this method to
     * supply the {@link Picker}'s item's layout id
     */
    protected int getPickerItemLayoutId() {
        return R.layout.picker_item;
    }

    /**
     * Classes extending {@link Picker} can choose to override this method to
     * supply the {@link Picker}'s item's {@link TextView}'s id from within the
     * layout provided by {@link Picker#getPickerItemLayoutId()} or 0 if the
     * layout provided by {@link Picker#getPickerItemLayoutId()} is a {link
     * TextView}.
     */
    protected int getPickerItemTextViewId() {
        return 0;
    }

    /**
     * Classes extending {@link Picker} can choose to override this method to
     * supply the {@link Picker}'s column's height in pixels.
     */
    protected int getPickerColumnHeightPixels() {
        return getActivity().getResources().getDimensionPixelSize(R.dimen.picker_column_height);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mContext = getActivity();
        mConstant = PickerConstant.getInstance(mContext.getResources());

        mFocusedAlpha = getFloat(R.dimen.list_item_selected_title_text_alpha);
        mUnfocusedAlpha = getFloat(R.dimen.list_item_unselected_text_alpha);
        mVisibleColumnAlpha = getFloat(R.dimen.picker_item_visible_column_item_alpha);
        mInvisibleColumnAlpha = getFloat(R.dimen.picker_item_invisible_column_item_alpha);

        mColumnChangeListener = new ChangeTextColorOnFocus();
        mAlphaAnimDuration = mContext.getResources().getInteger(
                R.integer.dialog_animation_duration);

        mDecelerateInterpolator = new DecelerateInterpolator(2.5F);
        mAccelerateInterpolator = new AccelerateInterpolator(2.5F);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {

        mColumns = getColumns();
        if (mColumns == null || mColumns.size() == 0) {
            return null;
        }

        mRootView = (ViewGroup) inflater.inflate(getRootLayoutId(), null);
        mPickerView = (ViewGroup) mRootView.findViewById(getPickerId());
        mColumnViews = new ArrayList<ScrollAdapterView>();
        mResult = new ArrayList<String>();

        int totalCol = mColumns.size();
        for (int i = 0; i < totalCol; i++) {
            final int colIndex = i;
            final String[] col = mColumns.get(i).getItems();
            mResult.add(col[0]);
            final ScrollAdapterView columnView = (ScrollAdapterView) inflater.inflate(
                    R.layout.picker_column, mPickerView, false);
            LayoutParams lp = columnView.getLayoutParams();
            lp.height = getPickerColumnHeightPixels();
            columnView.setLayoutParams(lp);
            mColumnViews.add(columnView);
            columnView.setTag(Integer.valueOf(colIndex));

            // add view to root
            mPickerView.addView(columnView);

            // add a separator if not the last element
            if (i != totalCol - 1 && getSeparator() != null) {
                TextView separator = (TextView) inflater.inflate(
                        getPickerSeparatorLayoutId(), mPickerView, false);
                separator.setText(getSeparator());
                mPickerView.addView(separator);
            }
        }
        initAdapters();
        mColumnViews.get(0).requestFocus();

        mClicked = false;
        mKeyDown = false;

        return mRootView;
    }

    private void initAdapters() {
        final int totalCol = mColumns.size();
        for (int i = 0; i < totalCol; i++) {
            final int colIndex = i;
            ScrollAdapterView columnView = mColumnViews.get(i);
            final String[] col = mColumns.get(i).getItems();
            setAdapter(columnView, col, colIndex);
            columnView.setOnFocusChangeListener(mColumnChangeListener);
            columnView.setOnItemSelectedListener(mColumnChangeListener);
            columnView.setOnItemClickListener(mOnClickListener);

            columnView.setOnKeyListener(new View.OnKeyListener() {
                @Override
                public boolean onKey(View v, int keyCode, KeyEvent event) {
                    switch (keyCode) {
                        case KeyEvent.KEYCODE_DPAD_CENTER:
                        case KeyEvent.KEYCODE_ENTER:
                            if (event.getAction() == KeyEvent.ACTION_DOWN) {
                                // We are only interested in the Key DOWN event here,
                                // because the Key UP event will generate a click, and
                                // will be handled by OnItemClickListener.
                                if (!mKeyDown) {
                                    mKeyDown = true;
                                    updateAllColumnsForClick(false);
                                }
                            }
                            break;
                    }
                    return false;
                }
            });
        }
    }

    private void unregisterListeners() {
        final int totalCol = mColumns.size();
        for (int i = 0; i < totalCol; i++) {
            ScrollAdapterView columnView = mColumnViews.get(i);
            columnView.setOnFocusChangeListener(null);
            columnView.setOnItemSelectedListener(null);
            columnView.setOnItemClickListener(null);
            columnView.setOnKeyListener(null);
        }
    }

    private void setAdapter(ScrollAdapterView columnView, final String[] col, final int colIndex) {
        List<String> arrayList = new ArrayList<String>(Arrays.asList(col));
        PickerScrollArrayAdapter pickerScrollArrayAdapter = (getPickerItemTextViewId() == 0) ?
                new PickerScrollArrayAdapter(mContext, getPickerItemLayoutId(), arrayList, colIndex)
                : new PickerScrollArrayAdapter(mContext, getPickerItemLayoutId(),
                        getPickerItemTextViewId(), arrayList, colIndex);
        columnView.setAdapter(pickerScrollArrayAdapter);
    }

    protected void updateAdapter(final int index, PickerColumn pickerColumn) {
        ScrollAdapterView columnView = mColumnViews.get(index);
        final String[] col = pickerColumn.getItems();

        ScrollArrayAdapter<String> adapter = (ScrollArrayAdapter<String>)(columnView.getAdapter());
        if (adapter != null) {
            adapter.setNotifyOnChange(false);
            adapter.clear();
            adapter.addAll(col);
            adapter.notifyDataSetChanged();
        }

        updateColumn(columnView, false, null);
        mColumns.set(index, pickerColumn);
    }

    protected void updateSelection(int columnIndex, int selectedIndex) {
        ScrollAdapterView columnView = mColumnViews.get(columnIndex);
        if (columnView != null) {
            columnView.setSelection(selectedIndex);
            String text = mColumns.get(columnIndex).getItems()[selectedIndex];
            mResult.set(columnIndex, text);
        }
    }

    public void setResultListener(ResultListener listener) {
        mResultListener = listener;
    }

    private void updateAllColumnsForClick(boolean keyUp) {
        ArrayList<Animator> animList = null;
        animList = new ArrayList<Animator>();
        View item;

        for (int j = 0; j < mColumnViews.size(); j++) {
            ScrollAdapterView column = mColumnViews.get(j);
            int selected = column.getSelectedItemPosition();
            for (int i = 0; i < column.getAdapter().getCount(); i++) {
                item = column.getItemView(i);
                if (item != null) {
                    if (selected == i) {
                        // set alpha for main item (selected) in the column
                        if (keyUp) {
                            setOrAnimateAlpha(item, true, mFocusedAlpha, mUnfocusedAlpha, animList,
                                    mAccelerateInterpolator);
                        } else {
                            setOrAnimateAlpha(item, true, mUnfocusedAlpha, -1, animList,
                                    mDecelerateInterpolator);
                        }
                    } else if (!keyUp) {
                        // hide all non selected items on key down
                        setOrAnimateAlpha(item, true, mInvisibleColumnAlpha, -1, animList,
                                mDecelerateInterpolator);
                    }
                }
            }
        }

        if (animList != null && animList.size() > 0) {
            AnimatorSet animSet = new AnimatorSet();
            animSet.playTogether(animList);

            if (mClicked) {
                animSet.addListener(new AnimatorListenerAdapter() {
                    @Override
                    public void onAnimationEnd(Animator animation) {
                        if (mResultListener != null) {
                            mResultListener.onCommitResult(mResult);
                        }
                    }
                });
            }
            animSet.start();
        }
    }

    private void updateColumn(ScrollAdapterView column, boolean animateAlpha,
            ArrayList<Animator> animList) {
        if (column == null) {
            return;
        }

        int selected = column.getSelectedItemPosition();
        View item;
        boolean focused = column.hasFocus();

        ArrayList<Animator> localAnimList = animList;
        if (animateAlpha && localAnimList == null) {
            // no global animation list, create a local one for the current set
            localAnimList = new ArrayList<Animator>();
        }

        for (int i = 0; i < column.getAdapter().getCount(); i++) {
            item = column.getItemView(i);
            if (item != null) {
                setOrAnimateAlpha(item, (selected == i), focused, animateAlpha, localAnimList);
            }
        }
        if (animateAlpha && animList == null && localAnimList != null && localAnimList.size() > 0) {
            // No global animation list, so play these start the current set of animations now
            AnimatorSet animSet = new AnimatorSet();
            animSet.playTogether(localAnimList);
            animSet.start();
        }
    }

    private void setOrAnimateAlpha(View view, boolean selected, boolean focused, boolean animate,
            ArrayList<Animator> animList) {
        if (selected) {
            // set alpha for main item (selected) in the column
            if ((focused && !mKeyDown) || mClicked) {
                setOrAnimateAlpha(view, animate, mFocusedAlpha, -1, animList,
                        mDecelerateInterpolator);
            } else {
                setOrAnimateAlpha(view, animate, mUnfocusedAlpha, -1, animList,
                        mDecelerateInterpolator);
            }
        } else {
            // set alpha for remaining items in the column
            if (focused && !mClicked && !mKeyDown) {
                setOrAnimateAlpha(view, animate, mVisibleColumnAlpha, -1, animList,
                        mDecelerateInterpolator);
            } else {
                setOrAnimateAlpha(view, animate, mInvisibleColumnAlpha, -1, animList,
                        mDecelerateInterpolator);
            }
        }
    }

    private void setOrAnimateAlpha(View view, boolean animate, float destAlpha, float startAlpha,
            ArrayList<Animator> animList, Interpolator interpolator) {
        view.clearAnimation();
        if (!animate) {
            view.setAlpha(destAlpha);
        } else {
            ObjectAnimator anim;
            if (startAlpha >= 0.0f) {
                // set a start alpha
                anim = ObjectAnimator.ofFloat(view, "alpha", startAlpha, destAlpha);
            } else {
                // no start alpha
                anim = ObjectAnimator.ofFloat(view, "alpha", destAlpha);
            }
            anim.setDuration(mAlphaAnimDuration);
            anim.setInterpolator(interpolator);
            if (animList != null) {
                animList.add(anim);
            } else {
                anim.start();
            }
        }
    }

    /**
     * Classes extending {@link Picker} can override this function to supply the
     * behavior when a list has been scrolled
     */
    protected void onScroll(View v) {
    }

    @Override
    public void onDestroyView() {
        unregisterListeners();
        if (mColumnChangeListener != null) {
            mColumnChangeListener.setDisabled();
        }
        super.onDestroyView();
    }

    private float getFloat(int resourceId) {
        TypedValue buffer = new TypedValue();
        mContext.getResources().getValue(resourceId, buffer, true);
        return buffer.getFloat();
    }

    private class PickerScrollArrayAdapter extends ScrollArrayAdapter<String> {

        private final int mColIndex;
        private final int mTextViewResourceId;

        PickerScrollArrayAdapter(Context context, int resource,
                List<String> objects, int colIndex) {
            super(context, resource, objects);
            mColIndex = colIndex;
            mTextViewResourceId = 0;
        }

        PickerScrollArrayAdapter(Context context, int resource, int textViewResourceId,
                List<String> objects, int colIndex) {
            super(context, resource, textViewResourceId, objects);
            mColIndex = colIndex;
            mTextViewResourceId = textViewResourceId;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            View view = super.getView(position, convertView, parent);
            view.setTag(Integer.valueOf(mColIndex));
            setOrAnimateAlpha(view,
                    (mColumnViews.get(mColIndex).getSelectedItemPosition() == position), false,
                    false, null);
            return view;
        }

        TextView getTextViewFromAdapterView(View adapterView) {
            if (mTextViewResourceId != 0) {
                return (TextView) adapterView.findViewById(mTextViewResourceId);
            } else {
                return (TextView) adapterView;
            }
        }
    }


    private class ChangeTextColorOnFocus implements View.OnFocusChangeListener,
            AdapterView.OnItemSelectedListener {
        private boolean mDisabled;

        ChangeTextColorOnFocus() {
            mDisabled = false;
        }

        public void setDisabled() {
            mDisabled = true;
        }

        @Override
        public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
            if (mDisabled) {
                // If the listener has been disabled (because the view is being destroyed)
                // then just ignore this call.
                return;
            }

            PickerScrollArrayAdapter pickerScrollArrayAdapter = (PickerScrollArrayAdapter) parent
                    .getAdapter();

            TextView textView = pickerScrollArrayAdapter.getTextViewFromAdapterView(view);

            int colIndex = (Integer) parent.getTag();

            updateColumn((ScrollAdapterView) parent, parent.hasFocus(), null);

            mResult.set(colIndex, textView.getText().toString());
            onScroll(textView);
        }

        @Override
        public void onNothingSelected(AdapterView<?> parent) {
            // N/A
        }

        @Override
        public void onFocusChange(View view, boolean hasFocus) {
            if (mDisabled) {
                // If the listener has been disabled (because the view is being destroyed)
                // then just ignore this call.
                return;
            }

            if (view instanceof ScrollAdapterView) {
                updateColumn((ScrollAdapterView) view, true, null);
            }
        }
    }
}
