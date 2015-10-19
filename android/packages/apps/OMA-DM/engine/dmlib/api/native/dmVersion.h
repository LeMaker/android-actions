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

#ifndef _DM_SYNCML_VERSION_H
#define _DM_SYNCML_VERSION_H

#ifdef __cplusplus 
extern "C" {
#endif

 /**
  * Retrieves a SyncML version
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par API Migration State: FINAL
  * \param version [out] - optional pointer to version in the numeric format
  * \return pinter to version in string format
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
const char *GetDMSyncMLVersion(unsigned long *version);

#ifdef __cplusplus
}
#endif

#endif
