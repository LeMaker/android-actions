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

#ifndef SYNCML_DM_NEW_DATA_TYPES_VALIDATION_H
#define SYNCML_DM_NEW_DATA_TYPES_VALIDATION_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "dmstring.h"

BOOLEAN 
is_float( const DMString& str );

BOOLEAN 
is_date( const DMString& str );

BOOLEAN 
is_time( const DMString& str );

#endif // SYNCML_DM_NEW_DATA_TYPES_VALIDATION_H
