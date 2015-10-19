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

package android.webgl.cts;

import android.util.Log;
import android.webgl.cts.R;
import android.webgl.WebGLActivity;
import java.lang.Override;
import java.io.File;
import java.io.InputStream;

/**
 * A Singleton class to wrap the WebGL Conformance Test Suite.
 */
public class WebGLConformanceSuite {
    private final String TAG = "WebGLConformanceSuite";
    private static volatile WebGLConformanceSuite mInstance = null;

    private WebGLConformanceSuite(WebGLActivity activity) throws Exception {
        Log.i(TAG, "Unzipping WebGL Conformance Suite: "
                + activity.getCacheDir().getPath());
        InputStream suite = activity.getResources().openRawResource(R.raw.webgl_sdk_tests);
        ZipUtil.unzipToPath(suite, activity.getCacheDir());
        InputStream harness = activity.getResources().openRawResource(R.raw.harness);
        ZipUtil.streamToPath(harness, activity.getCacheDir(), "harness.html");
    }

    public static WebGLConformanceSuite init(WebGLActivity activity)
            throws Exception {
        if (mInstance == null) {
            synchronized (WebGLConformanceSuite.class) {
                mInstance = new WebGLConformanceSuite(activity);
            }
        }
        return mInstance;
    }
}
