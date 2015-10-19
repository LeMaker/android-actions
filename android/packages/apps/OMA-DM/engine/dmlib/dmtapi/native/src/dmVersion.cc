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

#include <stdio.h>
#include "dmVersion.h"

#define DMSyncMLPackageName "Device Management SyncML Engine"
#define DMSyncMLCopyright  "Copyright (C) 2014 The Android Open Source Project"

#ifndef DMSyncMLLibVersion
#define DMSyncMLLibVersion  "Development"
#endif

#define DMSyncMLVersion DMSyncMLPackageName " " DMSyncMLLibVersion " " DMSyncMLCopyright

const char *GetDMSyncMLVersion(unsigned long *version)
{
    const char *p = DMSyncMLLibVersion;
    int i = 0;
    if (version != (unsigned long *)NULL)
    {
        *version = 0;
        while (p[i] != '\0') 
        {
            if (p[i] < '0' || p[i] > '9') 
            {
                ++i;
                continue;
            }
            *version <<= 4;
            (*version) |= (unsigned long)(p[i] - '0');
            ++i;
        }
    }
    return DMSyncMLVersion;
}
