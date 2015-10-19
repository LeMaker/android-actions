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

/*==================================================================================================

    Source Name: dmThreadQueue.cc

    General Description: Implementation of the DMThreadQueue class 

==================================================================================================*/

#include "dmprofile.h"
#include "dmAllocatedPointersPool.h"
#include <stdio.h>  

DMThreadQueue::~DMThreadQueue()
{
  pthread_cond_destroy( &m_oCond );
}

DMThreadQueue::DMThreadQueue() : 
  m_nHead(0), m_nSize(0) 
{
  pthread_cond_t t =  PTHREAD_COND_INITIALIZER;
  m_oCond = t;
  m_bTimerSet = false;
}

bool DMThreadQueue::Post( SYNCML_DM_THREAD_EVENT_TYPE_T nID, void* pData /*= NULL*/ ) 
{
  DMSingleLock oLock( m_csMutex );

  if ( m_nSize == DIM(m_aEvents) ) { // overflow - wait a little bit
    m_csMutex.Leave();
    sleep( 5 );
    m_csMutex.Enter();
  }
  
  if ( m_nSize == DIM(m_aEvents) ) // still overflow - something unexpected happen
    return false;

  m_aEvents[m_nHead] = DMThreadEvent( nID, pData );
  m_nHead ++; m_nHead %= DIM(m_aEvents);
  m_nSize++;
  
  pthread_cond_signal( &m_oCond );
  return true;
}

void DMThreadQueue::KillTimer() 
{
  DMSingleLock oLock( m_csMutex );

  m_bTimerSet = false;
}

void DMThreadQueue::SetTimer( int nTimeoutMS ) 
{
  DMSingleLock oLock( m_csMutex );

  GetTimeWithinTimeout( m_sTimerFireTime, nTimeoutMS );
  m_bTimerSet = true;

  pthread_cond_signal( &m_oCond );
}

bool DMThreadQueue::Wait( long nTimeoutMS, DMThreadEvent& event ) 
{
  DMSingleLock oLock( m_csMutex );
  struct timespec stimeout;
  GetTimeWithinTimeout( stimeout, nTimeoutMS );

  while ( !m_nSize && nTimeoutMS ) { // wait if empty
    struct timespec swaitTo;

    GetWaitTime( swaitTo, stimeout );
  
    pthread_cond_timedwait( &m_oCond, &m_csMutex.GetHandle(), &swaitTo);

    if ( TimerReady() ||(memcmp( &swaitTo, &stimeout, sizeof(swaitTo) ) ==0) )
      break;
  }

  if ( TimerReady() ) {
    m_bTimerSet = false;
    event = DMThreadEvent( SYNCML_DM_THREAD_EVENT_TYPE_TIMEOUT );
    return true;
  }
  
  if ( !m_nSize ) // 
    return false;
  
  int nIndex = (m_nHead + DIM(m_aEvents) - m_nSize) % DIM(m_aEvents);
  m_nSize--;
  event = m_aEvents[nIndex];
  return true;
}

void DMThreadQueue::GetTimeWithinTimeout( struct timespec &stimeout, long nTimeoutMS ) const
{
  struct timeval now;
  gettimeofday(&now,0);
  stimeout.tv_nsec = now.tv_usec + (nTimeoutMS % 1000) * 1000; // store usec
  stimeout.tv_sec = now.tv_sec + (nTimeoutMS / 1000) + (stimeout.tv_nsec / 10000000);
  stimeout.tv_nsec %= 10000000; 
  stimeout.tv_nsec *= 1000; // convert to nsec
}
  
void DMThreadQueue::GetWaitTime(struct timespec &sWaitTo, const struct timespec &stimeout ) const 
{
  if ( m_bTimerSet && 
    ( (m_sTimerFireTime.tv_sec < stimeout.tv_sec) ||
      ( (m_sTimerFireTime.tv_sec == stimeout.tv_sec) && (m_sTimerFireTime.tv_nsec < stimeout.tv_nsec) ) ) )
      sWaitTo = m_sTimerFireTime;
  else
    sWaitTo = stimeout;
}

bool DMThreadQueue::TimerReady() const 
{
  if ( !m_bTimerSet )
    return false;

  struct timespec tm;
  GetTimeWithinTimeout( tm, 0 );

  return (tm.tv_sec > m_sTimerFireTime.tv_sec) ||
    ( (tm.tv_sec == m_sTimerFireTime.tv_sec) && (tm.tv_nsec > m_sTimerFireTime.tv_nsec) );
  
}
