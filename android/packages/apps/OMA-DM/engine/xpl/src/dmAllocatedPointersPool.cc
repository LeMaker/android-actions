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

#include "dmMemory.h"
#include "dmThreadHelper.h"
#include <stdio.h>  
#include <sys/time.h>
#include <signal.h>
#include "dmAllocatedPointersPool.h"  

#ifdef DEBUG
DMAllocatedPointersPool::~DMAllocatedPointersPool()
{
#ifdef DEBUG
  //PrintUnreleased();
#endif
}

void DMAllocatedPointersPool::PrintUnreleased()
{
  DMSingleLock oLock( m_csPointerPoolLock );

  std::map<void*, int>::const_iterator vli = m_listOfAllocatedPointers.begin();
  int nBlockNum = 1;

  while (vli != m_listOfAllocatedPointers.end()) 
  {
    const char *ptr = (const char*)(const void*)vli->first;
    CPCHAR* ppStr = (CPCHAR*)ptr;

    ptr += 3*sizeof(const char*) + DMAllocatedPointersPool::c_nExtraBytes;

    printf( "%d. block (%d bytes) with addr %p was not deallocated; file %s, line %d\n", 
      nBlockNum++, (int)ppStr[2], ptr, ppStr[0], (int)ppStr[1] );
    vli++;
  }
  if ( nBlockNum > 1 )
    printf( "\n" );

  //m_listOfAllocatedPointers.clear();
}
#endif //DEBUG
