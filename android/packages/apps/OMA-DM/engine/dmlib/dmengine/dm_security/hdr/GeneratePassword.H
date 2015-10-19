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

#ifndef GENERATEPASSWORD_H
#define GENERATEPASSWORD_H

#include "xpl_Types.h"

class GeneratePassword 
{
  
 public:

  GeneratePassword();

  ~GeneratePassword();

  char * generateClientPasswordKey(const char * deviceId);

  char * generateKeyFromDict(const char * deviceId, const char dict[]);
  
  char * generateServerPasswordKey(const char * deviceId);
  
  char * generateClientPassword(const char * deviceId, const char * serverId);
  
  char * generateServerPassword(const char * deviceId, const char * serverId);

  char * generateClientPassword();
  
  char * generateServerPassword();
  
  char * encodeDeviceId(const char * deviceId);
  
  char * get36BasedNumber(const char * deviceId);
  
  char * generatePassword(const char * deviceId, const char * serverId, const char * key);
  
  char * encodeHex(const char data[]);

  UINT64 convertchar2Long(const char * input);

  void shuffle(char & buffer);

  void setServerId(const char * sid);

  void setIMEI(const char * did);

  const char * getIMEI();

  const char * getServerId();

 private:

  char * serverId;
  char * imei;
  static const char clientPasswordDict [15];
  static const char serverPasswordDict[15];
  static const char hexTable[16];
};
  
#endif
