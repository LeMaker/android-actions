/*
 * Copyright (C) 2006 The Android Open Source Project
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

#ifndef __IOOM_ADJ_DROPPER__H_
#define __IOOM_ADJ_DROPPER__H_

#include <stdint.h>
#include <sys/types.h>

#include <utils/RefBase.h>
#include <utils/Errors.h>
#include <binder/IInterface.h>
#include <ui/PixelFormat.h>

#include <binder/IBinder.h>
// ----------------------------------------------------------------------------
using namespace android;

class IOomAdjDropper : public IInterface
{
public:
    DECLARE_META_INTERFACE(OomAdjDropper);
	  enum {
          ADJUST_OOM=IBinder::FIRST_CALL_TRANSACTION+0
    };

    virtual int adjustOom(int  offset)=0;
   
};



#endif //  __IIMAGE_BUFFER_PRODUCER__H_