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

package android.os.cts;

import junit.framework.TestCase;

public class SeccompTest extends TestCase {

    public void testSeccomp() {
        if (CpuFeatures.isArm64Cpu() || CpuFeatures.isArm64CpuIn32BitMode()) {
            return; // seccomp not yet supported on arm64
        }
        if (OSFeatures.needsSeccompSupport()) {
            assertTrue("Please enable seccomp support "
                       + "in your kernel (CONFIG_SECCOMP_FILTER=y)",
                       OSFeatures.hasSeccompSupport());
        }
    }
}
