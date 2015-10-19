#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>

 
#include "dmt.hpp"


const char* c_szURI = "./DevDetail/FwV";
DMString g_strValue;
const int c_nProcesses = 3;
const int c_nThreads = 3;
const int c_nLoops = 3;
int g_nProcess = 0;
int g_nCurProcess = 0;

static int Setup( void )
{
  {
    PDmtTree ptrTree;
    PDmtNode ptrNode;
    char s[15]; sprintf( s, "-%d", c_nLoops * c_nProcesses * c_nThreads );
    
    if ( !DmtTreeFactory::Initialize() ||
      DmtTreeFactory::GetTree( "localhost", ptrTree ) ||
      ptrTree->GetNode( c_szURI, ptrNode ) ||
      ptrNode->GetStringValue( g_strValue ) ||
      ptrNode->SetStringValue( s ) 
      ) 
      return 0;
  }

  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  DmtEventSubscription oEvent;

  UINT8 event =  SYNCML_DM_EVENT_ADD | SYNCML_DM_EVENT_REPLACE |
                        SYNCML_DM_EVENT_DELETE | SYNCML_DM_EVENT_INDIRECT;


  oEvent.Set(event,SYNCML_DM_EVENT_DETAIL);
           
  DmtTreeFactory::SubscribeEvent("./DevDetail", oEvent);
   
  DmtTreeFactory::Uninitialize();
  return 1;
}

static int Restore( void )
{
  {
    PDmtTree ptrTree;
    PDmtNode ptrNode;
    DMString str;
    
    if ( !DmtTreeFactory::Initialize() ||
      DmtTreeFactory::GetTree( "localhost", ptrTree ) ||
      ptrTree->GetNode( c_szURI, ptrNode ) ||
      ptrNode->GetStringValue( str )  ||
      ptrNode->SetStringValue( g_strValue ) 
      ) 
      return 0;

    if ( str != "0" )
      printf( "Doesnot match expected value!!! %s instead of 0; (%d)\n\n", str.c_str(), c_nLoops * c_nProcesses * c_nThreads);
    else
      printf("Done with no errors\n");
  }

  DmtTreeFactory::Uninitialize();
  return 1;
}

static void PlayLoop( int n )
{
  n = 1;
  for ( int i = 0; i < c_nLoops; i++ )
  {
    PDmtTree ptrTree;
    PDmtNode ptrNode;
    DMString s;
    
    if ( DmtTreeFactory::GetTree( "localhost", ptrTree ) ||
      ptrTree->GetNode( c_szURI, ptrNode ) ||
      ptrNode->GetStringValue( s )  ) {
      printf( "operation failed!\n");
      exit (1);
    }

    ptrTree = NULL; ptrNode = NULL;
    //usleep( 10000 );
    sleep(1);

    DMString s2;
    if ( DmtTreeFactory::GetSubtreeEx( "localhost", NULL, SYNCML_DM_LOCK_TYPE_EXCLUSIVE, ptrTree ) ||
      ptrTree->GetNode( c_szURI, ptrNode ) ||
      ptrNode->GetStringValue( s2 )  ) {
      printf( "operation failed!\n");
      exit (1);
    }

    int nV = atoi( s2 );

    char sz[10]; sprintf( sz, "%d", nV + n );
    if ( ptrNode->SetStringValue( sz )  ) {
      printf( "operation failed!\n");
      exit (1);
    }

    //if ( s != s2 )
      //printf( "%d-%d: values are not the same: %s,%s (%d) {%s}\n", g_nCurProcess, (int)getpid(), s.c_str(), s2.c_str(), n, sz );

    //usleep( 10000 );
sleep(2);
  }
}

static void* ThreadProc( void* p )
{
  int n = (int )p;

  if ( n% 2 )
  {
    //PlayLoop( n );
    PlayLoop( -n );
  }
  else
  {
    //PlayLoop( -n );
    PlayLoop( n );
  }
  
  return NULL;
}

static int RunTests( void )
{
  if ( !DmtTreeFactory::Initialize() ){
    printf("failed to init tree\n");
    return 1;
  }
  
  pthread_t ids[ c_nThreads ];
  
  for ( int i = 0; i < c_nThreads; i++ )
  {
      int nRes = pthread_create( &ids[i], 0, ThreadProc, (void*)i );

      if ( nRes ){
        printf( "Failed to create thread %d!\n", nRes);
        return 1;
      }
  }

  for ( int i = 0; i < c_nThreads; i++ )
  {
    pthread_join( ids[i], NULL );
  }

  DmtTreeFactory::Uninitialize();
  return 0;
}

extern "C" int MultiProcessTest( void )
{
  if ( !Setup() )
    return 1;

  pid_t ids[ c_nProcesses ] ;

  for ( int g_nProcess = 0; g_nProcess < c_nProcesses; g_nProcess++ )
  {
    ids[g_nProcess] = fork();

    if ( ids[g_nProcess] == -1 ){
      printf( "fork failed!\n");
      return 2;
    }

    if ( !ids[g_nProcess] ){
      g_nCurProcess = g_nProcess;
      return RunTests();
    }
  }

  int nStat = -1;

  for ( int i = 0; i < c_nProcesses; i++ )
  {
    waitpid( ids[i], &nStat, 0 );

    if ( nStat != 0 )
      printf( "%d: Child returned status %d\n", i, nStat );
  }

  Restore();
  return 0;
}

