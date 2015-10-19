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

#include <stdarg.h>
#include <time.h>
#include "dmStringUtil.h"
#include "dmprofile.h"
#include "dmThreadHelper.h"
#include "dmstring.h"

#ifdef DM_FILE_OUTPUT

void DMFileOutput( const char* szFormat, ... )
{
  static int s_nFirstCall = 1; // mark first access to write some additional info
  static char filename[255];

  FILE * fp=NULL;

  if ( s_nFirstCall ){
    s_nFirstCall = 0;

  const char* dm_settings_env = getenv("dm_setting_root");

  if ( !dm_settings_env ) 
    dm_settings_env = "";

    DmStrcpy (filename, dm_settings_env);
    DmStrcat( filename, "/a/motorola/settings/DMOUTPUT.txt" );

    fp=fopen(filename, "a+");
    if (!fp)
      return;
    
    time_t elapstime;
    time(&elapstime);

    fprintf(fp, "\nLog started at %s \n",
       ctime(&elapstime));
    fclose(fp);
    fp=NULL;
  }
  
  fp=fopen(filename, "a+");
  if (!fp)
    return;
  
  va_list ap;
  va_start(ap, szFormat);
  vfprintf( fp, szFormat, ap );
  va_end(ap);    

   fclose(fp);
   fp=NULL;
}

#endif

#ifdef DM_PROFILER_ENABLED

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

CDMProfCell  g_aProfStorage[PROF_CELL_NUM];
int g_nProfCurCell = 0;

class CProfileCmdListener : public DMThread{
public:
  CProfileCmdListener() {
    StartThread();
  }
  ~CProfileCmdListener() {
    StopThread();
  }

  enum { eNone, eError, eReset, ePrint, ePrintPipe};
  virtual void* Run();
  int GetCommand( int nPipe );
  void PrintResult( int nFile );

  DMString _sOutPipe;
};

#define PROF_CMD_PIPE_NAME "/tmp/dmprof"

void* CProfileCmdListener::Run()
{
printf("DM profile command listener is started\n"
  "use file " PROF_CMD_PIPE_NAME " to manipulate it\n");

{
  // check for cvm
  char szFile[1024];

  sprintf( szFile, "/proc/%d/cmdline", (int)getpid() );
  int nF = open( szFile, O_RDONLY );
  if ( nF >= 0 ){
    read( nF, szFile, sizeof(szFile ) );
    close (nF);

    if ( strstr( szFile, "cvm") == NULL )
      return NULL;
  }
}

  if (mkfifo(PROF_CMD_PIPE_NAME, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
    printf("Failed to create pipe\n");

  int nPipe = open(PROF_CMD_PIPE_NAME, O_NONBLOCK );

  if ( nPipe < 0 ){
    printf( "Failed to open pipe - cmd listener terminated\n");
    return 0;
  }
    
  while (m_bRunning)
  {
    int nCmd = GetCommand( nPipe );

    if ( nCmd != eNone )
      printf( "Cmd received %d\n\n\n", nCmd );

   //printf("Stat %d:\n", g_nProfCurCell);

    switch ( nCmd ){
      case eReset:
        printf("Reset\n");
        g_nProfCurCell = 0;
        break;

      case ePrint:
        PrintResult(-1);
        break;

        case ePrintPipe:
          {
            int nFile = open( _sOutPipe, O_WRONLY | O_CREAT  | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
            PrintResult(nFile);
            if ( nFile > -1 )
              close( nFile );
          }
          break;
    };
  }

  close( nPipe );
  
  return 0;
}

int CProfileCmdListener::GetCommand( int nPipe )
{
  if ( nPipe < 0 | nPipe > 1023 )
    return -1;
  
    int rc;
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(nPipe,&fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    rc = select(nPipe+1, &fds, NULL, NULL, &tv);
    if (rc < 0)
      return eError;

  if (!FD_ISSET(nPipe,&fds) )
    return eNone;

  char szCmd[50] = "";
  
  int nBytes = read( nPipe, szCmd, sizeof(szCmd));

  if ( nBytes == 0 ){
    sleep(1);
    return eNone;
  }
  
  if ( strncmp( szCmd, "reset", 5) == 0 )
    return eReset;

  if ( strncmp( szCmd, "print", 5) == 0 )
    return ePrint;
  
  if ( strncmp( szCmd, "out", 3) == 0 ){
    _sOutPipe.assign(szCmd + 4, nBytes - 4);
    printf( "out to [%s]\n", _sOutPipe.c_str() );
    return ePrintPipe;
  }

  return eError;
  
}

void CProfileCmdListener::PrintResult( int nFile )
{
  int n = 0;
  long long nTot = 0;
  char szBuf[10096];
  
  while ( n < g_nProfCurCell ){
     
    if ( g_aProfStorage[n % PROF_CELL_NUM]._s  ) {
      int nLen = sprintf( szBuf, "%d: DMProfile: %s, time is %lld usec, from %lld to %lld\n", 
        n+1,
        g_aProfStorage[n % PROF_CELL_NUM]._s, 
        g_aProfStorage[n % PROF_CELL_NUM]._elapsed, 
        g_aProfStorage[n % PROF_CELL_NUM]._from, 
        g_aProfStorage[n % PROF_CELL_NUM]._elapsed + g_aProfStorage[n % PROF_CELL_NUM]._from );

      if ( nFile >= 0 )
        write( nFile, szBuf, nLen );
      else
        printf( "%s", szBuf );
      
      nTot += g_aProfStorage[n % PROF_CELL_NUM]._elapsed;
    }

    free( (void*)g_aProfStorage[n % PROF_CELL_NUM]._s );
    g_aProfStorage[n % PROF_CELL_NUM]._s = NULL;
    n++;
  }

  int nLen = sprintf( szBuf, "Total time is %lld\n", nTot );
  if ( nFile >= 0 )
    write( nFile, szBuf, nLen );
  else
    printf( "%s", szBuf );
  
  g_nProfCurCell = 0;
}

CProfileCmdListener  g_oProfileCmdListener;

#endif

#ifdef DM_PROFILER_STACK

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


DMProfileData  __aProfStackStorage[PROF_STACK_NUM];

int g_nProfCurCell = 0;


class CProfileStackCmdListener : public DMThread{
public:
  CProfileStackCmdListener() {
    StartThread();
  }
  ~CProfileStackCmdListener() {
    StopThread();
  }

  enum { eNone, eError, eReset, ePrint, ePrintPipe};
  virtual void* Run();
  int GetCommand( int nPipe );
  void PrintResult( int nFile );

  DMString _sOutPipe;
};

#define PROF_CMD_PIPE_NAME "/tmp/dmprofstack"
 
void* CProfileStackCmdListener::Run()
{
  printf("DM profile STACK command listener is started\n"
	 "use file " PROF_CMD_PIPE_NAME " to manipulate it\n");

  {
    // check for cvm
    char szFile[1024];

    sprintf( szFile, "/proc/%d/cmdline", (int)getpid() );
    int nF = open( szFile, O_RDONLY );
    if ( nF >= 0 ){
      read( nF, szFile, sizeof(szFile ) );
      close (nF);
      
      if ( strstr( szFile, "cvm") == NULL )
	return NULL;
    }
  }

  if (mkfifo(PROF_CMD_PIPE_NAME, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
    printf("Failed to create pipe\n");

  int nPipe = open(PROF_CMD_PIPE_NAME, O_NONBLOCK );

  if ( nPipe < 0 ){
    printf( "Failed to open pipe - cmd listener terminated\n");
    return 0;
  }
    
  while (m_bRunning)
  {
    int nCmd = GetCommand( nPipe );

    if ( nCmd != eNone )
      printf( "Cmd received %d\n\n\n", nCmd );

    switch ( nCmd ){
      case eReset:
        printf("Reset\n");
        g_nProfCurCell = 0;
        break;

      case ePrint:
        PrintResult(-1);
        break;

        case ePrintPipe:
          {
            int nFile = open( _sOutPipe, O_WRONLY | O_CREAT  | O_APPEND );
            PrintResult(nFile);
            if ( nFile > -1 )
              close( nFile );
          }
          break;
    };
  }

  close( nPipe );
  
  return 0;
}

int CProfileStackCmdListener::GetCommand( int nPipe )
{
  if ( nPipe < 0 | nPipe > 1023 )
    return -1;
  
    int rc;
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(nPipe,&fds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    rc = select(nPipe+1, &fds, NULL, NULL, &tv);
    if (rc < 0)
      return eError;

  if (!FD_ISSET(nPipe,&fds) )
    return eNone;

  char szCmd[50] = "";
  
  int nBytes = read( nPipe, szCmd, sizeof(szCmd));

  if ( nBytes == 0 ){
    sleep(1);
    return eNone;
  }
  
  if ( strncmp( szCmd, "reset", 5) == 0 )
    return eReset;

  if ( strncmp( szCmd, "print", 5) == 0 )
    return ePrint;
  
  if ( strncmp( szCmd, "out", 3) == 0 ){
    _sOutPipe.assign(szCmd + 4, nBytes - 4);
    printf( "out to [%s]\n", _sOutPipe.c_str() );
    return ePrintPipe;
  }

  return eError;  
}

void CProfileStackCmdListener::PrintResult( int nFile )
{
  int n = 0;
  long long nTot = 0;
  char szBuf[10096];
 
  int nLen = sprintf( szBuf, "<DMProfiles>\n");  
  if ( nFile >= 0 )
    write( nFile, szBuf, nLen );
  else
    printf( "%s", szBuf );
  
  while ( n < g_nProfCurCell ){
    
    if ( __aProfStackStorage[n % PROF_STACK_NUM]._elapsed  ) {

      nLen = sprintf( szBuf, "  <DMProfile>\n");
      if ( nFile >= 0 )
	write( nFile, szBuf, nLen );
      else
	printf( "%s", szBuf );

        nLen = sprintf( szBuf, "    <ID>%d</ID>\n    <ElapseTime>%lld</ElapseTime>\n    <BeginTime>%lld</BeginTime>\n    <EndTime>%lld</EndTime>\n%s", 
        n+1,
        __aProfStackStorage[n % PROF_STACK_NUM]._elapsed, 
        __aProfStackStorage[n % PROF_STACK_NUM]._from, 
			__aProfStackStorage[n % PROF_STACK_NUM]._elapsed + __aProfStackStorage[n % PROF_STACK_NUM]._from,
        __aProfStackStorage[n % PROF_STACK_NUM]._s);

      if ( nFile >= 0 )
        write( nFile, szBuf, nLen );
      else
        printf( "%s", szBuf );
      

      nLen = sprintf( szBuf, "  </DMProfile>\n");
      if ( nFile >= 0 )
	write( nFile, szBuf, nLen );
      else
	printf( "%s", szBuf );      

      nTot += __aProfStackStorage[n % PROF_STACK_NUM]._elapsed;
    }

    free( (void*)__aProfStackStorage[n % PROF_STACK_NUM]._s );
    __aProfStackStorage[n % PROF_STACK_NUM]._s = NULL;
    n++;
  }

  nLen = sprintf( szBuf, "</DMProfiles>\n");  
  if ( nFile >= 0 )
    write( nFile, szBuf, nLen );
  else
    printf( "%s", szBuf );
  
  g_nProfCurCell = 0;
}

CProfileStackCmdListener  g_oProfileStackCmdListener;

#endif
