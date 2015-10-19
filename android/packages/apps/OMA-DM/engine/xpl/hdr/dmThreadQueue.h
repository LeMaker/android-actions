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

#ifndef DMTHREADQUEUE_INCLUDE 
#define DMTHREADQUEUE_INCLUDE 

/*==================================================================================================

    Header Name: dmThreadQueue.h

    General Description: This file contains declaration of the DMThread and DMThreadQueue classes

==================================================================================================*/

#include <stdarg.h>
#include <stddef.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "jem_defs.hpp"
#include "dmdefs.h"
#include "dmThreadHelper.h"

#define DM_WAIT_FOREVER  LONG_MAX               

enum {
  SYNCML_DM_THREAD_EVENT_TYPE_NONE,
  SYNCML_DM_THREAD_EVENT_TYPE_TIMEOUT,
  SYNCML_DM_THREAD_EVENT_TYPE_SHUTDOWN,
  SYNCML_DM_THREAD_EVENT_TYPE_FILELOCK
};
typedef UINT8 SYNCML_DM_THREAD_EVENT_TYPE_T;


class DMThreadEvent {
public:

  /**
  * Constructor
  * \param nID [in] - event type to be sent
  * \param pData [in] - pointer on data to be sent
  */  
  DMThreadEvent(SYNCML_DM_THREAD_EVENT_TYPE_T nID = SYNCML_DM_THREAD_EVENT_TYPE_NONE,
                   void* pData = NULL ) : m_nID( nID ), m_pUserData(pData) {}

  /**
  * Retrieves event type
  */  
  SYNCML_DM_THREAD_EVENT_TYPE_T GetEventType() const {return m_nID;}

  /**
  * Retrieves event data
  */  
  void* GetData() const {return m_pUserData;}
  
protected:
  /** Event type */
  SYNCML_DM_THREAD_EVENT_TYPE_T m_nID;
  /** Event data */
  void* m_pUserData;
};

// maximum number of events in the queue
#define DM_EVENT_QUEUE_MAX_LEN    50

class DMThreadQueue : public JemBaseObject {

public:
  /**
  * Default constructor
  */  
  DMThreadQueue() ;

  /**
  * Constructor
  * \param nID [in] - event type to be sent
  * \param pData [in] - pointer on data to be sent
  * \return TRUE if success
  */  
  bool Post(SYNCML_DM_THREAD_EVENT_TYPE_T nID, 
                   void* pData = NULL ) ;

  /**
  * Stops timer
  */  
  void KillTimer() ;

  /**
  * Sets timer
  * \param nTimeoutMS [in] - time out in mseconds
  */  
  void SetTimer( int nTimeoutMS ) ;

   /**
  * Waits event to arrive
  * \param nTimeoutMS [in] - time out in mseconds
  * \return TRUE if event received
  */  
  bool Wait(long nTimeoutMS,
                  DMThreadEvent& event );

   /**
  * Checks if timer is set 
  * \return TRUE if timer is set
  */  
  bool IsTimerSet() const { return m_bTimerSet; }

protected:
  /**
  * Destructor
  */  
  ~DMThreadQueue() ;

  
private:
  /**
  * Retrives time adjusted on timeout
 * \param stimeout [out] - time
  * \param nTimeoutMS [in] - time out in mseconds
  */
  void GetTimeWithinTimeout(struct timespec &stimeout,
                                       long nTimeoutMS ) const ;

  /**
  * Calculates remaining wait time
  * \param sWaitTo [out] - time to wait
  * \param nTimeoutMS [in] - time out in mseconds
  */ 
  void GetWaitTime(struct timespec &sWaitTo, 
                                  const struct timespec &stimeout ) const ;

  /**
  * Verifies if timer is ready
   * \return TRUE if timer is ready
  */ 
  bool TimerReady() const ;

  /* Events placeholder */
  DMThreadEvent  m_aEvents[DM_EVENT_QUEUE_MAX_LEN];
  /** Queue Index */ 
  int m_nHead;
  /** Event count */
  int m_nSize;
  /** threas condition */
  pthread_cond_t    m_oCond;
  /** Critical section */
  DMCriticalSection  m_csMutex;
  /** Time when timer is started */
  struct timespec m_sTimerFireTime;
  /** Flag to verify if timer is set */
  bool  m_bTimerSet;
};

typedef JemSmartPtr<DMThreadQueue> PDMThreadQueue;

#endif 
