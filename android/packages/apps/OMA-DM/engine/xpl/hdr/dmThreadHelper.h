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

#ifndef DMTHREADHELPER_INCLUDE 
#define DMTHREADHELPER_INCLUDE

#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <memory.h>
#include <errno.h>

#ifdef DEBUG
  #define IMASSERT(expression) assert(expression)
  //#define NRES int nRes=
  #define NRES(foo) int nRes=(foo)
#else
  #define IMASSERT(expression)
  //#define NRES
  #define NRES(foo) foo
#endif


class DMCriticalSection
{
public:
  /**
  * Default constructor 
  */
  DMCriticalSection();

  /**
  * Destructor 
  */
  ~DMCriticalSection();

  /**
  * Locks mutex object
  */
  void Enter();

  /**
  * Unlocks mutex object
  */
  void Leave();

  /**
  * Tries to lock mutex object
  * \return TRUE if mutex object is locked 
  */
  bool TryEnter();

  /**
  * Retrieves handler on mutex object
  */
  pthread_mutex_t& GetHandle() { return m_section;}

private:
  /* handler */
  pthread_mutex_t   m_section;
};


class DMThread
{
 public:

  /**
  * Default constructor 
  */
  DMThread()
  { 
      m_bRunning = false; 
      memset(&m_hThread, 0, sizeof(m_hThread));
  }

  /**
  * Destructor 
  */
  virtual ~DMThread(){}

  /**
  * Starts thread
  * \return TRUE if success 
  */
  bool StartThread();
  
  /**
  * Stops thread
  * \return TRUE if success 
  */
  bool StopThread();

 protected:
  /**
  * Run method to be implemented in an inherited class
  */
  virtual void* Run() = 0;
 private:

  /**
  * Thread callback function 
  */
  static void* ThreadProc(void *pArg);

protected:
  /** Flag to specify if thread is running */
  bool  m_bRunning;
  /** Thread handler */
  pthread_t   m_hThread;
};


inline DMCriticalSection::DMCriticalSection( )
{
  memset( &m_section, 0, sizeof( m_section ) );

  NRES(pthread_mutex_init( &m_section, 0 ));
  IMASSERT( nRes == 0 );
}

inline DMCriticalSection::~DMCriticalSection()
{ 
  NRES(pthread_mutex_destroy( &m_section ));
  IMASSERT( nRes == 0 ); 
}

inline void DMCriticalSection::Enter() 
{ 
  NRES(pthread_mutex_lock( &m_section )); 
  IMASSERT( nRes == 0 );
}

inline void DMCriticalSection::Leave()
{ 
  NRES(pthread_mutex_unlock( &m_section ));

  IMASSERT( nRes == 0 ); 
}

inline bool DMCriticalSection::TryEnter()
{
  int nRes = pthread_mutex_trylock( &m_section ); 

  IMASSERT( nRes == 0 || nRes == EBUSY ); 
  
  return nRes == 0;
}


class  DMSingleLock  
{
  DMCriticalSection& m_oSection;
public:
  /**
  * Constructor 
  * \param oSection [in] - critical section(mutex)
  */
  DMSingleLock( DMCriticalSection& oSection ): m_oSection( oSection )
  { 
    m_oSection.Enter();
  }

  /**
  * Destructor 
  */
  ~DMSingleLock()
  {
    m_oSection.Leave();
  }
};

#define DmThSleep(a)  usleep(a)

#endif 
