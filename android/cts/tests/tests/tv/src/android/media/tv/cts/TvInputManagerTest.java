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

import android.content.Context;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.test.AndroidTestCase;

import java.util.List;

/**
 * Test for {@link android.media.tv.TvInputManager}.
 */
public class TvInputManagerTest extends AndroidTestCase {
    private static final String[] VALID_TV_INPUT_SERVICES = {
        StubTunerTvInputService.class.getName()
    };
    private static final String[] INVALID_TV_INPUT_SERVICES = {
        NoMetadataTvInputService.class.getName(), NoPermissionTvInputService.class.getName()
    };

    private String mStubId;
    private TvInputManager mManager;

    private static TvInputInfo getInfoForClassName(List<TvInputInfo> list, String name) {
        for (TvInputInfo info : list) {
            if (info.getServiceInfo().name.equals(name)) {
                return info;
            }
        }
        return null;
    }

    @Override
    public void setUp() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        mManager = (TvInputManager) mContext.getSystemService(Context.TV_INPUT_SERVICE);
        mStubId = getInfoForClassName(
                mManager.getTvInputList(), StubTunerTvInputService.class.getName()).getId();
    }

    public void testGetInputState() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertEquals(mManager.getInputState(mStubId), TvInputManager.INPUT_STATE_CONNECTED);
    }

    public void testGetTvInputInfo() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        assertEquals(mManager.getTvInputInfo(mStubId), getInfoForClassName(
                mManager.getTvInputList(), StubTunerTvInputService.class.getName()));
    }

    public void testGetTvInputList() throws Exception {
        if (!Utils.hasTvInputFramework(getContext())) {
            return;
        }
        List<TvInputInfo> list = mManager.getTvInputList();
        for (String name : VALID_TV_INPUT_SERVICES) {
            assertNotNull("getTvInputList() doesn't contain valid input: " + name,
                    getInfoForClassName(list, name));
        }
        for (String name : INVALID_TV_INPUT_SERVICES) {
            assertNull("getTvInputList() contains invalind input: " + name,
                    getInfoForClassName(list, name));
        }
    }
}
