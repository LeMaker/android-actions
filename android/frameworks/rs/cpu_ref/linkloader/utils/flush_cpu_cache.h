/*
 * Copyright 2011, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FLUSH_CPU_CACHE_H
#define FLUSH_CPU_CACHE_H

#if defined(__arm__) || defined(__aarch64__) || defined(__mips__)
#define FLUSH_CPU_CACHE(BEGIN, END) __builtin___clear_cache((char *)(BEGIN), (char *)(END))
#else
#define FLUSH_CPU_CACHE(BEGIN, END) do { } while (0)
#endif

#endif // FLUSH_CPU_CACHE_H
