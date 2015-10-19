/*
 * Copyright (C) 2007 The Android Open Source Project
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

// tag as surfaceflinger
#define LOG_TAG "IOomAdjDropper"

#include <stdint.h>
#include <sys/types.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#include <IOomAdjDropper.h>

#define LIKELY( exp )       (__builtin_expect( (exp) != 0, true  ))
#define UNLIKELY( exp )     (__builtin_expect( (exp) != 0, false ))


class BpOomAdjDropper : public BpInterface<IOomAdjDropper>
{
public:
    BpOomAdjDropper(const sp<IBinder>& impl)
        : BpInterface<IOomAdjDropper>(impl)
    {
    }

virtual int adjustOom(int dropOrRaise)
  {
      Parcel data, reply;
      data.writeInterfaceToken(IOomAdjDropper::getInterfaceDescriptor());
      data.writeInt32(dropOrRaise);
      remote()->transact(ADJUST_OOM, data, &reply);	
      reply.readExceptionCode();	
      int result=reply.readInt32();
      return result;
  }
};

IMPLEMENT_META_INTERFACE(OomAdjDropper, "android.os.IOomAdjDropper");
