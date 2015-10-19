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

import com.android.tv.settings.ActionBehavior;
import com.android.tv.settings.ActionKey;
import com.android.tv.settings.R;
import com.android.tv.settings.dialog.old.Action;

import android.content.res.Resources;

import java.util.ArrayList;

/**
 * The different possible action types (screens).
 */
enum ActionType {
    OPEN(new ActionBehavior[] { ActionBehavior.INIT },
            R.string.device_apps_app_management_open),
    FORCE_STOP(new ActionBehavior[] { ActionBehavior.INIT, ActionBehavior.OK,
            ActionBehavior.CANCEL },
            R.string.device_apps_app_management_force_stop,
            R.string.device_apps_app_management_force_stop_desc),
    UNINSTALL(new ActionBehavior[] { ActionBehavior.INIT, ActionBehavior.OK,
            ActionBehavior.CANCEL },
            R.string.device_apps_app_management_uninstall,
            R.string.device_apps_app_management_uninstall_desc),
    DISABLE(new ActionBehavior[] { ActionBehavior.INIT, ActionBehavior.OK,
            ActionBehavior.CANCEL },
            R.string.device_apps_app_management_disable,
            R.string.device_apps_app_management_disable_desc),
    ENABLE(new ActionBehavior[] { ActionBehavior.INIT, ActionBehavior.OK,
            ActionBehavior.CANCEL },
            R.string.device_apps_app_management_enable,
            R.string.device_apps_app_management_enable_desc),
    CLEAR_DATA(new ActionBehavior[] { ActionBehavior.INIT, ActionBehavior.OK,
            ActionBehavior.CANCEL },
            R.string.device_apps_app_management_clear_data,
            R.string.device_apps_app_management_clear_data_desc,
            R.string.device_apps_app_management_clear_data_what),
    CLEAR_DEFAULTS(new ActionBehavior[] { ActionBehavior.INIT, ActionBehavior.OK,
            ActionBehavior.CANCEL },
            R.string.device_apps_app_management_clear_default),
    CLEAR_CACHE(new ActionBehavior[] { ActionBehavior.INIT, ActionBehavior.OK,
            ActionBehavior.CANCEL },
            R.string.device_apps_app_management_clear_cache),
    NOTIFICATIONS(new ActionBehavior[] { ActionBehavior.INIT, ActionBehavior.ON,
            ActionBehavior.OFF },
            R.string.device_apps_app_management_notifications),
    PERMISSIONS(new ActionBehavior[] { ActionBehavior.INIT },
            R.string.device_apps_app_management_permissions);

    private final ActionBehavior[] mBehaviors;
    private final int mNameResource;
    private final int mDescResource;
    private final int mDesc2Resource;

    private ActionType(ActionBehavior[] behaviors, int nameResource) {
        this(behaviors, nameResource, 0, 0);
    }

    private ActionType(ActionBehavior[] behaviors, int nameResource, int descResource) {
        this(behaviors, nameResource, descResource, 0);
    }

    private ActionType(
            ActionBehavior[] behaviors, int nameResource, int descResource, int desc2Resource) {
        mBehaviors = behaviors;
        mNameResource = nameResource;
        mDescResource = descResource;
        mDesc2Resource = desc2Resource;
    }

    String getDesc(Resources resources) {
        return (mDescResource == 0) ? null : resources.getString(mDescResource);
    }

    String getDesc2(Resources resources) {
        return (mDesc2Resource == 0) ? null : resources.getString(mDesc2Resource);
    }

    Action toInitAction(Resources resources) {
        return toInitAction(resources, null);
    }

    Action toInitAction(Resources resources, String description) {
        return toAction(resources, ActionBehavior.INIT, description);
    }

    Action toAction(Resources resources, ActionBehavior actionBehavior, String description) {
        return new Action.Builder()
                .key(new ActionKey<ActionType, ActionBehavior>(this, actionBehavior).getKey())
                .title(resources.getString(mNameResource))
                .description(description)
                .build();
    }

    ArrayList<Action> toActions(Resources resources) {
        ArrayList<Action> actions = new ArrayList<Action>();
        for (ActionBehavior behavior : mBehaviors) {
            if (behavior != ActionBehavior.INIT) {
                actions.add(behavior.toAction(
                        new ActionKey<ActionType, ActionBehavior>(this, behavior).getKey(),
                        resources));
            }
        }
        return actions;
    }

    ArrayList<Action> toSelectableActions(Resources resources, ActionBehavior selectedBehavior) {
        ArrayList<Action> actions = new ArrayList<Action>();
        for (ActionBehavior behavior : mBehaviors) {
            if (behavior != ActionBehavior.INIT) {
                actions.add(behavior.toAction(
                        new ActionKey<ActionType, ActionBehavior>(this, behavior).getKey(),
                        resources, (selectedBehavior == behavior)));
            }
        }
        return actions;
    }
}
