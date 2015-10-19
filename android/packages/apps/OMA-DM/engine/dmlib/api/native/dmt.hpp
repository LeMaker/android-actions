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

#ifndef __DMT_API_H__
#define __DMT_API_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** \file dmt.hpp
      \brief  External API for DMTree

The dmt.hpp header file contains External API for DMTree C++ API

<b>Design Consideration</b>\n 
1. Only smart pointers are used, so user never has to allocate or deallocate  memory directly.\n\n
*/

#include <stdlib.h>
#include <string.h>

#include "jem_defs.hpp"
#include "dmtPrincipal.hpp"
#include "dmtAcl.hpp"
#include "dmtData.hpp"
#include "dmtAttributes.hpp"
#include "dmtDataChunk.hpp"
#include "dmtNode.hpp"
#include "dmtTree.hpp"
#include "dmtTreeFactory.hpp"
#include "dmtNotification.hpp"
#include "dmtSessionProp.hpp"
#include "dmtEvent.hpp"
#include "dmtEventData.hpp"

#endif
