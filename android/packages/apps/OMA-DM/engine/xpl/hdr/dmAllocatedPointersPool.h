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

#ifndef DMALLOCATED_POINTERS_POOL_INCLUDE 
#define DMALLOCATED_POINTERS_POOL_INCLUDE 

#include "dmThreadQueue.h"
#include "dmThreadHelper.h"
#include <malloc.h>
#include <stdio.h>  
#include <time.h>
#include <sys/time.h>

#include "dmstring.h"
#include "dmvector.h"

#define DM_MAX_TIMERS 10

#ifdef DEBUG
#include <map>

class DMAllocatedPointersPool
{
public:
  enum { c_nExtraBytes = 16 };

   /**
  * Default constructor
  */
   DMAllocatedPointersPool(){}

   /**
  * Destructor
  */
  ~DMAllocatedPointersPool();

   /**
  * Checks if pointer exist in the pool
  * \param ptr [in] - pointer
  * \return TRUE if pointer is found 
  */  
  bool exists(void* ptr);

  /**
  * Appends pointer to the pool
  * \param ptr [in] - pointer
  */  
  void append(void* ptr);

  /**
  * Removes pointer from the pool
  * \param ptr [in] - pointer
  */
  bool remove(void* ptr);

  /**
  * Prints unreleased pointers
  */
  void PrintUnreleased();


private:
  /** Critical section */
  DMCriticalSection  m_csPointerPoolLock;
  /** Pool of allocated pointers */
  std::map<void*, int> 	m_listOfAllocatedPointers;
};


/*====================================================================================================
 Inline functions implementation
==================================================================================================*/
inline bool DMAllocatedPointersPool::exists(void* ptr)
{
  DMSingleLock oLock( m_csPointerPoolLock );
  
  return m_listOfAllocatedPointers.find( ptr ) != m_listOfAllocatedPointers.end();
}

  
inline void DMAllocatedPointersPool::append(void* ptr)
{
  DMSingleLock oLock( m_csPointerPoolLock );
  
  m_listOfAllocatedPointers[ptr] = 0; // add new pointer
}
  
inline bool DMAllocatedPointersPool::remove(void* ptr)
{
  DMSingleLock oLock( m_csPointerPoolLock );
    
  std::map<void*, int>::iterator it = m_listOfAllocatedPointers.find(ptr);
    
  if ( it == m_listOfAllocatedPointers.end() )
    return false;
  
  m_listOfAllocatedPointers.erase( it );
  return true;
}

#endif //DEBUG

#endif 
