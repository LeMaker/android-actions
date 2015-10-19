#include "dmt.hpp"

#include <stdio.h>

#include "jem_defs.hpp"
#include "dmt.hpp"

#include "plugin/dmtPlugin.hpp"

//#define Debug printf
#define Debug //

//
// Plugin DLL
//

/* 
Plugin Ini Format:


[./DevDetail/xxx/Node Path]
data=xxxx.so

/=[interiornode|leafnode]
/interiornode=[leafnode2|leafnode3]
/interiornode/leafnode2=123
/interiornode/leafnode3=abc
/leafnode=my leaf node

*/

static bool DmStringParserGetItem( DMString& strItem, DMString& strReminder, char cDelim )
{
  if ( strReminder[0] == 0 )
    return false;

  const char* s = strchr( strReminder, cDelim );
  int nPos = s ? s - strReminder : -1; //strReminder.find( cDelim );
  
  if ( nPos < 0 ){
    strItem = strReminder;
    strReminder = "";
  }
  else {
    strItem.assign( strReminder, nPos );
    //strReminder.erase( 0, nPos+1 );
    strReminder = DMString(s+1);
  }
  return true;
}

static int DmStrToStringVector(const char * pStr, int nLen, DMStringVector& oVector, char cDelim )
{
   DMString strChild, strList; // = (const char*)pStr;
   if (nLen >0)
   {
      strChild= DMString((const char*)pStr, nLen);      
      strList= DMString((const char*)pStr, nLen);      
   } else
   {
      strChild= (const char*)pStr;
      strList = (const char*)pStr;
   }
   
   int i=0;
   while ( DmStringParserGetItem( strChild, strList, cDelim ) ) 
   {
      oVector.push_back(strChild);
      i++;
   }
   return i;
}

extern "C" 
SYNCML_DM_RET_STATUS_T DMT_PluginLib_Data_GetPluginTree(
	CPCHAR pluginRootNodePath,
	DMStringMap & mapParameters,	//For the Tree
	PDmtAPIPluginTree & pPluginTree	//root tree for the current path
)
{
   SYNCML_DM_RET_STATUS_T status = SYNCML_DM_SUCCESS;

   Debug("DMT_PluginLib_Data_GetTree, pluginRootNodePath=%s\n", pluginRootNodePath);
   
   PDmtPluginTree pMyTree=new DmtPluginTree();
    pMyTree->Init(pluginRootNodePath);
   int numNodes=0;

   for ( DMMap<DMString, DMString>::POS it = mapParameters.begin(); it != mapParameters.end(); it++ ) 
   {
      DMString paramName = mapParameters.get_key( it );
      DMString paramValue= mapParameters.get_value( it );

      const char * pName=paramName.c_str();
      
      //In parameter file, node name MUST start with /
      PDmtPluginNode pNode;
      Debug("parameter %s=%s\n", pName, paramValue.c_str());
         
      if (pName[0]=='/')
      {
         pName++;   //Plugin Node path is still relative path when created.

         const char * pValue=paramValue.c_str();
         if (pValue[0]=='[')
         {
            //Interior node
            pValue++;
            int len=strlen(pValue);
            if ( pValue[len-1]==']')
            {
               len--;
            }
            
            DMStringVector oChildren;
            DmStrToStringVector( pValue,          //const char * pStr, 
               len,
               oChildren,     //DMStringVector oVector, 
               '|'            //char cDelim 
            );
            
            pNode=PDmtPluginNode(new DmtPluginNode());

            pNode->Init(pMyTree, pName, oChildren);
            Debug("Add Interior Plugin Node %s\n", pName);
            pMyTree->SetNode(pName, 
               PDmtNode(pNode.GetPtr())
            );
            numNodes++;
         } else
         {
            //leaf node
            
            pNode=PDmtPluginNode(new DmtPluginNode());
            pNode->Init(pMyTree, pName, DmtData(paramValue.c_str()));

            Debug("Add Leaf Plugin Node %s\n", pName);
            pMyTree->SetNode(pName, 
               PDmtNode(pNode.GetPtr())
            );
            numNodes++;
         }
      }
   }

   if (numNodes==0)
   {
      const char * pName="";
      PDmtPluginNode pNode;

      DMStringVector oChildren;
      pNode=PDmtPluginNode(new DmtPluginNode());
      pNode->Init(pMyTree, pName, oChildren);

      Debug("No parameters found, Add a default Root Plugin Node %s\n", pName);
      pMyTree->SetNode(pName, 
         PDmtNode(pNode.GetPtr())
      );
   }

   pPluginTree=pMyTree;
   return status;
}

extern "C" 
int DMT_PluginLib_GetAPIVersion(void)
{
   return DMT_PLUGIN_VERSION_1_1;
}
