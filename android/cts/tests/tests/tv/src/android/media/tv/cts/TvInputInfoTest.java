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

package android.media.tv.cts;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.test.AndroidTestCase;

/**
 * Test for {@link android.media.tv.TvInputInfo}.
 */
public class TvInputInfoTest extends AndroidTestCase {
    private TvInputInfo mStubInfo;
    private PackageManager mPackageManager;

    @Override
    public void setUp() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        TvInputManager manager =
                (TvInputManager) getContext().getSystemService(Context.TV_INPUT_SERVICE);
        for (TvInputInfo info : manager.getTvInputList()) {
            if (info.getServiceInfo().name.equals(
                    StubTunerTvInputService.class.getName())) {
                mStubInfo = info;
                break;
            }
        }
        mPackageManager = getContext().getPackageManager();
    }

    public void testGetIntentForSettingsActivity() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        Intent intent = mStubInfo.createSettingsIntent();

        assertEquals(intent.getComponent(), new ComponentName(getContext(),
                TvInputSettingsActivityStub.class));
        String inputId = intent.getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        assertEquals(mStubInfo.getId(), inputId);
    }

    public void testGetIntentForSetupActivity() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        Intent intent = mStubInfo.createSetupIntent();

        assertEquals(intent.getComponent(), new ComponentName(getContext(),
                TvInputSetupActivityStub.class));
        String inputId = intent.getStringExtra(TvInputInfo.EXTRA_INPUT_ID);
        assertEquals(mStubInfo.getId(), inputId);
    }

    public void testTunerHasNoParentId() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertNull(mStubInfo.getParentId());
    }

    public void testGetTypeForTuner() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertEquals(mStubInfo.getType(), TvInputInfo.TYPE_TUNER);
    }

    public void testTunerIsNotPassthroughInput() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertFalse(mStubInfo.isPassthroughInput());
    }

    public void testLoadIcon() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertEquals(mStubInfo.loadIcon(getContext()).getConstantState(),
                mStubInfo.getServiceInfo().loadIcon(mPackageManager).getConstantState());
    }

    public void testLoadLabel() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertEquals(mStubInfo.loadLabel(getContext()),
                mStubInfo.getServiceInfo().loadLabel(mPackageManager));
    }
}
