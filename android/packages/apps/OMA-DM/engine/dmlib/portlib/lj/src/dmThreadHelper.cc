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

    Source Name: dmThreadHelper.cc

    General Description: Implementation of the DMThread class 

==================================================================================================*/

#include "dmThreadHelper.h"
#include "dmMemory.h"
#include "xpl_Logger.h"

bool DMThread::StartThread()
{
  if ( m_bRunning )
    return false;

  m_bRunning = true;

  int nRes = pthread_create( &m_hThread, 0, ThreadProc, (void*)this );

  if ( nRes != 0 )
  {    
    XPL_LOG_DM_TMN_Debug(("DMThread: can't create a thread; error %d \n", nRes));

    m_bRunning = false;
    return false;
  }
  return true;
}

bool DMThread::StopThread()
{
  if ( !m_bRunning )
    return false;

  m_bRunning = false;

  bool bRet = (pthread_join( m_hThread, NULL ) == 0);

  return bRet;
}

void* DMThread::ThreadProc(void *pArg)
{
  DMThread* pThis = (DMThread*)pArg;
  void* pRes = 0;

  if ( pThis->m_bRunning )
    pRes = pThis->Run(); 

  return pRes;
}
