#include <stdio.h>
#include <string.h>

///////////////////////////////////////////////////////////////
// global data
enum EnumMode { enum_Mode_Quiet, enum_Mode_Default, enum_Mode_MDF } g_eMode = enum_Mode_Default;

const char* c_szType[] = {
  "null",  "chr", "int", "bool",
  "bin", "node", "", "",
  "", "test", "", "",
  "", "", "", ""
};

const char* c_szAccess[] = {
  "",     // 0
  "access:Add",  // 1
  "access:Delete",  // 2
  "access:Add,Delete",  // 2 + 1
  "access:Get",  // 4
  "access:Get,Add",  // 1+4
  "access:Get,Delete",  // 2 + 4
  "access:Get,Add,Delete",  // 1 +2+4
  "access:Replace",  // 8
  "access:Add,Replace",  // 1 + 8
  "access:Delete,Replace",  // 2 + 8
  "access:Add,Delete,Replace",  // 1 +2+8
  "access:Get,Replace",  // 4 + 8
  "access:Add,Get,Replace",  // 1+ 4 +8
  "access:Delete,Get,Replace",  // 2 + 4 + 8
  "access:Add,Delete,Get,Replace",  // 1+ 2 + 4 + 8

  "access:Exec",     // 0
  "access:Exec,Add",  // 1
  "access:Exec,Delete",  // 2
  "access:Exec,Add,Delete",  // 2 + 1
  "access:Exec,Get",  // 4
  "access:Exec,Get,Add",  // 1+4
  "access:Exec,Get,Delete",  // 2 + 4
  "access:Exec,Get,Add,Delete",  // 1 +2+4
  "access:Exec,Replace",  // 8
  "access:Exec,Add,Replace",  // 1 + 8
  "access:Exec,Delete,Replace",  // 2 + 8
  "access:Exec,Add,Delete,Replace",  // 1 +2+8
  "access:Exec,Get,Replace",  // 4 + 8
  "access:Exec,Add,Get,Replace",  // 1+ 4 +8
  "access:Exec,Delete,Get,Replace",  // 2 + 4 + 8
  "access:Exec,Add,Delete,Get,Replace",  // 1+ 2 + 4 + 8
  
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
static int GetInt( const unsigned char* buf, int& nOffset )
{
  int value = 0;

  for (int i=0; i < 4; i++) 
  {
    value |= (*(buf+nOffset)) << (i*8);
    nOffset++;
  }  
  return value; 
}

///////////////////////////////////////////////////////////////
static int GetInt16( const unsigned char* buf, int& nOffset )
{
  int value = 0;

  for (int i=0; i < 2; i++) 
  {
    value |= (*(buf+nOffset)) << (i*8);
    nOffset++;
  }  
  return value; 
}


///////////////////////////////////////////////////////////////
static int GetPrintItem( const unsigned char* buf, int& nOffset, long nSize, int nLevel, const char* szParentURI )
{
  int nRes = 0;
  char szIndent[100];
  int nStartOffset = nOffset;
  char  szCurrentURI[1024]; 

  strcpy( szCurrentURI, szParentURI );

  for ( int i = 0; i < nLevel; i++ ){
	szIndent[i] = '\t';
  }
  szIndent[nLevel]=0;

  if ( nOffset + 11 >= nSize ){
	if ( g_eMode != enum_Mode_Quiet )
	  printf( "Item does not fit buffer completelly, offset %d, \n", nOffset);

	return 6;
  }

  int nNameOffset = GetInt( buf, nOffset );

  const unsigned char* szName = buf + nNameOffset;

  if ( nNameOffset >= nSize ){
	szName = (const unsigned char*)"<<out of the buffer>>";
	nRes = 7;
  }

  strcat( szCurrentURI, (const char*)szName );
  
  int nType = GetInt16( buf, nOffset );
  int nIDOffset = 0;

  if ( nType & 0x400 )
    nIDOffset = GetInt( buf, nOffset );

  const unsigned char* szID = nIDOffset ? buf + nIDOffset :(const unsigned char*)"<unset>";

  if ( nIDOffset >= nSize ){
	szID = (const unsigned char*)"<<out of the buffer>>";
	nRes = 7;
  }

  int nAccess = buf[nOffset++];
  int nMimeType = buf[nOffset++];
  int nChildren = GetInt16( buf, nOffset );

  int aChildren[500];

  if ( nOffset + nChildren * 4 >= nSize ){
	if ( g_eMode != enum_Mode_Quiet )
	  printf( "Item does not fit buffer completelly, offset %d, \n", nOffset);

	return 6;
  }

  for( int i = 0; i < nChildren; i++ ){
	aChildren[i] = GetInt(buf, nOffset );
  }

  int nConst = buf[nOffset++];

  if ( g_eMode == enum_Mode_Default)
    printf( "%s%04x-%04x: Name (%03x): \"%s\", type %x\tid %s\taccess %x,\tmime %x,\tchildren count %x,\t constr %x\n", 
  		  szIndent, nStartOffset, nOffset-1, nNameOffset,
  		  szName,
			/*szIndent,*/ nType, szID,
			nAccess, nMimeType, nChildren, nConst  );
  else if ( g_eMode == enum_Mode_MDF) {
    printf( "[%s]\n"
    "type:%s\n%s\n", 
  		  szCurrentURI,
  		  c_szType[nType &0x0f],
  		  c_szAccess[nAccess &0x1f] );

	if ( nType & 0x400 )
	  printf( "ID:%s\n", szID );

	if ( nType & 0x200 )
	  printf( "HandledByPlugin:1\n" );

	if ( nType & 0x100 )
	  printf( "storesPD:1\n" );
  }

  if ( strcmp( (const char*)szName, "." ) ==0 )
    szCurrentURI[0]=0;

  strcat(szCurrentURI, "/" );
  
  
  // constraints
  for ( int i = 0; i < nConst; i++ ){
	int nConstType = buf[nOffset++];

	const char* sz_Const[] = {
	  "",
	  "min",
	  "max",
	  "values",
	  "default",
	  "minLen",
	  "maxLen",
	  "regexp",
	  "nMaxLen",
	  "nValues",
	  "nRegexp",
	  "auto",
	  "recur-after-segment",
	  "max-recurrence",
	  "fk",
	  "child",
	  "depend",
	  "maxChild"
	};

	if ( nOffset + 4 >= nSize ){
	  if ( g_eMode != enum_Mode_Quiet )
		printf( "Item does not fit buffer completelly, offset %d, \n", nOffset);

	  return 6;
	}

	int nValue = 0;
    char szStrValue[30] = "";
	const unsigned char* szValue = (const unsigned char*)szStrValue;

	switch ( nConstType ){
	default:
	  if ( g_eMode != enum_Mode_Quiet )
	    printf( "Invalid constraint type %d, offset %d, \n", nConstType, nOffset);
	  return 6;
	case 3: case 7: case 9: case 10: case 11: case 12: case 14:  case 15: case 16:
	  nValue = GetInt( buf, nOffset );

	  if ( nValue >= nSize ){
		szValue = (const unsigned char*)"<<out of the buffer>>";
		nRes = 7;
	  }
	  else
		szValue = buf + nValue;
	  break;
    
	case 1: case 2: 
	  nValue = GetInt( buf, nOffset );
    sprintf(szStrValue, "%d", nValue);
	  break;
	case 5: case 6: case 8: case 13: case 17:
	  nValue = GetInt16( buf, nOffset );
    sprintf(szStrValue, "%d", nValue);
	  break;
	case 4:
	  if ( (nType & 0x7f) == 3 ) {// bool
		nValue = buf[nOffset++];
      sprintf(szStrValue, "%s", nValue ? "true": "false");
	  }
	  else {
		nValue = GetInt( buf, nOffset );

      if( (nType & 0x7f) == 2 ) // int
        sprintf(szStrValue, "%d", nValue);
      else {
      	  if ( nValue >= nSize ){
      		szValue = (const unsigned char*)"<<out of the buffer>>";
      		nRes = 7;
      	  }
      	  else
      		szValue = buf + nValue;
      }
        
	  }

	  break;
	}

  if ( g_eMode == enum_Mode_Default)
    printf("%s  constraint type %s, value %x \"%s\"\n", szIndent, sz_Const[nConstType], nValue, szValue );
  else if ( g_eMode == enum_Mode_MDF)
    printf("%s:%s\n", sz_Const[nConstType], szValue );
  

  }

  for ( int i = 0; i < nChildren; i++ ){
	int n = aChildren[i];
	
	int nResFromChildren = GetPrintItem( buf, n, nSize, nLevel + 1, szCurrentURI );

	if ( nResFromChildren )
	  nRes = nResFromChildren;
  }
  return nRes;
}

///////////////////////////////////////////////////////////////
static int ProcessBuffer( FILE* f, unsigned char* buf, long nSize )
{
  if ( fread( buf, 1, nSize, f ) != nSize )
	return 6;

  if ( g_eMode == enum_Mode_Default )
	printf("buffer loaded...\n");

  int nOffset = 0;

  int n = GetInt( buf, nOffset );
  int nVer = GetInt16( buf, nOffset );

  if ( g_eMode == enum_Mode_Default )
	printf( "%x: fsize %x; ver %x (%s)\n", nOffset - 6, n, nVer, n == nSize ? "valid" : "invalid");

  return GetPrintItem( buf, nOffset, nSize, 0, "" );
}

///////////////////////////////////////////////////////////////
static int ProcessFile( FILE* f )
{
  fseek( f, 0, SEEK_END );
  long nSize = ftell( f );
  fseek( f, 0, SEEK_SET );

  if ( nSize <= 0 )
	return 3;

  if ( g_eMode == enum_Mode_Default )
	printf( "File size is %d (dec)\n", nSize );

  unsigned char* buf = new unsigned char[nSize+1];
  if ( !buf )
	return 4;

  buf[nSize]=0;
  int nRet = ProcessBuffer( f, buf, nSize );

  delete buf;
  return nRet;
}

///////////////////////////////////////////////////////////////
static void Usage()
{
	printf("Simple reader and verifier for binary MDF files\n"
		   "always return 0 code for valid file and non zero for corrupted file\n"
		   "usage: bmdf_reader [-q] [-mdf] <bmdf file name>\n"
		   "-q -- quiet mode, return only error code (0 valid file, not 0 - invalid file)\n"
		   "-mdf -- generate text mdf\n"
		   );
}

///////////////////////////////////////////////////////////////
static const char*  ParseCmdLineOptions(int argc, char** argv)
{
  const char* szFileName = NULL;

  for ( int i = 1; i < argc; i++ ){
	if ( strcasecmp( argv[i], "-q") == 0 )
	  g_eMode = enum_Mode_Quiet;
	else if ( strcasecmp( argv[i], "-mdf") == 0 )
	  g_eMode = enum_Mode_MDF;
	else {
	  szFileName = argv[i];
	  break;
	}
  }

  return szFileName;
}

///////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
  const char* szFileName = ParseCmdLineOptions( argc, argv );

  if ( !szFileName ){
	Usage();
	return 1;
  }

  FILE* f = fopen( szFileName, "r" );

  if ( !f ){
	printf("unable to open file %s\n", szFileName );
	return 2;
  }

  int nRet = ProcessFile( f );
  fclose( f );

  if ( g_eMode == enum_Mode_Default )
	printf( "\ndone, file is %s\n", nRet ? "invalid" : "ok");

  return nRet;
}
