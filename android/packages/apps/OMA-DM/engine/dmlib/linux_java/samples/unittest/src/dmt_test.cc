#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include "dmt.hpp"
//#include "dm_security.h"
#include "dmVersion.h"
#include "xpl_dm_Manager.h"

extern "C" int MultiProcessTest( void );

using namespace std;
static const char* c_szErrorText = "\033[31mError!\033[0m";

static DmtPrincipal principal("localhost");
static PDmtTree ptrTree;
static PDmtErrorDescription e;
static DMString s_strRootPath = "";

static DMString s_strCurLine;
static bool s_bDone = false;
static bool s_bAtomic = false;
static bool s_bShowTimestamp = false;
static bool s_bAutoReleaseTree = true;
static bool s_bShowProfileTime = false;
static int counter = 0;
static bool countTestCase = false;

//helper class for profiling
struct TestProfile
{
  TestProfile( const char* sHeader )
  {
    gettimeofday( &m_tvStart, NULL );
    m_szHeader = sHeader;
    
  }
  ~TestProfile()
  {
    struct timeval  tv2;
    gettimeofday( &tv2, NULL );
    long long n1 = m_tvStart.tv_usec + (m_tvStart.tv_sec * 1000000 );
    long long n2 = tv2.tv_usec + (tv2.tv_sec * 1000000 );
    long long elapsed = n2 - n1;

    if ( s_bShowProfileTime )
      printf( "DMProfile: %s, time is %d.%d ms\n", m_szHeader.c_str(), (int)(elapsed/1000), (int)(elapsed%1000/100)  );
  }

  DMString m_szHeader;
  struct timeval  m_tvStart;
};

static void Error(char *fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    if ( e.GetErrorCode() == SYNCML_DM_SUCCESS)
      return;
    printf("%s ", c_szErrorText);
    vprintf(fmt, argp);
    printf(", DmtException: %d\n", e->GetErrorCode() );
}

static DMString GetLine()
{
    DMString s = "";
    int nC;
    while ( (nC = getchar() ) != EOF )
    {
        if ( nC == '\r' || nC == '\n') {
            return s;
        }
        char sz[2] = {nC, 0};
        s += sz;
    }

    s_bDone = true;
    return s;
}

static BOOLEAN StrToBin(DMString &binStr, INT32 *len)
{
	char *pivot_w;
	char *pivot_r;
	char digits[4];
	UINT8 vbin;
	INT32 i;

	*len = binStr.length();
	if (*len  & 1 )
	{
		printf("Error Input: Uneven string length.\n");
		return FALSE;
	}
	*len >>= 1;
	pivot_w = binStr.GetBuffer();


	pivot_r = pivot_w;
	digits[3] = '\0';
	for (i=0; i<*len; i++)
	{
		strncpy(digits, pivot_r, 2);
		sscanf(digits, "%x", &vbin);
		pivot_w[i] = vbin;
		pivot_r += 2;
	}

	return TRUE;
}

static DMString ConvertEscape(DMString strInput)
{
    DMString s;
    const char* szBuf = strInput.c_str();
    char ch;
    unsigned int code;

    while (true) {
        ch = szBuf[0];
        if (ch == '\\') {
            szBuf++;
            ch = szBuf[0];
            if (ch == 't') {
                code = '\t';
            } else if (ch == 'r') {
                code = '\r';
            } else if (ch == 'n') {
                code = '\n';
            } else if (ch == 'f') {
                code = '\f';
            } else if (ch == 'x') {
                code = 0;
                for (int i = 0; i < 2; i++) {
                    szBuf++;
                    ch = szBuf[0];
                    switch (ch) {
                        case '0': case '1': case '2': case '3':
                        case '4': case '5': case '6': case '7':
                        case '8': case '9':
                            code = (code << 4) + (ch - '0');
                            break;
                        case 'a': case 'b': case 'c': case 'd':
                        case 'e': case 'f':
                            code = (code << 4) + (ch - 'a' + 10);
                            break;
                        case 'A': case 'B': case 'C': case 'D':
                        case 'E': case 'F':
                            code = (code << 4) + (ch - 'A' + 10);
                            break;
                        default: 
                            printf("%s malformed \\xXX encoding.\n", c_szErrorText);
                            return "";
                    }
                }
            } else {
                code = ch;
            }
        } else {
            code = ch;
        }

        if (code <= 0) {
            break;
        } else if (code > 0 && code <= 0xFF) {
            char sz[2] = { (char)code, 0 };
            s += sz;
        } else {
            printf("%s in input string: %x.\n", c_szErrorText, code);
            return "";
        }
        szBuf++;
    }
    return s;
}

static DMString GetParam()
{
  DMString s;

  const char* szBuf = s_strCurLine.c_str();

  while ( szBuf[0] == ' ' )
    szBuf++;

  bool bEscaped = false;
  char ch;
  while ( true ) {
    ch = szBuf[0];
    if ( ch == 0 || (!bEscaped && ch == ' ')) {
        break;
    }
    if ( !bEscaped && ch == '\\') {
        bEscaped = true;
    } else {
        bEscaped = false;
    }
    
    char sz[2] = {ch, 0};
    s += sz;
    szBuf++;
  }

  DMString strTmp = szBuf;
  s_strCurLine = strTmp;

  return ConvertEscape(s);
}

static DMString GetCmd()
{
    if (s_bDone) {
        return "quit";
    }

    if (countTestCase) {
      counter++;
    }

    s_strCurLine = GetLine();
    return GetParam();
}

static void startCounter()
{
  printf("Counter started!\n");
  countTestCase = true;
}

static void endCounter()
{
  if (counter > 0)
  {
    counter--;
  }
  printf("Counter ended!\n");
  countTestCase = false;
}

static void displayCountStatus()
{
  if (counter > 0)
  {
    counter--;
  }
  printf("Count is ");
  if (countTestCase) {
    printf("ON\n");
  } else {
    printf("OFF\n");
  }
}

static void resetCounter()
{
  printf("Counter has reseted!\n");
  counter = 0;
}

static void getCounter()
{
  if (counter > 0)
  {
    counter--;
  }
  printf("Number of tested cases: %d\n", counter);
  return;
}


static DMString GetInputString(const char * szPrompt, const char * szDefault)
{
    printf("%s (default=%s): ", szPrompt, szDefault);
    DMString strIn = GetLine();
    printf("get: %s\n", strIn.c_str());
    if (strIn == "") {
        strIn = szDefault;
    }
    return ConvertEscape(strIn);
}


/**
 * Get DM tree to global tree variable (ptrTree)
 */
static PDmtTree GetTree()
{
  if (ptrTree != NULL) return ptrTree;
  if ( (e=DmtTreeFactory::GetSubtree(principal, s_strRootPath.c_str(), ptrTree)) != NULL ) {
      Error("can't get tree");
  }
  return ptrTree;
}

static  PDmtNode GetNode( const char*  szNodeName )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree != NULL ){
    if ( (e=ptrTree->GetNode( szNodeName, ptrNode)) != NULL ) {
        Error("can't get node %s", szNodeName);

    }
  }
  return ptrNode;
}
static void printESNChunk(const char * buffer, int size )
{
  int offset=0;
  int line =0;
  while(offset < size)
  {
  	putchar(buffer[offset++]);
	line++;
	if(line>64)
	{ line =0;
	   printf( "\n");
	}
  }
   printf( "\n");
}

static void displayESN(PDmtNode ptrNode)
{
   SYNCML_DM_RET_STATUS_T retStatus;
   if (ptrNode->IsExternalStorageNode()) 
   {  
	   DmtDataChunk chunkData;	   
	   UINT32 getLen;
	   UINT8 *bufp;
	   retStatus = ptrNode->GetFirstChunk(chunkData);  
	   if( retStatus != SYNCML_DM_SUCCESS)
		   return;
	   chunkData.GetReturnLen(getLen);	   
	   chunkData.GetChunkData(&bufp);  // the chunk data is available  
	   while (true) 
	   {
		   printESNChunk((const char*)bufp, getLen);
		   if (getLen == 0)
				   break;
		   else
		   {
			   //  save or process the data in *bufp				   
			   retStatus = ptrNode->GetNextChunk(chunkData);
			   if( retStatus != SYNCML_DM_SUCCESS)
				   return;
			   chunkData.GetReturnLen(getLen); 
			   chunkData.GetChunkData(&bufp);
		   }
			}										   
	
	} else {
	   DMString path;
	   ptrNode->GetPath(path);

	   Error("It's not a External Storage Node \n", path.c_str());
    }
}

static void PrintNode( PDmtNode ptrNode )
{
    DmtAttributes oAttr;
    DMString path;

    if( (e=ptrNode->GetPath(path)) != NULL )
    {
      Error("can't get attributes of node %d",  e.GetErrorCode());
    }
    
    if ( (e=ptrNode->GetAttributes( oAttr )) != NULL) {
      Error("can't get attributes of node %s",  path.c_str());
      return;
    }

    DmtData oData;
    if (!ptrNode->IsExternalStorageNode()) 
    {
    	if ( (e=ptrNode->GetValue( oData )) != NULL ) {
	        Error("can't get value of node %s", path.c_str());
      	  return;
    	}
    }
    printf("path=%s\n", (const char*)path.c_str());
    printf("isLeaf=%s\n", (ptrNode->IsLeaf()?"true":"false") );
    printf("name=%s\n", (const char*)oAttr.GetName().c_str() );
    printf("format=%s\n", (const char*)oAttr.GetFormat().c_str() );
    printf("type=%s\n", (const char*)oAttr.GetType().c_str() );
    printf("title=%s\n", (const char*)oAttr.GetTitle().c_str() );
    printf("acl=%s\n", (const char*)oAttr.GetAcl().toString().c_str() );
    printf("size=%d\n", (const char*)oAttr.GetSize() );
    if ( s_bShowTimestamp ) {
        if ( oAttr.GetTimestamp() == 0 ) {
            printf("timestamp=(Unknown)\n");
        } else {
            // convert msec to sec
            time_t timestamp = (time_t)(oAttr.GetTimestamp()/1000L);
            printf("timestamp=%s", ctime(&timestamp) );
        }
    }
    printf("version=%d\n", oAttr.GetVersion() );
    if ( !ptrNode->IsLeaf() ) {
      DMStringVector aChildren;
      oData.GetNodeValue( aChildren );
      printf("children:");
      if ( aChildren.size() == 0 ) {
          printf("null");
      }
      for (int i=0; i < aChildren.size(); i++) {
          printf("%s/", aChildren[i].c_str());
      }
      printf("\n");
    } else {
      if (ptrNode->IsExternalStorageNode()) 
        {
          printf("value=\n");
          displayESN(ptrNode);
        }
      else {
        if ( strcasecmp(oAttr.GetFormat(), "bin") == 0 ) {
          printf("Binary value: [");
          for ( int i = 0 ; i < oData.GetBinaryValue().size(); i++ ){
            printf( "%02x ", oData.GetBinaryValue().get_data()[i]);
          }
          printf( "]\n" );
        }
	else
	{
		DMString s;
		oData.GetString(s);
		printf("value=%s\n", s.c_str());
	}

      
    }
}
}

static void Get( const char* szNodeName )
{
  PDmtNode ptrNode = GetNode( szNodeName );

  if ( ptrNode == NULL )
    return;

  PrintNode(ptrNode);
}

static void CreateInterior( const char * szNode )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  if ( (e=ptrTree->CreateInteriorNode( szNode, ptrNode )) == NULL ) {
    printf( "node %s created successfully\n", szNode );
  } else {
    Error("can't create a node %s", szNode);
  }
}

static void CreateLeaf( const char * szNode, const char* szData )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  if ( (e=ptrTree->CreateLeafNode( szNode, ptrNode, DmtData( szData ) )) == NULL ) {
    printf( "node %s (%s) created successfully\n", szNode, szData );
  } else {
    Error("can't create a node %s", szNode);
  }
}

static void CreateLeafInteger( const char * szNode, const char* szData )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  int intValue = atoi(szData);

  if ( (e=ptrTree->CreateLeafNode( szNode, ptrNode, DmtData( intValue ) )) == NULL ) {
    printf( "node %s (%d) created successfully\n", szNode, intValue );
  } else {
    Error("can't create a node %s", szNode);
  }
}

static void CreateLeafBoolean( const char * szNode, const char* szData )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  bool bValue = ((strcasecmp(szData, "true") == 0) ? true : false);

  if ( (e=ptrTree->CreateLeafNode( szNode, ptrNode, DmtData( bValue ) )) == NULL ) {
    printf( "node %s (%s) created successfully\n", 
                szNode, (bValue?"true":"false") );
  } else {
    Error("can't create a node %s", szNode);
  }
}



static void CreateLeafBinary( const char * szNode, const char* szData )
{
	INT32 len;
	DMString binStr = szData;
	
	if ( StrToBin( binStr, &len) == FALSE)
		return;

  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  if ( (e=ptrTree->CreateLeafNode( szNode, ptrNode, DmtData( (const byte*)binStr.c_str(), len ) )) == NULL ) {
    printf( "node %s (%s) created successfully\n", szNode, szData );
  } else {
    Error("can't create a node %s", szNode);
  }
}

static void CreateLeafBinaryE( const char * szNode )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  if ( (e=ptrTree->CreateLeafNode( szNode, ptrNode, DmtData( ) )) == NULL ) {
    printf( "node %s () created successfully\n", szNode );
  } else {
    Error("can't create a node %s", szNode);
  }
}


static void CreateLeafFloat( const char * szNode, const char* szData )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  if ( (e=ptrTree->CreateLeafNode( szNode, ptrNode, DmtData( szData, SYNCML_DM_DATAFORMAT_FLOAT) ) )  == NULL ) {
    printf( "node %s (%s) created successfully\n", szNode, szData );
  } else {
    Error("can't create a node %s", szNode);
  }
}

static void CreateLeafDate( const char * szNode, const char* szData )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  if ( (e=ptrTree->CreateLeafNode( szNode, ptrNode, DmtData( szData, SYNCML_DM_DATAFORMAT_DATE) ) )  == NULL ) {
    printf( "node %s (%s) created successfully\n", szNode, szData );
  } else {
    Error("can't create a node %s", szNode);
  }
}

static void CreateLeafTime( const char * szNode, const char* szData )
{
  PDmtNode ptrNode;
  GetTree();

  if ( ptrTree == NULL ) {
    return;
  }

  if ( (e=ptrTree->CreateLeafNode( szNode, ptrNode, DmtData( szData, SYNCML_DM_DATAFORMAT_TIME) ) )  == NULL ) {
    printf( "node %s (%s) created successfully\n", szNode, szData );
  } else {
    Error("can't create a node %s", szNode);
  }
}

static void Delete(const char* szNode )
{
  GetTree();
  if ( ptrTree == NULL ) {
    return;
  }
  
  if ( (e=ptrTree->DeleteNode( szNode )) == NULL ) {
    printf( "node %s deleted successfully\n", szNode );
  } else {
    Error("can't delete node %s", szNode);
  }
}

static void Rename(const char* szNode, const char* szNewName)
{
  GetTree();
  if ( ptrTree == NULL ) {
      return;
  }

  if ( (e=ptrTree->RenameNode( szNode, szNewName )) == NULL ) {
    printf( "node %s renamed to %s successfully\n", szNode, szNewName);
  } else {
    Error("can't rename node %s to %s", szNode, szNewName);
  }
}

static void setESN(const char * szNode, const char * szFile)
{
  FILE* f = fopen( szFile, "r" );
  if ( !f ) {
    printf("%s can't open file %s\n",c_szErrorText, szFile);
    return;
  }

  // assume 100k is enough
  const int c_nSize = 100 * 1024;
  char* szBuf = new char [c_nSize];

  int n = fread(szBuf, 1, c_nSize, f );

  printf("read %d bytes\n", n);
  if ( n > 0 ) {
   SYNCML_DM_RET_STATUS_T retStatus;
  PDmtNode ptrNode = GetNode(szNode);
  if ( ptrNode == NULL ) {
    return;
  }

    szBuf[n] = 0;

 if (ptrNode->IsExternalStorageNode()) 
 { 	
	DmtDataChunk chunkData;		
	int setLen = 0;
	int offset = 0;
	bool isFirstChunk = true;
	bool isLastChunk = false;
	int chunksize = chunkData.GetChunkSize();

	while(!isLastChunk)
	{	setLen =  n - offset;
		if(setLen > 0)	
		{
			if(setLen > chunksize)
				setLen = chunksize;
		}
		else
			isLastChunk = true;

		printESNChunk(&szBuf[offset], setLen);
		chunkData.SetChunkData((const UINT8 *)&szBuf[offset], setLen);
		if(isFirstChunk)
		{	
			retStatus = ptrNode->SetFirstChunk(chunkData);	
			isFirstChunk = false;
		}
		else
		{	if(!isLastChunk)
				retStatus = ptrNode->SetNextChunk(chunkData);	
			else
				retStatus = ptrNode->SetLastChunk(chunkData);	
		}

		offset += setLen;
	}

  } else {
    Error("It's not a External Storage Node \n", szNode);
  }
  }

  delete [] szBuf;
  fclose( f );
}
static void SetTitle(const char * szNode, const char * szTitle)
{
  PDmtNode ptrNode = GetNode(szNode);
  if ( ptrNode == NULL ) {
    return;
  }

  if ( (e=ptrNode->SetTitle(szTitle)) == NULL ) {
    printf("set title of node %s to %s successfully\n",
             szNode, szTitle);
    PrintNode(ptrNode);
  } else {
    Error("can't set title of node %s to %s", szNode, szTitle);
  }
}

static void SetAcl(const char * szNode, const char * szAcl)
{
  PDmtNode ptrNode = GetNode(szNode);
  if (ptrNode == NULL) {
      return;
  }

  DmtAcl oAcl(szAcl);
  if ( (e=ptrNode->SetAcl(oAcl)) == NULL ) {
    printf("set acl of node %s to %s successfully\n",
             szNode, szAcl);
    PrintNode(ptrNode);
  } else {
    Error("can't set acl of node %s to %s", szNode, szAcl);
  }
}

static void ReplaceString( const char * szNode, const char * szValue)
{
    PDmtNode ptrNode = GetNode(szNode);
    if (ptrNode == NULL) {
        return;
    }
            
    if ( (e=ptrNode->SetStringValue(szValue)) == NULL ) {
        printf("set value of node %s to %s successfully\n", szNode, szValue);
        PrintNode(ptrNode);
    } else {
        Error("can't set value of node %s to %s", szNode, szValue);
    }
}

static void ReplaceInteger( const char * szNode, const char * szValue)
{
    PDmtNode ptrNode = GetNode(szNode);
    if (ptrNode == NULL) {
        return;
    }
    //int intValue = atoi(szValue);
    int intValue = 0;
    //store the sscanf result;
    int int_item_converted = 0;
    int_item_converted = sscanf(szValue, "%d", &intValue);
    //printf("data is %s \n", szValue);
    printf("INT_ITEM_CONVERTED is %d \n", int_item_converted);
    if(int_item_converted==0) {
        printf("The input %s is not an integer\n", szValue);
	 return;
     }
    if ( (e=ptrNode->SetIntValue( intValue )) == NULL ) {
        printf("set value of node %s to %d successfully\n", szNode, intValue);
        PrintNode(ptrNode);
    } else {
        Error("can't set value of node %s to %d", szNode, intValue);
    }
}

static void ReplaceBoolean( const char * szNode, const char * szValue)
{
    PDmtNode ptrNode = GetNode(szNode);
    if (ptrNode == NULL) {
        return;
    }
    bool boolValue = ((strcasecmp(szValue, "true") == 0) ? true : false);
    if ( (e=ptrNode->SetBooleanValue( boolValue )) == NULL ) {
        printf("set value of node %s to %s successfully\n", 
                szNode, (boolValue?"true":"false"));
        PrintNode(ptrNode);
    } else {
        Error("can't set value of node %s to %s", 
                szNode, (boolValue?"true":"false"));
    }
}

static void ReplaceFloat( const char * szNode, const char * szValue)
{
    PDmtNode ptrNode = GetNode(szNode);
    if (ptrNode == NULL) {
        return;
    }

    if ( (e=ptrNode->SetFloatValue( szValue )) == NULL ) {
        printf("set value of node %s to %s successfully\n", szNode, szValue);
        PrintNode(ptrNode);
    } else {
        Error("can't set value of node %s to %s", szNode, szValue);
    }
}

static void ReplaceDate( const char * szNode, const char * szValue)
{
    PDmtNode ptrNode = GetNode(szNode);
    if (ptrNode == NULL) {
        return;
    }
    if ( (e=ptrNode->SetDateValue( szValue )) == NULL ) {
        printf("set value of node %s to %s successfully\n", szNode, szValue);
        PrintNode(ptrNode);
    } else {
        Error("can't set value of node %s to %s", szNode, szValue);
    }
}

static void ReplaceTime( const char * szNode, const char * szValue)
{
    PDmtNode ptrNode = GetNode(szNode);
    if (ptrNode == NULL) {
        return;
    }

    if ( (e=ptrNode->SetTimeValue( szValue )) == NULL ) {
        printf("set value of node %s to %s successfully\n", szNode, szValue);
        PrintNode(ptrNode);
    } else {
        Error("can't set value of node %s to %s", szNode, szValue);
    }
}

static void ReplaceBytes( const char * szNode, const char * szValue)
{
	INT32 len;
	DMString binStr = szValue;
	
	if ( StrToBin(binStr, &len) == FALSE)
		return;

    PDmtNode ptrNode = GetNode(szNode);
    if (ptrNode == NULL) {
        return;
    }

    if ( (e=ptrNode->SetBinaryValue( (const byte*)binStr.c_str(), len )) == NULL ) {
        printf("set value of node %s to %s successfully\n", szNode, szValue);
        PrintNode(ptrNode);
    } else {
        Error("can't set value of node %s to %s", szNode, szValue);
    }
}

static void DumpSubTree( PDmtNode ptrNode )
{
    PrintNode(ptrNode);
    printf("\n");
    if ( e != NULL ) return;

    if ( !ptrNode->IsLeaf() ) {
      DMVector<PDmtNode> aChildren;
      if ( (e=ptrNode->GetChildNodes( aChildren )) != NULL ) {
        DMString path;
        ptrNode->GetPath(path);
        Error("can't get child nodes of %s", path.c_str());
        return;
      }
      for (int i=0; i < aChildren.size(); i++) {
          DumpSubTree( aChildren[i] );
      }
    }
}

static void DumpTree( const char * szNode )
{
    PDmtNode ptrNode = GetNode( szNode );
    if ( ptrNode == NULL  ) {
        return;
    }

    DumpSubTree( ptrNode );
}

static void Execute( const char * szNode, const char * szData )
{
    PDmtNode ptrNode = GetNode(szNode);
    if (ptrNode == NULL) {
        return;
    }
    DMString strResult;
    if ( (e=ptrNode->Execute(szData, strResult)) == NULL ) {
        printf("execute node %s successfully, result=%s\n",
                szNode, strResult.c_str() );
    } else {
        Error("can't execute node %s", szNode);
    }
}

static void Open( const char * szNode)
{
    if ( strcmp(szNode, ".") == 0) {
        s_strRootPath = "";
    } else {
        s_strRootPath = szNode;
    }
    s_bAtomic = false;
    ptrTree = NULL;
    printf("Open tree: %s\n", s_strRootPath.c_str());
}

static void GetExclusiveTree( const char * szNode)
{
    if ( strcmp(szNode, ".") == 0) {
        s_strRootPath = "";
    } else {
        s_strRootPath = szNode;
    }
    s_bAtomic = false;
    ptrTree = NULL;

  if ( (e=DmtTreeFactory::GetSubtreeEx(principal, s_strRootPath.c_str(), DmtTreeFactory::LOCK_TYPE_EXCLUSIVE, ptrTree)) != NULL ) {
      Error("can't get tree");
  }
    
    printf("GetExclusiveTree: %s\n", s_strRootPath.c_str());
}

static void Release()
{
    s_bAtomic = false;
    ptrTree = NULL;
    s_strRootPath = "";
    printf("release tree successfully\n");
}

static void Flush()
{
    if (ptrTree != NULL) {
        if ((e=ptrTree->Flush()) == NULL) {
            printf("flush tree successfully\n");
        } else {
            Error("can't flush tree");
        }
    } else {
        printf("tree is already released\n");
    }
    s_bAtomic = false;
}

static void Begin()
{
    GetTree();
    if (ptrTree == NULL) {
        return;
    }
    if ( (e=ptrTree->Begin()) == NULL ) {
        s_bAtomic = true;
        printf("begin an atomic operation successfully\n");
    } else {
        Error("can't begin an atomic operation");
    }
}

static void Commit()
{
    if (!s_bAtomic || ptrTree == NULL) {
        printf("not in the middle of atomic operations\n");
    } else if ( (e=ptrTree->Commit()) == NULL) {
        printf("commit atomic operations successfully\n");
    } else {
        Error("can't commit atomic operations");
    }
    s_bAtomic = false;
}

static void Rollback()
{
    if ( !s_bAtomic || ptrTree == NULL) {
        printf("not in the middle of the atomic operations\n");
    } else if ( (e=ptrTree->Rollback()) == NULL) {
        printf("rollback atomic operations successfully\n");
    } else {
        Error("can't rollback atomic oeprations");
    }
    s_bAtomic = false;
}

static void Clone(const char * szNode, const char * szNewNode)
{
    GetTree();
    if (ptrTree == NULL) {
        return;
    }

    if ( (e=ptrTree->Clone(szNode, szNewNode)) == NULL) {
        printf("clone %s to %s successfully\n", szNode, szNewNode);
    } else {
        Error("can't clone %s to %s", szNode, szNewNode);
    }
}

static void GetMap( const char * szNode)
{
    GetTree();
    if (ptrTree == NULL) {
        return;
    }

    DMMap<DMString, DmtData> oMap;
    if ( (e=ptrTree->GetChildValuesMap(szNode, oMap)) == NULL) {
        printf("map table size=%d\n", oMap.size());
        for (DMMap<DMString, DmtData>::POS it = oMap.begin(); it != oMap.end(); it++) {
            DMString strKey = oMap.get_key(it);
            DmtData oData = oMap.get_value(it);
            DMString strData;
            if ( (e=oData.GetString(strData)) == NULL) {
                printf("%s=%s\n", strKey.c_str(), strData.c_str());
            } else {
                Error("can't get value of node %s", strKey.c_str() );
            }
        }
    } else {
        Error("can't get map of node %s", szNode);
    }
}

static void SetMap( const char * szNode )
{
    GetTree();
    if (ptrTree == NULL) {
        return;
    }

    DMMap<DMString, DmtData> oMap;
    while (true) {
        DMString strKey = GetParam();
        DMString strData = GetParam();

        if (strKey == "" || strData == "") {
            break;
        }
        DmtData oData(strData);
        oMap.put(strKey, oData);
    }
    if ( (e=ptrTree->SetChildValuesMap(szNode, oMap)) == NULL ) {
        printf("set map for %s succesfully, size = %d\n",
                szNode, oMap.size());
    } else {
        Error("can't set map of node %s", szNode);
    }
}


static void Dump( const char* buf, int size )
{
  int nOffset = 0;
 
  while ( size > 0){
    int nLine = size > 16 ? 16: size;
 
    char s[250];
    int pos = 0;
 
    pos += sprintf( s+pos, "%04x:", nOffset );
 
    for ( int i = 0; i < nLine; i++ ){
      pos += sprintf( s+pos, " %02x", (unsigned int)((unsigned char)buf[i]) );
    }
    for ( int i = nLine; i < 16; i++ ){
      pos += sprintf( s+pos, "   " );
    }
 
    pos += sprintf( s+pos, "  " );
    for ( int i = 0; i < nLine; i++ ){
      pos += sprintf( s+pos, "%c", (buf[i] > 31 ? buf[i] : '.') );
    }
 
    printf( "%s\n", s );
    buf += nLine;
    size -= nLine;
    nOffset += nLine;
  }
}

static void ProcessScript( const char* szFile, const char* isBinary )
{
  FILE* f = fopen( szFile, "r" );
  if ( !f ) {
    printf("%s can't open file %s\n",c_szErrorText, szFile);
    return;
  }

  bool bBinary = (isBinary[0] == '1');
  
  printf("Process WBXML script: %s\n", bBinary? "true" : "false");
  
  // assume 100k is enough
  const int c_nSize = 100 * 1024;
  char* szBuf = new char [c_nSize];

  int n = fread(szBuf, 1, c_nSize, f );


  printf("Read %d bytes\n", n);
  if ( n > 0 ) {
    szBuf[n] = 0;
    
    DMString oResult;
    DMVector<UINT8> bResult; 
    DmtPrincipal p("localhost");
    SYNCML_DM_RET_STATUS_T res = DmtTreeFactory::ProcessScript( p, (const byte*)szBuf, n, bBinary, bResult);

    if (bBinary == true) 
    {
      printf( "\nPrint result in HEX mode: \n\n" );
      //  Dump(oResult.c_str(), oResult.length());

      printf( "Result Size: %d\n", bResult.size());
      
      int resultSize = bResult.size();
      char* resultBuf = new char [resultSize];

      for (int i = 0; i < bResult.size(); i++) {
        resultBuf[i] = (char)bResult[i];
      }

      Dump(resultBuf, resultSize);

    } else 
    {
      
      int resultSize = bResult.size();
      char* resultBuf = new char [resultSize+1];

      for (int i = 0; i < bResult.size(); i++) {
        resultBuf[i] = (char)bResult[i];
      }
      resultBuf[resultSize] = '\0';

      printf( "The test Script error code %d; text:\n\n%s\n\n",
	      (int)res, 
	      (const char*)resultBuf );
    }
  }

  delete [] szBuf;
  fclose( f );
}

static void StartServerSession( const char* szServerID, const char* szParam2 )
{
    bool bBinary = (szParam2[0] == '1');

    printf("start server session, %s, bin = %s\n", 
                szServerID, bBinary? "true" : "false");
	e = DmtTreeFactory::StartServerSession( szServerID, bBinary );
	
	if ( e != NULL ) {
      Error("can't start server session");
    }
}

static void StartServerSessionEx( const char* szServerID, const char* szBin, const char* szCorrelator1, const char* szCorrelator2 )
{
    bool bBinary = (szBin[0] == '1');

    printf("start server session, %s, bin = %s, correlator(s) %s, %s\n", 
                szServerID, bBinary? "true" : "false",
               szCorrelator1, szCorrelator2 );

    DmtSessionProp oProps( DmtFirmAlert( "./DevDetail", "200", 
      "org.openmobilealliance.firmwareupdate.download", "int",
      "critical", szCorrelator1 ), bBinary );

    if ( szCorrelator2 && *szCorrelator2 ){
      oProps.addFirmAlert(DmtFirmAlert("./DevInfo", "402", 
      NULL, NULL, NULL, szCorrelator2) );
    
      oProps.addFirmAlert(DmtFirmAlert("./DevInfo/somewhere", NULL, 
        NULL, NULL, "NULL", NULL) );
      
      oProps.addFirmAlert(DmtFirmAlert("./DevInfo/somewhere", "data", 
        NULL, NULL, "NULL", NULL) );
    }
    
	e = DmtTreeFactory::StartServerSession( szServerID, oProps );
	
	if ( e.GetErrorCode() != SYNCML_DM_SUCCESS ) {
      Error("can't start server session");
    }
}


//typedef SYNCMLDM_SEC_CREDENTIALS_T * (* hmac_callback)
//       (const SYNCMLDM_HMAC_SEC_INFO_T *ps_hmac_sec_info);
           
static void BuildCredHMAC(const char * server_Id)
{
  /***
    SYNCMLDM_SEC_CREDENTIALS_T    *pHmacCreds = NULL;
    SYNCMLDM_HMAC_SEC_INFO_T       hmacSecInfo;

    printf("Build the credentials for HMAC authentication scheme.\n");

   void *dllhandle=dlopen("libdmssession.so", RTLD_NOW);
   if (dllhandle==NULL)
   {
      dlclose( dllhandle );
      return;
   }
    hmacSecInfo.pb_syncml_document = NULL;
    hmac_callback pFunc=(hmac_callback)dlsym(dllhandle, "syncmldm_sec_build_hmac_cred"); 
    pHmacCreds = pFunc((const SYNCMLDM_HMAC_SEC_INFO_T *)&hmacSecInfo);
   if ( dllhandle != NULL)
	 dlclose(dllhandle);
    printf("credhmac end.\n");
    */
}


static DMString CreateServerId(const char * szServerIP)
{
    DMString strServerID = "";
    DMString strPath = "./SyncML/DMAcc/";
    DMString strPath2;
    DMString strInput;
    PDmtNode ptrNode;

    if (s_bAtomic) {
        s_bAtomic = false;
        ptrTree = NULL;
    }

    if ( (e=DmtTreeFactory::GetTree(principal, ptrTree)) != NULL) {
        Error("can't get tree");
        return "";
    } 

 //   if ( (e=ptrTree->Begin()) != NULL) {
//       Error("can't begin atomic operations");
//        ptrTree = NULL;
//        return "";
 //   }
    
//    strInput = GetInputString("Enter account/profile name:", "test");
    strPath += "SampleServer";
    
    if ( ptrTree->IsValidNode( strPath.c_str() ) ) {
        if ( (e=ptrTree->DeleteNode(strPath.c_str())) != NULL) {
            Error("can't delete existing node %s", strPath.c_str());
 //           ptrTree->Rollback();
            ptrTree = NULL;
            return "";
        }
    }

    // Interior node
    if ( (e=ptrTree->CreateInteriorNode(strPath.c_str(), ptrNode)) != NULL) {
        Error("can't create node %s", strPath.c_str());
   //     ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }

    // PortNbr
    //strInput = GetInputString("Enter port no:", "80");
    strPath2 = strPath;
    strPath2 += "/PortNbr";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("80")) ) != NULL) {
        Error("can't create node %s", strPath2.c_str() );
   //     ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }

    // AddrType
    strPath2 = strPath;
    strPath2 += "/Name";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("SampleServer")) ) == NULL) {
        printf("Stored name=SampleServer\n");
    } else {
        Error("can't create node %s", strPath2.c_str());
     //   ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }
    
    // AddrType
    strPath2 = strPath;
    strPath2 += "/AddrType";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("1")) ) == NULL) {
        printf("Stored AddrType=1\n");
    } else {
        Error("can't create node %s", strPath2.c_str());
     //   ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }
    
    // Address
    //DMString strAddr = "http://";
    //strAddr += szServerIP;
    //strAddr += "/Manage";

    //strInput = GetInputString("Enter Addr:", strAddr.c_str());
    strPath2 = strPath;
    strPath2 += "/Addr";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("http://10.72.34.36/Manage")) ) != NULL) {
        Error("can't create node %s", strPath2.c_str());
    //    ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }

    // AuthPref
//    strInput = GetInputString("Enter AuthPref:", "syncml:auth-Basic");
    strPath2 = strPath;
    strPath2 += "/AuthPref";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("syncml:auth-Basic")) ) != NULL) {
        Error("can't create node %s", strPath2.c_str());
    //    ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }

    // User name
/////    strInput = GetInputString("Enter UserName:", "SampleServer");
    strPath2 = strPath;
    strPath2 += "/UserName";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData((CPCHAR)"аезклл")) ) != NULL) {
        Error("can't create node %s", strPath2.c_str());
   //     ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }

    // Client Password
 //   strInput = GetInputString("Enter ClientPW:", "SampleServer");
    strPath2 = strPath;
    strPath2 += "/ClientPW";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData((CPCHAR)"липомм")) ) != NULL) {
        Error("can't create node %s", strPath2.c_str());
    //    ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }
    
    // Client Nonce
//    strInput = GetInputString("Enter ClientNonce:", "MTIzNDU=");
    strPath2 = strPath;
    strPath2 += "/ClientNonce";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("123abc")) ) != NULL) {
        Error("can't create node %s", strPath2.c_str());
   //     ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }
    
    // ServerID
//    strInput = GetInputString("Enter ServerId:", "sid");
    strPath2 = strPath;
    strPath2 += "/ServerId";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("Scts")) ) == NULL) {
        strServerID = strInput;
    } else {
        Error("can't create node %s", strPath2.c_str());
   //     ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }

    // Server Password
   // strInput = GetInputString("Enter ServerPW:", "SampleClient");
    strPath2 = strPath;
    strPath2 += "/ServerPW";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData((CPCHAR)"ьйвдаа")) ) != NULL) {
        Error("can't create node %s", strPath2.c_str());
   //     ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }

    // Server Nonce
  //  strInput = GetInputString("Enter ServerNonce:", "MTIzNDU=");
    strPath2 = strPath;
    strPath2 += "/ServerNonce";
    if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("MTIzNDEyMzQxMjM0MTIzNA==")) ) != NULL) {
        Error("can't create node %s", strPath2.c_str());
   //     ptrTree->Rollback();
        ptrTree = NULL;
        return "";
    }
    
    // ConRef
    //strPath2 = strPath;
    //strPath2 += "/ConRef";
   // if ( (e=ptrTree->CreateLeafNode(strPath2.c_str(), ptrNode, DmtData("1")) ) == NULL) {
    //    printf("Stored ConRef=1\n");
    //} else {
    //    Error("can't create node %s", strPath2.c_str());
    //    ptrTree->Rollback();
  //      ptrTree = NULL;
   //     return "";
   // }
    
//    ptrTree->Commit();
    ptrTree = NULL;
    return strServerID;
}

static void CreateAndStartServerSession(const char * szAddr, const char * szParam2) 
{
    printf("Create Server ID\n");
    DMString strServerID = CreateServerId(szAddr);
    if (strServerID="") {
        printf("Failed to create server id\n");
    } else {
        StartServerSession(strServerID.c_str(), szParam2);
    }
}

static void ShowTimestamp(const char * sParam) 
{
    s_bShowTimestamp = (sParam[0] == '1');
    if (s_bShowTimestamp) {
        printf("now show timestamp of nodes\n");
    } else {
        printf("now don't show timestamp of nodes\n");
    }
}

static void ShowProfileTime(const char * sParam) 
{
    s_bShowProfileTime = (sParam[0] == '1');
    if (s_bShowProfileTime) {
        printf("now show profile time\n");
    } else {
        printf("now don't show profile time\n");
    }
}

static void DMVersion()
{

  unsigned long libVersion = 0;
  const char *version = GetDMSyncMLVersion(&libVersion); 
  CPCHAR dm_ver = XPL_DM_GetEnv(SYNCML_DM_VERSION);
  printf("DM Version: %s\n", dm_ver);
  printf("LIB Version = 0x%06x    \nDMSyncMLVersion = %s\n", libVersion, version);

}

static void SuperAgent()
{

  CPCHAR dm_ver = XPL_DM_GetEnv(SYNCML_DM_VERSION);
  PDmtNode ptrNode;
  if (strcmp(dm_ver, "1.1.2") == 0) { 
    CreateInterior("./SyncML/DMAcc/SuperAgent");
    CreateLeaf("./SyncML/DMAcc/SuperAgent/ServerId", "LJAgent");
    if (e != NULL) {
      return;
    }
    ptrNode = GetNode("./SyncML/DMAcc/SuperAgent/ServerId");
  } else if (strcmp(dm_ver, "1.2") == 0) {
    CreateInterior("./DMAcc/SuperAgent");
    CreateLeaf("./DMAcc/SuperAgent/ServerID", "LJAgent");
    if (e != NULL) {
      return;
    }
    ptrNode = GetNode("./DMAcc/SuperAgent/ServerID");
  } else {
    Error("DM Version not supported!\n");
    return;
  }

  if ( ptrNode == NULL ) {
    if (strcmp(dm_ver, "1.1.2") == 0) {
      Error("can't get Node ./SyncML/DMAcc/SuperAgent/ServerId");
      return;
    } else if (strcmp(dm_ver, "1.2") == 0) {
      Error("can't get Node ./DMAcc/SuperAgent/ServerID");
      return;
    }
  }


    DmtAcl oAcl( "Replace=*");
    if ( (e=ptrNode->SetAcl(oAcl)) != NULL ) 
    {
      if (strcmp(dm_ver, "1.1.2") == 0) {
        Error("can't set acl of node ./SyncML/DMAcc/SuperAgent/ServerId to Replace=*");
	return;
      } else if (strcmp(dm_ver, "1.2") == 0) {
	Error("can't set acl of node ./DMAcc/SuperAgent/ServerID to Replace=*");
	return;
      }
    }
	
	DmtData dmData;
	if( (e=ptrNode->GetValue(dmData)) == NULL )
	{
		Error("Get node value even if don't have get permission");
		return;
	}
	ptrNode = NULL;
	ptrTree = NULL;

	
	DmtPrincipal lj("LJAgent");

	PDmtTree tree;
	if ( (e=DmtTreeFactory::GetSubtree(lj, s_strRootPath.c_str(), tree)) != NULL )
	{
		Error("can't get tree for LJAgent");
		return;
	}

	if( tree != NULL )
	{
	  if (strcmp(dm_ver, "1.1.2") == 0) {
		if( (e=tree->GetNode("./SyncML/DMAcc/SuperAgent/ServerId", ptrNode)) != NULL )
		{
			Error("can't get node for LJAgent");
		}

	  } else if (strcmp(dm_ver, "1.2") == 0) {
	    if( (e=tree->GetNode("./DMAcc/SuperAgent/ServerID", ptrNode)) != NULL )
		{
			Error("can't get node for LJAgent");
		}
	  }	

	  if( ptrNode != NULL )
	  {
	    PrintNode(ptrNode);
	    if (strcmp(dm_ver, "1.1.2") == 0) {
	      if( (e=tree->DeleteNode("./SyncML/DMAcc/SuperAgent")) != NULL )
	      {
		Error("can't delete ./SyncML/DMAcc/SuperAgent by LJAgent");
	      }
	    } else if (strcmp(dm_ver, "1.2") == 0) {
	      if( (e=tree->DeleteNode("./DMAcc/SuperAgent")) != NULL )
	      {
		Error("can't delete ./DMAcc/SuperAgent by LJAgent");
	      }
	    }			
	  }
	}
}
	
                                                                             
static void SubTreeStruct(PDmtNode ptrNode, int depth)
{
   for (int i=0; i < depth; i++) {
       if (i == (depth-1)) {
           printf("^---");
       } else {
           printf("    ");
       }
   }
   
   DMString strName;
   ptrNode->GetNodeName(strName);
   if (ptrNode->IsLeaf() ) {
       DMString strValue;
       DMString strName;
       ptrNode->GetStringValue(strValue);
       printf("%s: %s\n", strName.c_str(), strValue.c_str());
   } else {
       printf("%s(node)\n", strName.c_str());
       DMVector<PDmtNode> aChildren;
       if ( (e=ptrNode->GetChildNodes( aChildren )) != NULL ) {
            DMString strPath;
            ptrNode->GetPath(strPath);
            Error("can't get child nodes of %s\n", strPath.c_str());
           return;
       }
       for (int i=0; i < aChildren.size(); i++) {
           SubTreeStruct(aChildren[i], depth+1);
       }
   }
}

static void TreeStruct(const char * sParam)
{
    PDmtNode ptrNode = GetNode( sParam );
    if ( ptrNode == NULL  ) {
        return;
    }
    SubTreeStruct( ptrNode, 0 );
}


static void SubscribeOnEvents()
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

   
    DmtEventSubscription oEvent;
    DmtEventSubscription oEvent1;

    UINT8 event =  SYNCML_DM_EVENT_ADD | SYNCML_DM_EVENT_REPLACE |
                          SYNCML_DM_EVENT_DELETE | SYNCML_DM_EVENT_INDIRECT;


    oEvent.Set(event,SYNCML_DM_EVENT_DETAIL);
           
    dm_stat = DmtTreeFactory::SubscribeEvent("./UnitTest", oEvent);
    if ( dm_stat != SYNCML_DM_SUCCESS )
       return;

    dm_stat = DmtTreeFactory::SubscribeEvent("./TEST", oEvent);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return;

    dm_stat = DmtTreeFactory::SubscribeEvent("./CLONE", oEvent);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return;
    
    dm_stat = DmtTreeFactory::SubscribeEvent("./TestRWPluginNode", oEvent);
    if ( dm_stat != SYNCML_DM_SUCCESS )
        return;

    GetTree();

    PDmtNode ptrNode;
    DmtData oData("server");


    ptrTree->DeleteNode("./DMAcc/Test");
    ptrTree->DeleteNode("./DMAcc/Test1");
    ptrTree->DeleteNode("./DMAcc/Test2");
    ptrTree->DeleteNode("./DMAcc/Test3");
    ptrTree->CreateInteriorNode( "./DMAcc/Test", ptrNode );
    ptrTree->CreateInteriorNode( "./DMAcc/Test1", ptrNode );
    ptrTree->CreateInteriorNode( "./DMAcc/Test2", ptrNode );
    ptrTree->CreateLeafNode( "./DMAcc/Test/ServerID", ptrNode, oData );
    ptrTree->CreateLeafNode( "./DMAcc/Test1/ServerID", ptrNode, oData );
    ptrTree->CreateLeafNode( "./DMAcc/Test2/ServerID", ptrNode, oData );
    if ( ptrNode )
    {
        ptrNode->SetValue(oData);
        ptrNode->SetValue(oData);
        ptrNode->SetValue(oData);
    }    
    ptrTree->RenameNode("./DMAcc/Test", "Test3");
    ptrTree->DeleteNode(  "./DMAcc/Test3" );
    ptrTree->DeleteNode("./DMAcc/Test1");
    ptrTree->DeleteNode("./DMAcc/Test2");

}


static void Usage()
{
  printf( "Supported commands:\n"
    "exit, quit, q - exit\n"
    "? - help screen\n"
    "get <node> - print node info\n"
    "createi <node> - create interior node\n"
    "createl <node> <string> - create string leaf node\n"
    "createli <node> <integer> - create integer leaf node\n"
    "createlz <node> true|false - create boolean leaf node\n"
    "createlb <node> <binary> - create binary leaf node\n"
    "createlbe <node> - create binary leaf node with default value\n"
    "createlf <node> <float> - create float leaf node\n"
    "createld <node> <date> - create date leaf node\n"
    "createlt <node> <time> - create time leaf node\n"
    "delete <node> - delete node\n"
    "rename <node> <new name> - rename node name\n"
    "setESN <node> <file> - set External Storage Node data\n"
    "settitle <node> <title> - set node title\n"
    "setacl <node> <acl> - set node acl\n"
    "set|replace <node> <string>- set string value\n"
    "seti <node> <integer> - set integer value\n"
    "setz <node> true|false - set boolean value\n"
    "setf <node> <float> - set float value\n"
    "setd <node> <date> - set date value\n"
    "sett <node> <time> - set time value\n"
    "setb <node> <bytes> - set bytes value\n"
    "dump <node> - dump the subtree from the node\n"
    "exec <node> - execute node\n"
    "getmap <node> - \n"
    "hmaccred <string> - build the credentials for HMAC \n"
/*    "setmap <node> - \n" */
    "ProcessScript <file> | <using binary xml [0/1]> - reads file and calls ProcessScript\n"
    "tree|open <node> - open subtree\n"
    "GetExclusiveTree <node> - get exclusive subtree subtree\n"
    "release - release the tree\n"
    "flush - flush the tree\n"
/*    "reset <mode> - reset tree\n" */
    "begin - begin atomic operations\n"
    "rollback - rollback atomic operations\n"
    "commit - commit atmoic operations\n"
/*  "alert - \n" */
    "clone <node> <new node> - clone a new node\n"
    "StartServerSession|connectsid <server ID> <using binary xml [0/1]> - starts server session\n"
    "sss <server ID> <using binary xml [0/1]> <correlator 1> <correlator 2> - starts server session\n"
    "connect <server_ip> <using binary xml [0/1]> - create new server ID and start server session\n"
    "createsid <server ip> - create new server ID\n"
    "showtimestamp [0/1] - toggle showing timestamp\n"
    "showprofiletime [0/1] - toggle showing profiling time\n"
    "version - show version of DM engine\n"
    "superagent - test SuperAgent function\n"
    "SetAutoRelease on|off - turn on/off autorelease of the tree\n"
    "struct - Show tree struct\n"
    "startCounter - Start counter for executed command\n"
    "endCounter - End counter for command\n"
    "displayCountStatus - Display count status ON/OFF\n"
    "resetCounter - Reset the counter back to zero\n"
    "getCounter - Get the number of executed command\n"
    );
}

int InteractiveMode(int argc, char** argv)
{
	if ( !DmtTreeFactory::Initialize() ) {
		printf("Fail to init DM tree\n");
		return 1;
	}

    if ( GetTree() == NULL ) {
        return 1;
    }
	ptrTree = NULL;

    SubscribeOnEvents();

    printf( "Interaction mode, type \"exit\" to exit, ? for help\n");

    while ( true ) {
        printf("\n] ");
    
        DMString sOperation = GetCmd();
        const char* szOperation = sOperation.c_str();

        if ( strcasecmp(szOperation, "") == 0 || szOperation[0] == '#') {
            continue;
        }

        printf("Cmd: %s%s\n", szOperation, s_strCurLine.c_str());

        DMString sParam1 = GetParam();
        if ( strcasecmp( szOperation, "setmap" ) == 0 ) {
            SetMap(sParam1);
            if ( !s_bAtomic ) {
                //ptrTree = NULL;
            }
            continue;
        }
        DMString sParam2 = GetParam();
        DMString sParam3 = GetParam();
        DMString sParam4 = GetParam();

  TestProfile  oProf( szOperation );
    

        if ( strcasecmp( szOperation, "exit" ) == 0 || 
            strcasecmp( szOperation, "quit" ) == 0 || 
            strcasecmp( szOperation, "q" ) == 0) {
            break;
        } else if ( strcasecmp( szOperation, "get" ) == 0 ) {
            Get( sParam1 );
        } else if ( strcasecmp( szOperation, "delete" ) == 0 ) {
            Delete( sParam1 );
        } else if ( strcasecmp( szOperation, "createi" ) == 0 ) {
           CreateInterior( sParam1 );
        } else if ( strcasecmp( szOperation, "createl" ) == 0 ) {
           CreateLeaf( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "createli" ) == 0 ) {
           CreateLeafInteger( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "createlz" ) == 0 ) {
           CreateLeafBoolean( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "createlb" ) == 0 ) {
           CreateLeafBinary( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "createlbe" ) == 0 ) {
           CreateLeafBinaryE( sParam1 );
        } else if ( strcasecmp( szOperation, "createlf" ) == 0 ) {
           CreateLeafFloat( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "createld" ) == 0 ) {
           CreateLeafDate( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "createlt" ) == 0 ) {
           CreateLeafTime( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "rename") == 0 ) {
            Rename( sParam1, sParam2);
        } else if ( strcasecmp( szOperation, "settitle") == 0 ) {
            SetTitle( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "setESN") == 0 ) {
		setESN( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "setacl" ) == 0 ) {
            SetAcl( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "set") == 0 ||
                strcasecmp( szOperation, "replace" ) == 0 ) {
            ReplaceString( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "seti" ) == 0 ) {
            ReplaceInteger( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "setz" ) == 0 ) {
            ReplaceBoolean( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "setb" ) == 0 ) {
            ReplaceBytes( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "setf" ) == 0 ) {
            ReplaceFloat( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "setd" ) == 0 ) {
            ReplaceDate( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "sett" ) == 0 ) {
            ReplaceTime( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "hmaccred" ) == 0 ) {
           BuildCredHMAC( sParam1 );
        } else if ( strcasecmp( szOperation, "dump" ) == 0 ) {
            DumpTree( sParam1 );
        } else if (strcasecmp( szOperation, "exec") == 0 ||
                strcasecmp(szOperation, "execute") == 0 ) {
            Execute( sParam1, sParam2 );
        } else if (strcasecmp( szOperation, "getmap") == 0) {
            GetMap(sParam1);
        } else if (strcasecmp( szOperation, "open") == 0 || 
                strcasecmp(szOperation, "tree") == 0 ) {
            Open( sParam1 );
        }        else if (strcasecmp(szOperation, "GetExclusiveTree") == 0 ) {
            GetExclusiveTree( sParam1 );
        } else if (strcasecmp( szOperation, "release") == 0) {
            Release();
        } else if (strcasecmp( szOperation, "flush") == 0) {
            Flush();
        } else if (strcasecmp( szOperation, "begin") == 0) {
            Begin();
        } else if (strcasecmp( szOperation, "commit") == 0) {
            Commit();
        } else if (strcasecmp( szOperation, "rollback") == 0) {
            Rollback();
        } else if (strcasecmp( szOperation, "clone") == 0) {
            Clone( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "ProcessScript" ) == 0 ) {
	  ProcessScript( sParam1 , sParam2);
        } else if ( strcasecmp( szOperation, "connectsid" ) == 0 ||
                strcasecmp(szOperation, "StartServerSession") == 0 ) { 
           StartServerSession( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "sss" ) == 0 ) {
            StartServerSessionEx( sParam1, sParam2, sParam3, sParam4 );
        } else if ( strcasecmp( szOperation, "createsid" ) == 0 ) {
            CreateServerId( sParam1 );
        } else if ( strcasecmp( szOperation, "connect") == 0) {
            CreateAndStartServerSession( sParam1, sParam2 );
        } else if ( strcasecmp( szOperation, "showtimestamp") == 0) {
            ShowTimestamp( sParam1 );
        }        else if ( strcasecmp( szOperation, "showprofiletime") == 0) {
            ShowProfileTime( sParam1 );
        } else if ( strcasecmp( szOperation, "version") == 0) {
            DMVersion(); 
	 } else if ( strcasecmp( szOperation, "superagent") == 0) {
            SuperAgent(); 
	 } else if ( strcasecmp( szOperation, "SetAutoRelease" ) == 0 ) {
			s_bAutoReleaseTree = 
					(strcasecmp(sParam1, "on") == 0 ||
					strcasecmp(sParam1, "yes") == 0 ||
					strcasecmp(sParam1, "true") == 0 ||
					strcasecmp(sParam1, "1") == 0);
					printf("Autorelease is now %s\n", s_bAutoReleaseTree ? "ON" : "OFF");
        } else if ( strcasecmp( szOperation, "struct") == 0) {
            TreeStruct( sParam1 );
       
       } else if ( strcasecmp( szOperation, "startCounter") == 0) {
            startCounter();
       } else if ( strcasecmp( szOperation, "endCounter") == 0) {
            endCounter();
       } else if ( strcasecmp( szOperation, "displayCountStatus") == 0) {
            displayCountStatus();
        } else if ( strcasecmp( szOperation, "resetCounter") == 0) {
            resetCounter();
        } else if ( strcasecmp( szOperation, "getCounter") == 0) {
            getCounter();	
        } else if ( strcasecmp( szOperation, "?" ) == 0 ||
                strcasecmp( szOperation, "help" ) == 0) {
           Usage();
        } else {
           printf( "Unknown option %s.\n", szOperation );
           Usage();
        }
        if (!s_bAtomic && s_bAutoReleaseTree) {
            ptrTree = NULL;
        }
    }

    DmtTreeFactory::Uninitialize();
    return 0;
}

int defaultFunction()
{
    if ( !DmtTreeFactory::Initialize() ) {
        printf("Fail to init DM tree\n");
        return 1;
    }

    if ( (e=DmtTreeFactory::GetTree(principal, ptrTree)) != NULL ) {
        printf( "%s can't get tree, %d\n", c_szErrorText, e->GetErrorCode() );
        return 1;
    }

    PDmtNode ptrNode;
	DMString strNode = "./DevDetail/Bearer/GSM";

	if ( (e=ptrTree->GetNode( strNode, ptrNode )) != NULL )
	  printf( "can't get node %s, error %s\n", strNode.c_str(), (const char*)e->GetErrorText().c_str() );
	else {
	  e = ptrNode->SetValue( DmtData("123"));
	  if ( e != NULL )
		printf( "can't set node value [%s], error %s\n", strNode.c_str(), (const char*)e->GetErrorText().c_str() );

	}

	ptrNode = NULL;
	ptrTree = NULL;

	printf(" start server session\n");
	e = DmtTreeFactory::StartServerSession( "Scts", false );
	
	if ( e != NULL ) {
	  printf( "error %d\n", e->GetErrorCode());
    }

    DmtTreeFactory::Uninitialize();

    printf("Done.\n");
    return 0;
}

static void isVersionOn(int argc, char** argv)
{
  for(int i=1; i<argc; i++)
  {
    if(strcmp(argv[i], "-version") == 0 )
    {
        DMVersion();
        break;
    }
  }
}   

int main(int argc, char** argv)
{
  isVersionOn(argc, argv);
  const char* szParam1 = argc > 1 ? argv[1] : "";
  if ( strcmp( szParam1, "unittest" ) == 0 ) {
	return defaultFunction();
  }
  else if ( strcmp( szParam1, "mt" ) == 0 ) {
	return MultiProcessTest();
  }

  return InteractiveMode(argc, argv);
}
