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

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.drawable.Drawable;
import android.os.Parcel;
import android.os.Parcelable;

import java.util.ArrayList;

/**
 * Draws a horizontal bar chart with colored slices, each represented by
 * {@link Entry}. Pulled from Android Settings App.
 */
public class PercentageBarChart extends Drawable {

    public static class Entry implements Comparable<Entry>, Parcelable {

        final int order;
        final float percentage;
        final Paint paint;

        public Entry(int order, float percentage, int color) {
            this.order = order;
            this.percentage = percentage;
            this.paint = new Paint();
            this.paint.setColor(color);
            this.paint.setStyle(Paint.Style.FILL);
        }

        @Override
        public int compareTo(Entry another) {
            return order - another.order;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeInt(order);
            dest.writeFloat(percentage);
            dest.writeInt(paint.getColor());
        }

        public static final Parcelable.Creator<Entry> CREATOR = new Parcelable.Creator<Entry>() {
            @Override
            public Entry createFromParcel(Parcel in) {
                return new Entry(in.readInt(), in.readFloat(), in.readInt());
            }

            @Override
            public Entry[] newArray(int size) {
                return new Entry[size];
            }
        };
    }

    private final ArrayList<Entry> mEntries;
    private final int mBackgroundColor;
    private final int mMinTickWidth;
    private final int mWidth;
    private final int mHeight;
    private final boolean mIsLayoutRtl;
    private final Paint mEmptyPaint;

    public PercentageBarChart(ArrayList<Entry> entries, int backgroundColor, int minTickWidth,
            int width, int height, boolean isLayoutRtl) {
        super();
        mEntries = entries;
        mBackgroundColor = backgroundColor;
        mMinTickWidth = minTickWidth;
        mWidth = width;
        mHeight = height;
        mIsLayoutRtl = isLayoutRtl;
        mEmptyPaint = new Paint();
        mEmptyPaint.setColor(backgroundColor);
        mEmptyPaint.setStyle(Paint.Style.FILL);
    }

    @Override
    public int getIntrinsicHeight() {
        return mHeight;
    }

    @Override
    public int getIntrinsicWidth() {
        return mWidth;
    }

    @Override
    public void draw(Canvas canvas) {
        mEmptyPaint.setColor(mBackgroundColor);

        float end = (mIsLayoutRtl) ? 0 : mWidth;
        float lastX = (mIsLayoutRtl) ? mWidth : 0;

        for (final Entry e : mEntries) {
            if (e.percentage == 0.0f) {
                continue;
            }

            final float entryWidth = Math.max(mMinTickWidth, mWidth * e.percentage);

            // progress toward the end.
            final float nextX = lastX + ((lastX < end) ? entryWidth : -entryWidth);

            // if we've hit the limit, stop drawing.
            if (drawEntry(canvas, lastX, nextX, e.paint)) {
                return;
            }
            lastX = nextX;
        }

        drawEntry(canvas, lastX, end, mEmptyPaint);
    }

    @Override
    public void setAlpha(int alpha) {
    }

    @Override
    public void setColorFilter(ColorFilter cf) {
    }

    @Override
    public int getOpacity() {
        return PixelFormat.OPAQUE;
    }

    /**
     * Draws a rectangle from the lesser of the two x inputs to the greater of
     * the two. If either of the two x inputs lie outside the bounds of this
     * drawable, limit the rectangle drawn to the bounds.
     *
     * @param canvas the canvas to draw to.
     * @param x1 the first x input. This may be greater or smaller than x2.
     * @param x2 the second x input. This may be greater or smaller than x1.
     * @param paint the color to draw.
     * @return true if either of the x inputs was beyond the bounds of this
     *         drawable, false otherwise.
     */
    private boolean drawEntry(Canvas canvas, float x1, float x2, Paint paint) {
        boolean hitLimit = false;
        float left = x1 > x2 ? x2 : x1;
        float right = x1 > x2 ? x1 : x2;
        if (left < 0) {
            left = 0;
            hitLimit = true;
        }
        if (right > mWidth) {
            right = mWidth;
            hitLimit = true;
        }
        canvas.drawRect(left, 0, right, mHeight, paint);
        return hitLimit;
    }
}
