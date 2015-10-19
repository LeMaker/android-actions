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

package com.android.tv.settings.widget;

import android.view.View;

import com.android.tv.settings.widget.ScrollAdapterView;
import com.android.tv.settings.widget.ScrollAdapterView.OnScrollListener;

/**
 * ScrollAdapterView OnScrollListener that supports edit mode.
 */
public class ScrollAdapterEditScrollListener implements OnScrollListener {

    /**
     * Callback listener which listens to when scrolling started.
     */
    public interface OnScrollStartListener {
        public void onScrollStart(int direction);
    }

    /**
     * Flag indicating whether currently is in edit mode or not.
     */
    private boolean mEnableEditMode;

    /**
     * Index of current selected view in edit mode.
     */
    private int mCurrentIndex;

    /**
     * Index of first selected view when entering edit mode.
     */
    private int mEditStartIndex;

    private float mLastPosition = 0;

    /**
     * The scrolling distance in edit mode.
     */
    private float mEditScrollDis;

    private ScrollAdapterView mScrollAdapterView;

    private int mOrientation;

    private int mDirection;

    private boolean mStartScrolling;

    private OnScrollStartListener mOnScrollStartListener;

    public ScrollAdapterEditScrollListener(ScrollAdapterView scrollAdapterView) {
        mScrollAdapterView = scrollAdapterView;
    }

    public void setOnScrollStartListener(OnScrollStartListener onScrollStartListener) {
        mOnScrollStartListener = onScrollStartListener;
    }

    public void startEditMode(int index) {
        mEnableEditMode = true;
        mOrientation = mScrollAdapterView.getOrientation();

        mCurrentIndex = index;
        mEditStartIndex = index;

        View selectedView = mScrollAdapterView.getItemView(index);
        if (selectedView != null) {
            float space = mScrollAdapterView.getSpace();
            if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                mEditScrollDis = selectedView.getMeasuredWidth() + space;
            } else if (mOrientation == ScrollAdapterView.VERTICAL) {
                mEditScrollDis = selectedView.getMeasuredHeight() + space;
            }
        }
    }

    public void exitEditMode() {
        mEnableEditMode = false;
    }

    public int getCurrentIndex() {
        return mCurrentIndex;
    }

    public int getEditStartIndex() {
        return mEditStartIndex;
    }

    public boolean isEditMode() {
        return mEnableEditMode;
    }

    @Override
    public void onScrolled(View view, int position, float mainPosition, float secondPosition) {
        mCurrentIndex = position;

        float currentPosition = position + mainPosition;

        if (mEnableEditMode) {
            View startView = mScrollAdapterView.getItemView(mEditStartIndex);
            if (startView != null && startView.getAlpha() != 0f) {
                startView.setAlpha(0f);
            }

            int size = mScrollAdapterView.getAdapter().getCount();

            if (mCurrentIndex >= mEditStartIndex) {
                // If current index is greater than starting index, translate current view
                // to the left/down while scrolling position changing.
                View curView = mScrollAdapterView.getItemView(mCurrentIndex + 1);
                float scrollDis = mainPosition * mEditScrollDis;
                if (curView != null) {
                    if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                        curView.setTranslationX(-scrollDis);
                    } else {
                        curView.setTranslationY(-scrollDis);
                    }
                }

                for (int index = mEditStartIndex; index < mCurrentIndex + 1; ++index) {
                    curView = mScrollAdapterView.getItemView(index);
                    if (curView != null) {
                        // Set the translation of all views between start index and
                        // current index to -mEditScrollDis.
                        if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                            curView.setTranslationX(-mEditScrollDis);
                        } else {
                            curView.setTranslationY(-mEditScrollDis);
                        }
                    }
                }

                // Reset all other views translation to 0.
                if (mCurrentIndex + 2 < size) {
                    for (int index = mCurrentIndex + 2; index < size; ++index) {
                        curView = mScrollAdapterView.getItemView(index);
                        if (curView != null) {
                            if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                                curView.setTranslationX(0);
                            } else {
                                curView.setTranslationY(0);
                            }
                        }
                    }
                }

                for (int index = 0; index < mEditStartIndex; ++index) {
                    curView = mScrollAdapterView.getItemView(index);
                    if (curView != null) {
                        if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                            curView.setTranslationX(0);
                        } else {
                            curView.setTranslationY(0);
                        }
                    }
                }
            } else if (mCurrentIndex < mEditStartIndex) {
                // If current index is less than starting index, translate current view
                // to the right/down while scrolling position changing.
                View curView = mScrollAdapterView.getItemView(mCurrentIndex);
                float scrollDis = (1 - mainPosition) * mEditScrollDis;
                if (curView != null) {
                    if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                        curView.setTranslationX(scrollDis);
                    } else {
                        curView.setTranslationY(scrollDis);
                    }
                }

                if (mCurrentIndex + 1 <= size) {
                    for (int index = mCurrentIndex + 1; index < mEditStartIndex; ++index) {
                        curView = mScrollAdapterView.getItemView(index);
                        if (curView != null) {
                            // Set the translation of all views between start index and
                            // current index to mEditScrollDis.
                            if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                                curView.setTranslationX(mEditScrollDis);
                            } else {
                                curView.setTranslationY(mEditScrollDis);
                            }
                        }
                    }
                }

                // Reset all other views translation to 0.
                for (int index = 0; index < mCurrentIndex; ++index) {
                    curView = mScrollAdapterView.getItemView(index);
                    if (curView != null) {
                        if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                            curView.setTranslationX(0);
                        } else {
                            curView.setTranslationY(0);
                        }
                    }
                }

                for (int index = mEditStartIndex; index < size; ++index) {
                    curView = mScrollAdapterView.getItemView(index);
                    if (curView != null) {
                        if (mOrientation == ScrollAdapterView.HORIZONTAL) {
                            curView.setTranslationX(0);
                        } else {
                            curView.setTranslationY(0);
                        }
                    }
                }
            }

            if (mScrollAdapterView.isInScrollingOrDragging() && !mStartScrolling) {
                // Start scrolling
                mStartScrolling = true;

                if (currentPosition > mLastPosition) {
                    mDirection = mOrientation == ScrollAdapterView.HORIZONTAL ? View.FOCUS_RIGHT
                            : View.FOCUS_DOWN;

                    if (mOnScrollStartListener != null) {
                        mOnScrollStartListener.onScrollStart(mDirection);
                    }
                } else {
                    mDirection = mOrientation == ScrollAdapterView.HORIZONTAL ? View.FOCUS_LEFT
                            : View.FOCUS_UP;

                    if (mOnScrollStartListener != null) {
                        mOnScrollStartListener.onScrollStart(mDirection);
                    }
                }
            } else if (mStartScrolling && !mScrollAdapterView.isInScrollingOrDragging()) {
                // End of scrolling.
                mStartScrolling = false;
            } else if (mStartScrolling && currentPosition > mLastPosition) {
                if ((mOrientation == ScrollAdapterView.HORIZONTAL)
                        && (mDirection == View.FOCUS_LEFT)) {
                    // Revert scrolling direction to right before finishing previous left scroll.
                    mDirection = View.FOCUS_RIGHT;

                    if (mOnScrollStartListener != null) {
                        mOnScrollStartListener.onScrollStart(mDirection);
                    }
                } else if ((mOrientation == ScrollAdapterView.VERTICAL)
                        && (mDirection == View.FOCUS_UP)) {
                    // Revert scrolling direction to down before finishing previous up scroll.
                    mDirection = View.FOCUS_DOWN;

                    if (mOnScrollStartListener != null) {
                        mOnScrollStartListener.onScrollStart(mDirection);
                    }
                }
            } else if (mStartScrolling && currentPosition < mLastPosition) {
                if ((mOrientation == ScrollAdapterView.HORIZONTAL)
                        && (mDirection == View.FOCUS_RIGHT)) {
                    // Revert scrolling direction to left before finishing previous right scroll.
                    mDirection = View.FOCUS_LEFT;

                    if (mOnScrollStartListener != null) {
                        mOnScrollStartListener.onScrollStart(mDirection);
                    }
                } else if ((mOrientation == ScrollAdapterView.VERTICAL)
                        && (mDirection == View.FOCUS_DOWN)) {
                    // Revert scrolling direction to up before finishing previous down scroll.
                    mDirection = View.FOCUS_UP;

                    if (mOnScrollStartListener != null) {
                        mOnScrollStartListener.onScrollStart(mDirection);
                    }
                }
            }
        }

        mLastPosition = currentPosition;
    }
}
