#include "dmt.hpp"

#include <stdlib.h>
#include <string>
#include <map>
#include <stdio.h>

#include "plugin/dmtPlugin.hpp"

#define Debug printf

/* 
Plugin Ini Format:


[./DevDetail/xxx/Node Path]
constraint=xxxx.so

./DevDetail/interiornode=          
/interiornode/leafnode2=

*/

extern "C" 
SYNCML_DM_RET_STATUS_T DMT_PluginLib_CheckConstraint(
	CPCHAR path,   //This is the path that matches the ini file parameter
	DMStringMap& mapParameters,
	PDmtTree tree        //Global Tree with same access rights for current session
   )
{
   if (tree == NULL)
   {  
      //When tree is NULL, means we cannot get session/lock?
      return (SYNCML_DM_FAIL);
   }
   bool nodeExist=false;

#if 0   
      //Test if DmtTree is OK
      PDmtNode node;
      PDmtErrorDescription e=tree->GetNode( 
         ".",  //const char* path, 
         node //PDmtNode& ptrNode 
      );
      if (e!=NULL)
      {
         printf( "Can't get node from DmtTree for %s, err=%s\n", ".", (const char*)e->GetErrorText().c_str() );         
      }
      nodeExist=tree->IsValidNode(".");      
      Debug( " nodeExist for %s=%d\n", ".", nodeExist );
      nodeExist=tree->IsValidNode("./SyncML");      
      Debug( " nodeExist for %s=%d\n", "./SyncML", nodeExist );

#endif 

   Debug("CheckConstraint for path %s  tree=0x%x\n", path, tree.GetPtr() );

   int numPaths=0;   //How many in parameters
   int numNodes=0;   //How many exists

   for ( DMMap<DMString, DMString>::POS it = mapParameters.begin(); it != mapParameters.end(); it++ ) 
   {
      DMString paramName = mapParameters.get_key( it );
      DMString paramValue= mapParameters.get_value( it );

      const char * pName=paramName.c_str();

      if (* pName !='.' && * pName !='/' )
         continue;

      numPaths++;
      
      DMString strPath;
      if (* pName =='.') 
         strPath=paramName;      //Absolute path
      else  //(* pName =='/')    //Relative path
      {
         strPath +=path;
         if ( (* (pName+1)) !='\0' )
            strPath +=pName;  // pName already started with "/", no need for
      }   
      const char * szPath=strPath.c_str();      

      //Debug("szPath.len=%d\n", strlen(szPath));
      nodeExist=tree->IsValidNode(szPath);
      Debug("check for isValidNode of %s nodeExist=%d\n", szPath, nodeExist);
      
      if (nodeExist)
      {
         Debug(" %s isValidNode\n", szPath);
         numNodes++;
      }
   }
   Debug("numNodes=%d\n", numNodes);
   if (numNodes ==0 || numNodes== numPaths)
   {
      Debug("constraint Succeed\n");
      return (SYNCML_DM_SUCCESS);
   }
   Debug("constraint Failed !!!!!!\n");
   return (SYNCML_DM_CONSTRAINT_FAIL);
}


extern "C" 
int DMT_PluginLib_GetAPIVersion(void)
{
   return DMT_PLUGIN_VERSION_1_1;
}

