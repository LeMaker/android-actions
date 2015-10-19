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

package com.android.tv.settings.dialog;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.Log;

import java.util.ArrayList;

/**
 * A data class which represents a settings layout within an
 * {@link SettingsLayoutFragment}. Represents a list of choices the
 * user can make, a radio-button list of configuration options, or just a
 * list of information.
 */
public class Layout implements Parcelable {

    public interface LayoutNodeRefreshListener {
        void onRefreshView();
        Node getSelectedNode();
    }

    public interface ContentNodeRefreshListener {
        void onRefreshView();
    }

    public interface Node {
        String getTitle();
    }

    private abstract static class LayoutTreeNode implements Node {
        LayoutTreeBranch mParent;

        void Log(int level) {
        }
    }

    private abstract static class LayoutTreeBranch extends LayoutTreeNode {
        ArrayList<LayoutTreeNode> mChildren;
        LayoutTreeBranch() {
            mChildren = new ArrayList<LayoutTreeNode>();
        }
    }

    public static class LayoutRow {
        public static final int NO_CHECK_SET = 0;
        public static final int VIEW_TYPE_ACTION = 0;
        public static final int VIEW_TYPE_STATIC = 1;

        private String mTitle;
        private StringGetter mDescription;
        private LayoutTreeNode mNode;
        private boolean mEnabled;
        private int mViewType;
        private boolean mChecked = false;
        private Drawable mIcon = null;

        public Node getNode() {
            return mNode;
        }

        public Uri getIconUri() {
            return null;
        }

        public Drawable getIcon() {
            return mIcon;
        }

        public int getCheckSetId() {
            return 0;
        }

        public boolean isChecked() {
            return mChecked;
        }

        public void setChecked(boolean v) {
            mChecked = v;
        }

        public boolean infoOnly() {
            return false;
        }

        public boolean isEnabled() {
            return mEnabled;
        }

        public boolean hasNext() {
            return false;
        }

        public boolean hasMultilineDescription() {
            return false;
        }

        public String getTitle() {
            return mTitle;
        }

        public StringGetter getDescription() {
            return mDescription;
        }

        public int getViewType() {
            return mViewType;
        }

        public boolean isGoBack() {
            if (mNode instanceof Action) {
                Action a = (Action) mNode;
                if (a.mActionId == Action.ACTION_BACK) {
                    return true;
                }
            }
            return false;
        }

        public Action getUserAction() {
            if (mNode instanceof Action) {
                Action a = (Action) mNode;
                if (a.mActionId != Action.ACTION_NONE) {
                    return a;
                }
            }
            return null;
        }

        public int getContentIconRes() {
            if (mNode instanceof Header) {
                return ((Header) mNode).mContentIconRes;
            }
            return 0;
        }

        public LayoutRow(LayoutTreeNode node) {
            mNode = node;
            mViewType = VIEW_TYPE_ACTION;
            Appearence a;
            if (node instanceof Header) {
                a = ((Header) node).mAppearence;
                mEnabled = true;
            } else if (node instanceof Action) {
                a = ((Action) node).mAppearence;
                mEnabled = true;
            } else if (node instanceof Status) {
                a = ((Status) node).mAppearence;
                mEnabled = true;
            } else {
                a = null;
                mEnabled = false;
                if (node instanceof Static) {
                    mViewType = VIEW_TYPE_STATIC;
                    Static s = (Static) node;
                    mTitle = s.mTitle;
                }
            }
            if (a != null) {
                mTitle = a.getTitle();
                mDescription = a.mDescriptionGetter;
                mIcon = a.getIcon();
                mChecked = a.isChecked();
            }
        }
    }

    public abstract static class DrawableGetter {
        public abstract Drawable get();

        /**
         * Notification from client that antecedent data has changed and the drawable should be
         * redisplayed.
         */
        public void refreshView() {
            //TODO - When implementing, ensure that multiple updates from the same event do not
            // cause multiple view updates.
        }
    }

    public abstract static class StringGetter {
        private ContentNodeRefreshListener mListener;

        public void setListener(ContentNodeRefreshListener listener) {
            mListener = listener;
        }

        public abstract String get();

        /**
         * Notification from client that antecedent data has changed and the string should be
         * redisplayed.
         */
        public void refreshView() {
            if (mListener != null) {
                mListener.onRefreshView();
            }
        }
    }

    /**
     * Implementation of "StringGetter" that stores and returns a literal string.
     */
    private static class LiteralStringGetter extends StringGetter {
        private final String mValue;
        public String get() {
            return mValue;
        }
        LiteralStringGetter(String value) {
            mValue = value;
        }
    }

    /**
     * Implementation of "StringGetter" that stores a string resource id and returns a string.
     */
    private static class ResourceStringGetter extends StringGetter {
        private final int mStringResourceId;
        private final Resources mRes;
        public String get() {
            return mRes.getString(mStringResourceId);
        }
        ResourceStringGetter(Resources res, int stringResourceId) {
            mRes = res;
            mStringResourceId = stringResourceId;
        }
    }

    public abstract static class LayoutGetter extends LayoutTreeNode {
        // Layout manages this listener; removing it when this node is not visible and setting it
        // when it is.  Users are expected to set the listener with Layout.setRefreshViewListener.
        private LayoutNodeRefreshListener mListener;

        public void setListener(LayoutNodeRefreshListener listener) {
            mListener = listener;
        }

        public void notVisible() {
            mListener = null;
        }

        public abstract Layout get();

        public Node getSelectedNode() {
            if (mListener != null) {
                return mListener.getSelectedNode();
            } else {
                return null;
            }
        }

        /**
         * Notification from client that antecedent data has changed and the list containing the
         * contents of this getter should be updated.
         */
        public void refreshView() {
            if (mListener != null) {
                mListener.onRefreshView();
            }
        }

        @Override
        public String getTitle() {
            return null;
        }

        void Log(int level) {
            Log.d("Layout", indent(level) + "LayoutGetter");
            Layout l = get();
            l.Log(level + 1);
        }
    }

    private static class Appearence {
        private Drawable mIcon;
        private DrawableGetter mIconGetter;
        private String mTitle;
        private StringGetter mDescriptionGetter;
        private boolean mChecked = false;

        public String toString() {
            StringBuilder stringBuilder = new StringBuilder()
                .append("'")
                .append(mTitle)
                .append("'");
            if (mDescriptionGetter != null) {
                stringBuilder
                    .append(" : '")
                    .append(mDescriptionGetter.get())
                    .append("'");
            }
            stringBuilder
                .append(" : '")
                .append(mChecked)
                .append("'");
            return stringBuilder.toString();
        }

        public String getTitle() {
            return mTitle;
        }

        public Drawable getIcon() {
            if (mIconGetter != null) {
                return mIconGetter.get();
            } else {
                return mIcon;
            }
        }

        public boolean isChecked() {
            return mChecked;
        }
    }

    /**
     * Header is a container for a sub-menu of "LayoutTreeNode" items.
     */
    public static class Header extends LayoutTreeBranch {
        private Appearence mAppearence = new Appearence();
        private int mSelectedIndex = 0;
        private String mDetailedDescription;
        private int mContentIconRes = 0;

        public static class Builder {
            private Resources mRes;
            private Header mHeader = new Header();

            public Builder(Resources res) {
                mRes = res;
            }

            public Builder icon(int resId) {
                mHeader.mAppearence.mIcon = mRes.getDrawable(resId);
                return this;
            }

            public Builder icon(DrawableGetter drawableGetter) {
                mHeader.mAppearence.mIconGetter = drawableGetter;
                return this;
            }

            public Builder contentIconRes(int resId) {
                mHeader.mContentIconRes = resId;
                return this;
            }

            public Builder title(int resId) {
                mHeader.mAppearence.mTitle = mRes.getString(resId);
                return this;
            }

            public Builder description(int resId) {
                mHeader.mAppearence.mDescriptionGetter = new ResourceStringGetter(mRes, resId);
                return this;
            }

            public Builder title(String title) {
                mHeader.mAppearence.mTitle = title;
                return this;
            }

            public Builder description(String description) {
                mHeader.mAppearence.mDescriptionGetter = new LiteralStringGetter(description);
                return this;
            }

            public Builder description(StringGetter description) {
                mHeader.mAppearence.mDescriptionGetter = description;
                return this;
            }

            public Builder detailedDescription(int resId) {
                mHeader.mDetailedDescription = mRes.getString(resId);
                return this;
            }

            public Builder detailedDescription(String detailedDescription) {
                mHeader.mDetailedDescription = detailedDescription;
                return this;
            }

            public Header build() {
                return mHeader;
            }
        }

        @Override
        public String getTitle() {
            return mAppearence.getTitle();
        }

        public Header add(LayoutTreeNode node) {
            node.mParent = this;
            mChildren.add(node);
            return this;
        }

        String getDetailedDescription() {
            return mDetailedDescription;
        }

        void Log(int level) {
            Log.d("Layout", indent(level) + "Header  " + mAppearence);
            for (LayoutTreeNode i : mChildren)
                i.Log(level + 1);
        }
    }

    public static class Action extends LayoutTreeNode {
        public static final int ACTION_NONE = -1;
        public static final int ACTION_INTENT = -2;
        public static final int ACTION_BACK = -3;
        private int mActionId;
        private Intent mIntent;
        private Appearence mAppearence = new Appearence();
        private Bundle mActionData;
        private boolean mDefaultSelection = false;

        private Action(int id) {
            mActionId = id;
        }

        private Action(Intent intent) {
            mActionId = ACTION_INTENT;
            mIntent = intent;
        }

        public static class Builder {
            private Resources mRes;
            private Action mAction;

            public Builder(Resources res, int id) {
                mRes = res;
                mAction = new Action(id);
            }

            public Builder(Resources res, Intent intent) {
                mRes = res;
                mAction = new Action(intent);
            }

            public Builder title(int resId) {
                mAction.mAppearence.mTitle = mRes.getString(resId);
                return this;
            }

            public Builder description(int resId) {
                mAction.mAppearence.mDescriptionGetter = new LiteralStringGetter(mRes.getString(
                        resId));
                return this;
            }

            public Builder title(String title) {
                mAction.mAppearence.mTitle = title;
                return this;
            }

             public Builder icon(int resId) {
                 mAction.mAppearence.mIcon = mRes.getDrawable(resId);
                 return this;
             }

            public Builder description(String description) {
                mAction.mAppearence.mDescriptionGetter = new LiteralStringGetter(description);
                return this;
            }

            public Builder description(StringGetter description) {
                mAction.mAppearence.mDescriptionGetter = description;
                return this;
            }

            public Builder checked(boolean checked) {
                mAction.mAppearence.mChecked = checked;
                return this;
            }

            public Builder data(Bundle data) {
                mAction.mActionData = data;
                return this;
            }

            /*
             * Makes this action default initial selection when the list is displayed.
             */
            public Builder defaultSelection() {
                mAction.mDefaultSelection = true;
                return this;
            }

            public Action build() {
                return mAction;
            }
        }

        void Log(int level) {
            Log.d("Layout", indent(level) + "Action  #" + mActionId + "  " + mAppearence);
        }

        public int getId() {
            return mActionId;
        }

        public Intent getIntent() {
            return mIntent;
        }

        @Override
        public String getTitle() {
            return mAppearence.getTitle();
        }

        public Bundle getData() {
            return mActionData;
        }
    }

    public static class Status extends LayoutTreeNode {
        private Appearence mAppearence = new Appearence();

        public static class Builder {
            private Resources mRes;
            private Status mStatus = new Status();

            public Builder(Resources res) {
                mRes = res;
            }

            public Builder icon(int resId) {
                mStatus.mAppearence.mIcon = mRes.getDrawable(resId);
                return this;
            }

            public Builder title(int resId) {
                mStatus.mAppearence.mTitle = mRes.getString(resId);
                return this;
            }

            public Builder description(int resId) {
                mStatus.mAppearence.mDescriptionGetter = new LiteralStringGetter(mRes.getString(
                        resId));
                return this;
            }

            public Builder title(String title) {
                mStatus.mAppearence.mTitle = title;
                return this;
            }

            public Builder description(String description) {
                mStatus.mAppearence.mDescriptionGetter = new LiteralStringGetter(description);
                return this;
            }

            public Builder description(StringGetter description) {
                mStatus.mAppearence.mDescriptionGetter = description;
                return this;
            }

            public Status build() {
                return mStatus;
            }
        }

        @Override
        public String getTitle() {
            return mAppearence.getTitle();
        }

        void Log(int level) {
            Log.d("Layout", indent(level) + "Status  " + mAppearence);
        }
    }

    public static class Static extends LayoutTreeNode {
        private String mTitle;

        public static class Builder {
            private Resources mRes;
            private Static mStatic = new Static();

            public Builder(Resources res) {
                mRes = res;
            }

            public Builder title(int resId) {
                mStatic.mTitle = mRes.getString(resId);
                return this;
            }

            public Builder title(String title) {
                mStatic.mTitle = title;
                return this;
            }

            public Static build() {
                return mStatic;
            }
        }

        @Override
        public String getTitle() {
            return mTitle;
        }

        void Log(int level) {
            Log.d("Layout", indent(level) + "Static  '" + mTitle + "'");
        }
    }

    /**
     * Pointer to currently visible item.
     */
    private Header mNavigationCursor;

    /**
     * Index of selected item when items are displayed. This is used by LayoutGetter to implemented
     * selection stability, where a LayoutGetter can arrange for a list that is refreshed regularly
     * to carry forward a selection.
     */
    private int mInitialItemIndex = -1;
    private final ArrayList<LayoutRow> mLayoutRows = new ArrayList<LayoutRow>();
    private final ArrayList<LayoutGetter> mVisibleLayoutGetters = new ArrayList<LayoutGetter>();
    private final ArrayList<LayoutTreeNode> mChildren = new ArrayList<LayoutTreeNode>();
    private String mTopLevelBreadcrumb = "";
    private LayoutNodeRefreshListener mListener;

    public ArrayList<LayoutRow> getLayoutRows() {
        return mLayoutRows;
    }

    public void setRefreshViewListener(LayoutNodeRefreshListener listener) {
        mListener = listener;
    }

    /**
     * Return the breadcrumb the user should see in the content pane.
     */
    public String getBreadcrumb() {
      if (mNavigationCursor.mParent == null) {
          // At the top level of the layout.
          return mTopLevelBreadcrumb;
      } else {
          // Showing a header down the hierarchy, breadcrumb is title of item above.
          return ((Header) (mNavigationCursor.mParent)).mAppearence.mTitle;
      }
    }

    /**
     * Navigate up one level, return true if a parent node is now visible. Return false if the
     * already at the top level node. The controlling fragment interprets a false return value as
     * "stop activity".
     */
    public boolean goBack() {
        if (mNavigationCursor.mParent != null) {
            Header u = (Header) mNavigationCursor.mParent;
            if (u != null) {
                mNavigationCursor = u;
                updateLayoutRows();
                return true;
            }
        }
        return false;
    }

    /**
     * Parcelable implementation.
     */
    public Layout(Parcel in) {
    }

    public Layout() {
        mNavigationCursor = null;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
    }

    public static final Parcelable.Creator CREATOR = new Parcelable.Creator() {
        public Layout createFromParcel(Parcel in) {
            return new Layout(in);
        }

        public Layout[] newArray(int size) {
            return new Layout[size];
        }
    };

    String getTitle() {
        return mNavigationCursor.mAppearence.mTitle;
    }

    Drawable getIcon() {
        return mNavigationCursor.mAppearence.getIcon();
    }

    String getDescription() {
        return mNavigationCursor.getDetailedDescription();
    }

    public void goToTitle(String title) {
        while (mNavigationCursor.mParent != null) {
            mNavigationCursor = (Header) (mNavigationCursor.mParent);
            if (TextUtils.equals(mNavigationCursor.mAppearence.mTitle, title)) {
                break;
            }
        }
        updateLayoutRows();
    }

    /*
     * Respond to a user click on "layoutRow" and return "true" if the state of the display has
     * changed. A controlling fragment will respond to a "true" return by updating the view.
     */
    public boolean onClickNavigate(LayoutRow layoutRow) {
        LayoutTreeNode node = layoutRow.mNode;
        if (node instanceof Header) {
            mNavigationCursor.mSelectedIndex = mLayoutRows.indexOf(layoutRow);
            mNavigationCursor = (Header) node;
            updateLayoutRows();
            return true;
        }
        return false;
    }

    public void reloadLayoutRows() {
        updateLayoutRows();
    }

    public Layout add(Header header) {
        header.mParent = null;
        mChildren.add(header);
        return this;
    }

    public Layout add(LayoutTreeNode leaf) {
        leaf.mParent = null;
        mChildren.add(leaf);
        return this;
    }

    public Layout breadcrumb(String topLevelBreadcrumb) {
        mTopLevelBreadcrumb = topLevelBreadcrumb;
        return this;
    }

    /**
     * Sets the selected node to the first top level node with its title member equal to "title". If
     * "title" is null, empty, or there are no top level nodes with a title member equal to "title",
     * set the first node in the list as the selected.
     */
    public Layout setSelectedByTitle(String title) {
        for (int i = 0; i < mChildren.size(); ++i) {
            if (TextUtils.equals(mChildren.get(i).getTitle(), title)) {
                mInitialItemIndex = i;
                break;
            }
        }
        return this;
    }

    public void Log(int level) {
        for (LayoutTreeNode i : mChildren) {
            i.Log(level + 1);
        }
    }

    public void Log() {
        Log.d("Layout", "----- Layout");
        Log(0);
    }

    public void navigateToRoot() {
        if (mChildren.size() > 0) {
            mNavigationCursor = (Header) mChildren.get(0);
        } else {
            mNavigationCursor = null;
        }
        updateLayoutRows();
    }

    public int getSelectedIndex() {
        return mNavigationCursor.mSelectedIndex;
    }

    public void setSelectedIndex(int index) {
        mNavigationCursor.mSelectedIndex = index;
    }

    public void setParentSelectedIndex(int index) {
        if (mNavigationCursor.mParent != null) {
            Header u = (Header) mNavigationCursor.mParent;
            u.mSelectedIndex = index;
        }
    }

    private void addNodeListToLayoutRows(ArrayList<LayoutTreeNode> list) {
        for (LayoutTreeNode node : list) {
            if (node instanceof LayoutGetter) {
                // Add subitems of "node" recursively.
                LayoutGetter layoutGetter = (LayoutGetter) node;
                layoutGetter.setListener(mListener);
                mVisibleLayoutGetters.add(layoutGetter);
                Layout layout = layoutGetter.get();
                for (LayoutTreeNode child : layout.mChildren) {
                    child.mParent = mNavigationCursor;
                }
                int initialIndex = layout.mInitialItemIndex;
                if (initialIndex != -1) {
                    mNavigationCursor.mSelectedIndex = mLayoutRows.size() + initialIndex;
                }
                addNodeListToLayoutRows(layout.mChildren);
            } else {
                if (node instanceof Action && ((Action) node).mDefaultSelection) {
                    mNavigationCursor.mSelectedIndex = mLayoutRows.size();
                }
                mLayoutRows.add(new LayoutRow(node));
            }
        }
    }

    private void updateLayoutRows() {
        mLayoutRows.clear();
        for (LayoutGetter layoutGetter : mVisibleLayoutGetters) {
            layoutGetter.notVisible();
        }
        mVisibleLayoutGetters.clear();
        addNodeListToLayoutRows(mNavigationCursor.mChildren);
    }

    private static String indent(int level) {
        String s = new String();
        for (int i = 0; i < level; ++i) {
            s += "  ";
        }
        return s;
    }
}
