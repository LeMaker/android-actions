/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef PIXMAN_ANDROID_H_
#define PIXMAN_ANDROID_H_

typedef struct _android_simple_image android_simple_image;

struct _android_simple_image {
    int width;
    int height;
    int bpp;
    void* user_object;
    void (*get_scanline)(android_simple_image* self, void** buffer, int line);
};

void android_simple_scale(android_simple_image* src_image,
        android_simple_image* dst_image, float scale, int src_x, int src_y);

#endif /* PIXMAN_ANDROID_H_ */
