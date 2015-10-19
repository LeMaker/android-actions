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

/*==================================================================================================

    Header Name: dmExractServerIDSession.cc

    General Description: Implementation of DMBootstrapSession class.

==================================================================================================*/

#include "dmBootstrapSession.h"
#include "dm_ua_handlecommand.h"
#include "xpl_Logger.h"
#include "dmProcessScriptSession.h"
#include "dmClientSrvCreds.h"

extern "C" {
#include <sml.h>
#include <smldtd.h>
#include <smldevinfdtd.h>
#include <smlmetinfdtd.h>
#include <mgrutil.h>
#include <smlerr.h>
}

/*==================================================================================================
                                 SOURCE FUNCTIONS
==================================================================================================*/

static Ret_t
HandleBootstrapStartMessage (InstanceID_t    id,
                             VoidPtr_t       userData,
                             SmlSyncHdrPtr_t pContent);


static Ret_t
HandleBootstrapChkAddCommand (InstanceID_t id, 
                              VoidPtr_t    userData,
                              SmlAddPtr_t  pContent);

/*==================================================================================================
FUNCTION        : DMBootstrapSession::SessionStart

DESCRIPTION     : The UserAgen::SessionStart calls this function after it creates MgmtSession object.
                  The function will perform the following operations:
                  1) Call SessionStart() to setup the DM tree.
                  2) Register the DM engine with the SYNCML toolkit.
                  3) Connect the client with the server.
                  4) Build and send the package one.
ARGUMENT PASSED : p_SessionStart
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMBootstrapSession::Start(const UINT8 *docInputBuffer , 
                                UINT32 inDocSize, 
                                BOOLEAN isWBXML, 
                                DMString & serverID)
{
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_FAIL;
    Ret_t                  sml_ret_stat;

    XPL_LOG_DM_SESS_Debug(("Entered DMBootstrapSession::SessionStart\n"));

#ifdef DM_BOOTSTRAP_DEBUG
    FILE *fp = fopen("/tmp/dm_triage.dat", "w");
    fwrite(docInputBuffer, sizeof(char), inDocSize, fp);
    fclose(fp);
#endif

    ret_stat = Init(isWBXML);
    if ( ret_stat != SYNCML_DM_SUCCESS )
        return ret_stat;
    
    /* Register the DM engine with the SYNCML toolkit. */
    ret_stat = RegisterDmEngineWithSyncmlToolkit(&serverID);
    if ( ret_stat != SYNCML_DM_SUCCESS )
    {
        XPL_LOG_DM_SESS_Debug(("Exiting: RegisterDmEngineWithSyncmlToolkit failed\n"));
        return (ret_stat);
    }
    
    sml_ret_stat = smlLockWriteBuffer(recvInstanceId, &pWritePos, &workspaceFreeSize);

    if ( sml_ret_stat != SML_ERR_OK )
        return SYNCML_DM_FAIL;
    
    memcpy(pWritePos,docInputBuffer,inDocSize);
    
    smlUnlockWriteBuffer(recvInstanceId, inDocSize);

    smlProcessData(recvInstanceId, SML_ALL_COMMANDS);

    if ( serverID.GetBuffer() == NULL )
        return SYNCML_DM_FAIL;
    
    return SYNCML_DM_SUCCESS;
}


/*==================================================================================================
FUNCTION        : DMSession::SetToolkitCallbacks

DESCRIPTION     : This function will to set toolkit callback functions.
             
                  
ARGUMENT PASSED :
OUTPUT PARAMETER:
RETURN VALUE    : It returns SYNCML_DM_SUCCESS.
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMBootstrapSession::SetToolkitCallbacks(SmlCallbacks_t * pSmlCallbacks)
{
    pSmlCallbacks->startMessageFunc = HandleBootstrapStartMessage;
    pSmlCallbacks->addCmdFunc       = HandleBootstrapChkAddCommand;
    return (SYNCML_DM_SUCCESS);
}



/*========================= C FUNCTIONS USED BY THE SYNCML TOOLKIT FOR BOOTSTRAP =================*/
/*==================================================================================================
FUNCTION        : HandleBootstrapStartMessage (C function)

DESCRIPTION     : This function should analyze SyncHeader data from received DM document.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleBootstrapStartMessage (InstanceID_t    id,
                             VoidPtr_t       userData,
                             SmlSyncHdrPtr_t pContent)
{


    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    dm_stat = VerifyProtocolVersion(pContent);    

    smlFreeSyncHdr(pContent);

    if ( dm_stat == SYNCML_DM_FAIL )
      return (SML_ERR_XLT_INVAL_SYNCML_DOC);
    else    
         return (SML_ERR_OK);
}

/*==================================================================================================
FUNCTION        : BootstrapChkTndsServerID

DESCRIPTION     : The utility function is to check Server ID from TNDS object.

ARGUMENT PASSED : p_meta_info
OUTPUT PARAMETER: pp_type
                  p_format
RETURN VALUE    : SYNCML_DM_RET_STATUS_T
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T BootstrapChkTndsServerID(
                                    SmlDmTndNodeListPtr_t p_nodelist,
                                    DMString &serverID)
{
   SYNCML_DM_RET_STATUS_T ret = SYNCML_DM_FAIL;
   while ( NULL != p_nodelist && ret != SYNCML_DM_SUCCESS )
   {
      SmlDmTndNodePtr_t p_tnd_node = p_nodelist->node;
      if ( p_tnd_node != NULL )
      {
         DMString nodeName;
         nodeName =(CPCHAR)p_tnd_node->nodename->content;

         KCDBG("TNDS Node Name: %s", nodeName.c_str());

         if ( nodeName == DM_SERVERID_1_2 )
         {
            if ( NULL == p_tnd_node->value )
            {
               return SYNCML_DM_SUCCESS;
            }
       
            if ( strncmp((CPCHAR)p_tnd_node->value->content, DM_INBOX, strlen(DM_INBOX)) == 0 )
            {
               serverID = (CPCHAR)p_tnd_node->value->content + strlen(DM_INBOX) + 1;
            }
            else
            {
               serverID = (CPCHAR)p_tnd_node->value->content;
            }

            return SYNCML_DM_SUCCESS;
         }
         ret = BootstrapChkTndsServerID(p_tnd_node->nodelist, serverID);
      }

      // Process sibling node
      p_nodelist = p_nodelist->next;
   }

   return ret;
}

/*==================================================================================================
FUNCTION        : BootstrapChkSetMetaData

DESCRIPTION     : The utility function to set up the meta data.

ARGUMENT PASSED : p_meta_info
OUTPUT PARAMETER: pp_type
                  p_format
RETURN VALUE    : SYNCML_DM_RET_STATUS_T
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T BootstrapChkSetMetaData(SmlPcdataPtr_t p_meta_data, 
                                    DMBuffer& pp_type,
                                    SYNCML_DM_FORMAT_T *p_format)
{
    SmlMetInfMetInfPtr_t p_meta_info;
    CPCHAR p_temp_content;

    /* Setup OUTPUT parameter type and format. If p_meta_data is not set, assign default value
       to OUTPUT parameters, type as "text/plain", format as "chr". */
    if (p_meta_data != NULL)
    {
        if (p_meta_data->content != NULL)
        {
            if ( SML_PCDATA_EXTENSION != p_meta_data->contentType &&
                 SML_EXT_METINF != p_meta_data->extension )
            {
                *p_format = SYNCML_DM_FORMAT_CHR;
                pp_type.assign("text/plain");
            }
            else
            {
                p_meta_info = (SmlMetInfMetInfPtr_t)p_meta_data->content;
                if ((p_meta_info->format != NULL) &&
                   (p_meta_info->format->length != 0))
                {
                    p_temp_content = (CPCHAR)p_meta_info->format->content;
                    *p_format = DMTree::ConvertFormatStr(p_temp_content);
                }
                else
                {
                    /* If there is no format information, set p_format as default format "chr" */
                    *p_format = SYNCML_DM_FORMAT_CHR;
                }
                /* Set p_temp_type to the passed in type */
                if((p_meta_info->type != NULL) && (p_meta_info->type->length != 0))
                {
                    pp_type.assign((UINT8*)p_meta_info->type->content,p_meta_info->type->length);
                }
                else
                {
                    /* If there is no type information, set the type as 'text/plain' */
                    pp_type.assign("text/plain");
                }
            }
        }
    }
    else
    {
        *p_format = SYNCML_DM_FORMAT_CHR;
        pp_type.assign("text/plain");
    }

    if ( pp_type.getBuffer() == NULL )
        return SYNCML_DM_DEVICE_FULL;
    
    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
Function:    HandleBootstrapChkAddCommand (C function)

Description: When the ADD element is processed from Bootstrap, this callback function
             will be called.

             This function will perform the following operations:
             1) Call the BootstrapObj.SetServerId with the value in the Bootstrap ADD item.

==================================================================================================*/
Ret_t
HandleBootstrapChkAddCommand (InstanceID_t id, 
                              VoidPtr_t    userData,
                              SmlAddPtr_t  pContent)
{
    DMString           *pServerID = (DMString*)userData;
    char               *pTempNodeName = NULL;
    SmlItemListPtr_t    p_add_list_item  = NULL;
    SmlItemPtr_t        p_add_item       = NULL;
    BOOLEAN isDmVer12 = dmTreeObj.IsVersion_12(); 
        
    KCDBG("DMBootstrapSession::HandleBootstrapChkAddCommand: Enter");
      
    /* The Bootstrap message must have a commandId of 1.*/
    if (smlLibStrcmp((const char*)pContent->cmdID->content, "1") != 0)
    {
        smlFreeGeneric((SmlGenericCmdPtr_t)pContent);
        return (SML_ERR_XLT_INVAL_SYNCML_DOC);
    }

    p_add_list_item = pContent->itemList;
    p_add_item      = p_add_list_item->item;

    while (p_add_item != NULL)
    {
#ifdef TNDS_SUPPORT
       DMBuffer            oCommandType;
       SYNCML_DM_FORMAT_T  commandFormat = SYNCML_DM_FORMAT_INVALID;
    
       if ( isDmVer12  )
       {
          BootstrapChkSetMetaData(pContent->meta, oCommandType, &commandFormat);
          KCDBG("DMBootstrapSession:: Command Meta Type: %s, Format : %d", oCommandType.getBuffer(), commandFormat);
   
          if (p_add_item->meta != NULL)
          {
              BootstrapChkSetMetaData(p_add_item->meta, oCommandType, &commandFormat);
              KCDBG("DMBootstrapSession:: Item Meta Type: %s, Format : %d", oCommandType.getBuffer(), commandFormat);
          }

          if ( ((SYNCML_DM_FORMAT_XML == commandFormat) && oCommandType.compare(SYNCML_CONTENT_TYPE_DM_TNDS_XML)) ||
                ((SYNCML_DM_FORMAT_BIN == commandFormat) && oCommandType.compare(SYNCML_CONTENT_TYPE_DM_TNDS_WBXML)) ) 
          {
             KCDBG("DMBootstrapSession:: TNDS Bootstrap");
   
             if ( NULL != p_add_item->data &&
                  NULL != p_add_item->data->content )
             {
                SmlDmTndPtr_t p_tnd_info = (SmlDmTndPtr_t)p_add_item->data->content;
                SmlDmTndNodeListPtr_t p_nodelist = p_tnd_info->nodelist;
                DMString serverID = NULL;
                BootstrapChkTndsServerID(p_nodelist, serverID); 
                if ( NULL == serverID )
                {
                    KCDBG("DMBootstrapSession:: Server ID not found");
                    smlFreeGeneric((SmlGenericCmdPtr_t)pContent);
                    return (SML_ERR_XLT_INVAL_SYNCML_DOC);
                }
                KCDBG("DMBootstrapSession:: Server ID: %s", serverID.c_str());
                (*pServerID) = serverID.c_str();
                smlFreeGeneric((SmlGenericCmdPtr_t)pContent);
                return SML_ERR_OK;
             }
             else   
             {
                KCDBG("DMBootstrapSession:: SML_ERR_XLT_INVAL_SYNCML_DOC");
                if ( NULL == p_add_item )
                {
                    KCDBG("DMBootstrapSession:: p_add_item is NULL");
                }
                else if ( NULL == p_add_item->data )
                {
                    KCDBG("DMBootstrapSession:: p_add_item->data is NULL");
                }
                else if ( NULL == p_add_item->data->content )
                {
                    KCDBG("DMBootstrapSession:: p_add_item->data->content is NULL");
                }
                smlFreeGeneric((SmlGenericCmdPtr_t)pContent);
                return (SML_ERR_XLT_INVAL_SYNCML_DOC);
             }
          }
       }
#endif

        KCDBG("DMBootstrapSession:: Plain Profile Bootstrap");

        /* Find the last '/' in the local URI.*/
        pTempNodeName = (char *)strrchr((const char*)p_add_item->target->locURI->content, '/');
        
        /* If there was no '/', just use the local URI. Other step forward one char past the '/'.*/
        pTempNodeName = (pTempNodeName == NULL) ?
                        (char *)p_add_item->target->locURI->content : pTempNodeName + 1;
                        
        /* Check for the ServerId in the local URI for this item.*/
        if ( isDmVer12 ) 
        {
           if (strcmp(pTempNodeName, DM_SERVERID_1_2) == 0)
           {
              /* Copy the ServerID data.*/
              (*pServerID) = (CPCHAR)p_add_item->data->content;
              break;
           }
        }
        else
        {
           if (strcmp(pTempNodeName, DM_SERVERID_1_1) == 0)
           {
               /* Copy the ServerId data.*/
               (*pServerID) = (CPCHAR)p_add_item->data->content;
               break;
           }
   
        }
           
        /* Move the pointers to the next item in the Add command, if there is a next item.*/
        if (p_add_list_item->next != NULL)
        {
            p_add_list_item = p_add_list_item->next;
            p_add_item = p_add_list_item->item;
        }    
        else
        { 
            p_add_item = NULL;
        }
    } /* End of while */

    smlFreeGeneric((SmlGenericCmdPtr_t)pContent);

    return SML_ERR_OK;
}
