#include "dmt.hpp"

#include <stdlib.h>
#include <string>
#include <map>
#include <stdio.h>

#include "plugin/dmtCommitPlugin.hpp"
#include "plugin/dmtPlugin.hpp"


#define DIM(array) (sizeof(array)/sizeof((array)[0]))


extern "C" 
void DMT_PluginLib_OnCommit(const DmtEventMap & updatedNodes,
                                              DMStringMap & mapParameters,
                                              PDmtTree tree )
{
 
    DMString strParam;
    int bPrint = (mapParameters.lookup("print", strParam) && strParam == "on" );
  
    if ( !bPrint )
        return;

    mapParameters.lookup("id", strParam);
    printf( "Commit plug-in: OnCommit [%s]<START> \n", strParam.c_str() );
  
    for (DmtEventMap::POS nPos = 0; nPos < updatedNodes.end(); nPos++ )
    {
       const DMString & eventPath =  updatedNodes.get_key(nPos);
       const DmtEventDataVector & aVector =  updatedNodes.get_value(nPos);

       for (INT32 index=0; index<aVector.size(); index++)
       {
  
           DMString szActions;
            if  ( (aVector[index]->GetAction() & SYNCML_DM_EVENT_ADD) == SYNCML_DM_EVENT_ADD )
            {
                szActions = "ADD";
                printf( "notification for uri [%s/%s], [%s]\n", 
                         eventPath.c_str(), (aVector[index]->GetName()).c_str(), szActions.c_str() );
            }

            if  ( (aVector[index]->GetAction() & SYNCML_DM_EVENT_REPLACE) == SYNCML_DM_EVENT_REPLACE )
            {
                szActions = "REPLACE";
                printf( "notification for uri [%s/%s], [%s]\n", 
                         eventPath.c_str(), (aVector[index]->GetName()).c_str(), szActions.c_str() );
            }
                      
            if  ( (aVector[index]->GetAction() & SYNCML_DM_EVENT_DELETE) == SYNCML_DM_EVENT_DELETE )
            {
                szActions = "DELETE";
                printf( "notification for uri [%s/%s], [%s]\n", 
                         eventPath.c_str(), (aVector[index]->GetName()).c_str(), szActions.c_str() );
            }

            if  ( (aVector[index]->GetAction() & SYNCML_DM_EVENT_RENAME) == SYNCML_DM_EVENT_RENAME )
            {
                szActions = "RENAME";
                printf( "notification for uri [%s/%s], [%s]\n", 
                         eventPath.c_str(), (aVector[index]->GetName()).c_str(), szActions.c_str() );
            }

             if  ( (aVector[index]->GetAction() & SYNCML_DM_EVENT_INDIRECT) == SYNCML_DM_EVENT_INDIRECT )
            {
                szActions =  "INDIRECT UPDATE";
                printf( "notification for uri [%s/%s], [%s]\n", 
                         eventPath.c_str(), (aVector[index]->GetName()).c_str(), szActions.c_str() );
            }
                        
          };
        
      
  }
  printf( "Commit plug-in: OnCommit <END> \n");
}


extern "C" 
int DMT_PluginLib_GetAPIVersion(void)
{
   return DMT_PLUGIN_VERSION_1_1;
}

 
