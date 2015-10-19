#ifndef __DMPROFILE_H__
#define __DMPROFILE_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#ifdef DM_PERFORMANCE_ENABLED

#include "trace_perf.h"

#endif

enum {
    DM_INITIALIZE_ENTER = 18000000,
    DM_INITIALIZE_EXIT = 18000001,
    DM_GET_TREE_ENTER = 18000002,
    DM_GET_TREE_EXIT = 18000003,
    DM_GET_NODE_ENTER = 18000004,
    DM_GET_NODE_EXIT = 18000005,
    DM_UNINITIALIZE_ENTER = 18000006,
    DM_UNINITIALIZE_EXIT = 18000007,
    DM_INITIALIZE_MOUNT = 18000008,
    DM_INITIALIZE_MDF = 18000009,
    DM_INITIALIZE_PLUGIN = 18000010,
    DM_INITIALIZE_ACRHIVER = 18000011,
    DM_INITIALIZE_FILE = 18000012,
    DM_INITIALIZE_LOCK = 18000013,
    DM_INITIALIZE_ACL = 18000014,
    DM_INITIALIZE_EVENT = 18000015,
    DM_INITIALIZE_LOAD = 18000016,

};


#ifdef DM_PERFORMANCE_ENABLED
#define DM_PERFORMANCE(event)   TRACE_PERF(event) 
#else
#define DM_PERFORMANCE(event)
#endif


#ifdef DM_PROFILER_ENABLED
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

#ifdef DEBUG
extern int s_nBlocks , s_nSize, s_nCnt;
#endif

// static storage for performance statistic
#define PROF_CELL_NUM   20000
struct CDMProfCell {
  const char* _s;
  long long  _elapsed, _from;
};

extern CDMProfCell  g_aProfStorage[PROF_CELL_NUM];
extern int g_nProfCurCell;

struct CDMProfile {

  CDMProfile( const char* s) {
    _s = s;
    gettimeofday( &_tv1, NULL );
#ifdef DEBUG
    _nasBlocks = s_nBlocks;
    _nWasSize = s_nSize;
    _nWasTotal = s_nCnt;
#endif
  }
  ~CDMProfile() {
    struct timeval  tv2;
    gettimeofday( &tv2, NULL );
    long long n1 = _tv1.tv_usec + (_tv1.tv_sec * 1000000 );
    long long n2 = tv2.tv_usec + (tv2.tv_sec * 1000000 );
    long long elapsed = n2 - n1;
    //printf( "DMProfile: %s, time is %lld usec, from %lld to %lld\n", _s, elapsed, n1, n2 );
    int nCurCell = g_nProfCurCell++ % PROF_CELL_NUM;
    g_aProfStorage[nCurCell]._s = strdup( _s );
    g_aProfStorage[nCurCell]._elapsed = elapsed;
    g_aProfStorage[nCurCell]._from = n1;

#ifdef DEBUG
#ifdef DM_DM_MEMORY_USAGE_ENABLED
    printf( "DMProfile: %s, time is %lld usec, from %lld to %lld\n", _s, elapsed, n1, n2 );

    printf( "Total Blocks %d (delta is %d), size %d (delta is %d), total allocated (including deallocated) %d, (d %d)\n\n", 
      s_nBlocks,  s_nBlocks - _nasBlocks,
      s_nSize, s_nSize - _nWasSize,
      s_nCnt, s_nCnt - _nWasTotal );
#endif
#endif
  }

  struct timeval  _tv1;
  const char* _s;
#ifdef DEBUG
  int _nasBlocks, _nWasSize, _nWasTotal;
#endif
};

#define DM_PROFILE_EXT(msg,num)  CDMProfile __oProf##num(msg );
#define DM_PROFILE(msg)  CDMProfile __oProf(msg );

#else
#define DM_PROFILE(msg)
#define DM_PROFILE_EXT(msg,num)
#endif


#ifdef DM_PROFILER_STACK

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include "dmvector.h"

// static storage for performance statistic
#define PROF_STACK_NUM   20000
struct DMProfileData {
  const char* _s;
  long long  _elapsed, _from;
};

extern DMProfileData  __aProfStackStorage[PROF_STACK_NUM];
extern int g_nProfCurCell;
 
struct CDMProfileCapture {

  CDMProfileCapture(DMStringVector itemNames, DMStringVector itemValues) {
    _itemNames = itemNames;
    _itemValues = itemValues;
    gettimeofday( &_tv1, NULL );
  }

  ~CDMProfileCapture() {
    struct timeval  tv2;
    gettimeofday( &tv2, NULL );
    long long n1 = _tv1.tv_usec + (_tv1.tv_sec * 1000000 );
    long long n2 = tv2.tv_usec + (tv2.tv_sec * 1000000 );
    long long elapsed = n2 - n1;

    DMString tmp = "";
    DMString tmpValue = "";

    for (int i = 0; i < _itemNames.size(); i++) {
      tmp += "    <";
      tmp += _itemNames[i];
      tmp += ">";
      tmpValue = _itemValues[i];
      
      tmpValue.replaceAll('<', '#');
      tmpValue.replaceAll('>', '#');

      tmp += tmpValue;

      const char* ch = _itemValues[i].c_str();

      int size = strlen(ch);

      if (ch[size - 1] == '\n') {
	tmp += "    </";
      } else {
        tmp += "</";
      }
      
      tmp += _itemNames[i];
      tmp += ">\n";        
    }
    
    int nCurCell = g_nProfCurCell++ % PROF_STACK_NUM;
    __aProfStackStorage[nCurCell]._s = strdup(tmp.c_str());

    __aProfStackStorage[nCurCell]._elapsed = elapsed;
    __aProfStackStorage[nCurCell]._from = n1;
  }

  struct timeval  _tv1;
  const char* _s;
  DMStringVector _itemNames;
  DMStringVector _itemValues;
};

#define DM_PROFILE_STACK(itemNames, itemValues)  CDMProfileCapture __oProfStack(itemNames, itemValues);

#else
#define DM_PROFILE_STACK(itemNames, itemValues)
#endif


#ifdef DM_FILE_OUTPUT
void DMFileOutput( const char* szFormat, ... );
#else
#define DMFileOutput
#endif

#endif // __DMPROFILE_H__

