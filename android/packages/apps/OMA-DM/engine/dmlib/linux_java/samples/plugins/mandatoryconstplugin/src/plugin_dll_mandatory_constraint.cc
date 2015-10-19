#include "dmt.hpp"

#include "dmMemory.h"
#include "dmDebug.h"     // debug macroses

#include <stdlib.h>
#include <string>
#include <map>
#include <stdio.h>

#include "plugin/dmtPlugin.hpp"

#define Debug printf

//---------------------------Declaration----------------------

PDmtErrorDescription 
DMCheckMandatoryForSingleNode(
	   const char * szPath,             //Single path (removed *)
   	DMStringMap & mapParameters,
	   PDmtTree tree                    //Global Tree with same access rights for current session
   )
{
   bool nodeExist=false;
   int numNodes=0;

   Debug("Checking single node=%s\n", szPath);
   nodeExist=tree->IsValidNode(szPath);
   if (nodeExist)
      numNodes ++;   //Including myself

   int numPaths=0;
   for ( DMMap<DMString, DMString>::POS it = mapParameters.begin();
      it != mapParameters.end(); it++ )
   {
      DMString name = mapParameters.get_key( it );
      //ignore value = it->second;

      DMString childPath=name;

      //For Relative path, start with /. Absolute path start with .
      if (name[0] == '/')  
      {
         childPath=szPath;
         //childPath +="/";
         childPath += name;
         numPaths ++;
      } else
      if (name[0] != '.')
      {
         //other parameters, ignore for now...
         continue;
      }

      const char * szName;
      szName=childPath.c_str();
      nodeExist=tree->IsValidNode(szName);
      Debug(" Check for existence of %s=%d\n", szName, nodeExist);
      if (nodeExist)
         numNodes ++;      
   }
   Debug("numNodes=%d\n", numNodes);
   Debug("numPaths=%d\n", numPaths);

   if (numNodes ==0 || numNodes== 1+ numPaths)
   {
      Debug("constraint Succeed\n");
      return PDmtErrorDescription();
   }
   Debug("constraint Failed !!!!!!\n");
   return PDmtErrorDescription( new DmtErrorDescription(enumDmtResult_ConstraintFailed) );
}

extern "C" 
PDmtErrorDescription DMT_PluginLib_CheckConstraint(
	const char * path,          //Node that was invoked on for that was changed
	DMStringMap & mapParameters,
	PDmtTree tree        //Global Tree with same access rights for current session
   )
{
   Debug("CheckConstraint [%s]\n", path);

   if (tree == NULL)
   {  
      //When tree is NULL, means we cannot get session/lock?
      return PDmtErrorDescription( new DmtErrorDescription(enumDmtResult_UnableStartSession) );
   }

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

      //No multi node
      return DMCheckMandatoryForSingleNode(
         path, 
         mapParameters,
         tree
      );
}


extern "C" 
int DMT_PluginLib_GetAPIVersion(void)
{
   return DMT_PLUGIN_VERSION_1_1;
}

