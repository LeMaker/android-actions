/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef ACT_DATA_SOURCE_H_
#define ACT_DATA_SOURCE_H_
    
#include <media/stagefright/DataSource.h>

#include "./common/al_libc.h"
#include "./common/storageio.h"
#include "./common/stream_input.h"

namespace android {
#ifdef __cplusplus
extern "C" {
#endif

storage_io_t *create_storage_io(void);
off64_t init_storage_io(storage_io_t *io, const sp<DataSource>& source);
void dispose_storage_io(storage_io_t *io);

stream_input_t *stream_input_open(void);
off64_t stream_input_init(stream_input_t *input, const sp<DataSource>& source);
void stream_input_dispose(stream_input_t *input);

#ifdef __cplusplus
}
#endif // __cplusplus
} // namespace android

#endif  // ACT_DATA_SOURCE_H_
