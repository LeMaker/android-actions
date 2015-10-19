#ifndef _DM_TREE_PLUGIN_ROOT_NODE_CLASS_H
#define _DM_TREE_PLUGIN_ROOT_NODE_CLASS_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif
//------------------------------------------------------------------------
//  General Description: This file contains the definition of 
//                       DMPluginRootNode class
//------------------------------------------------------------------------
// Revision History:
//    Author           Modification     Tracking        Description 
//  (core ID)             Date          Number         of Changes
// -----------------------------------------------------------------------
//                        2003-2007                      refactoring 
// cdp180              03/16/2007    LIBll55345   Removing ACL check for internal calls                                  
//------------------------------------------------------------------------
//                       INCLUDE FILES
//------------------------------------------------------------------------
#include "dm_tree_node_class.H"     
#include "dmt.hpp"                     
#include "dmPlugin.h"                  
#include "dmMetaDataNode.h" 

//------------------------------------------------------------------------
//                        CONSTANTS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                         MACROS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                          ENUMS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                 STRUCTURES AND OTHER TYPEDEFS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                  GLOBAL VARIABLE DECLARATIONS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                    FUNCTION PROTOTYPES
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                          CLASS
//------------------------------------------------------------------------

//class Definition for Default interior nodes
//Only DM_XXX methods are implemented in this class.
// The Plug-in MUST implement functions for 
//    Initialization,
//    CreateObject callback,
//    SessionEndcallback and 
//    GetClassUUID methods.

class DMPluginRootNode:public DMNode
{
public:   
   //default constructor
   DMPluginRootNode( PDMPlugin pPlugin );
 
   //destructor of the class
   virtual ~DMPluginRootNode(); 
           
   //function to add an INTERIOR node for which no plug-in 
   //registered
   virtual SYNCML_DM_RET_STATUS_T Add (DMAddData & oAddData);
     
   //To delete a node's data :COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T Delete(CPCHAR pURI);

   //To get node's data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T Get(CPCHAR pURI, DMGetData & oReturnData);

   virtual SYNCML_DM_RET_STATUS_T Find(CPCHAR pbUri);

   virtual SYNCML_DM_RET_STATUS_T SetAddedNode(CPCHAR pbUri);

   virtual SYNCML_DM_RET_STATUS_T RemoveAddedNode(CPCHAR pbUri);
                
   //To get node's format of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetFormat(CPCHAR pURI, SYNCML_DM_FORMAT_T *pdwRetPropertyData);

   //To get node's type of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetType(CPCHAR pURI, DMString& strType);

   //To get node's size of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetSize (CPCHAR pURI,UINT32 * pdwRetPropertyData);

   //To update node's new name in database: COMMAND NOT ALLOWED 
   //on interior node
   virtual SYNCML_DM_RET_STATUS_T Rename(CPCHAR pURI, CPCHAR psNewNodeName);

   //To replace node's size of data: COMMAND NOT ALLOWED 
   //on interior node
   virtual SYNCML_DM_RET_STATUS_T Replace(DMAddData & oReplace);

   // To rollback the operation,not supported in Phase 1
   virtual SYNCML_DM_RET_STATUS_T Rollback (SYNCML_DM_COMMAND_T bDMCommand, CPCHAR pURI);               

   //Work on Multiple Nodes 
   virtual SYNCML_DM_RET_STATUS_T GetName(CPCHAR pURI, DMString& strName );
       
   virtual SYNCML_DM_RET_STATUS_T SetName(CPCHAR pURI, CPCHAR pbNewName);
    
   virtual SYNCML_DM_RET_STATUS_T GetTitle(CPCHAR pURI,DMString& ppbTitle);
      
   virtual SYNCML_DM_RET_STATUS_T SetTitle(CPCHAR pURI, CPCHAR pbNewTitle);
#ifdef LOB_SUPPORT
   virtual SYNCML_DM_RET_STATUS_T  IsESN(CPCHAR pbUri, BOOLEAN& bESN);
#endif                
   SYNCML_DM_RET_STATUS_T GetTree(PDmtAPIPluginTree & ptrTree); 
   
   inline PDMPlugin GetPlugin() { return m_ptrPlugin;}

   CPCHAR GetPluginURI(CPCHAR pURI);

   virtual BOOLEAN  IsGetAccess(CPCHAR pURI) const;

#ifndef DM_IGNORE_TSTAMP_AND_VERSION

   //TStamp and VerNo are AutoSet.
   virtual XPL_CLK_CLOCK_T GetTStamp(CPCHAR pURI);

   virtual SYNCML_DM_RET_STATUS_T SetTStamp(CPCHAR /*pURI*/, XPL_CLK_CLOCK_T /*timeStamp*/)
   { 
      return SYNCML_DM_FEATURE_NOT_SUPPORTED; 
   }

   virtual UINT16 GetVerNo(CPCHAR pURI);

   virtual SYNCML_DM_RET_STATUS_T SetVerNo(CPCHAR /*pURI*/, UINT16 /*wVerNo*/)
   { 
      return SYNCML_DM_FEATURE_NOT_SUPPORTED; 
   }
  
#endif

  private:
   virtual SYNCML_DM_RET_STATUS_T GetTree();


   PDMPlugin m_ptrPlugin;
   PDmtAPIPluginTree m_ptrPluginTree;

};



class DMOverlayDataPluginNode:public DMPluginRootNode
{
public:   
   //default constructor
   DMOverlayDataPluginNode( PDMPlugin pPlugin );


   virtual SYNCML_DM_RET_STATUS_T GetFormat(CPCHAR pURI, SYNCML_DM_FORMAT_T *pdwRetPropertyData);

   inline void SetNodeAttributes(  SYNCML_DM_ACCESS_TYPE_T  wAccessType,
      SYNCML_DM_FORMAT_T nNodeFormat ) 
    {
      m_wAccessType = wAccessType;
      m_nNodeFormat = nNodeFormat;
    }


private:
    SYNCML_DM_ACCESS_TYPE_T  m_wAccessType;
    SYNCML_DM_FORMAT_T m_nNodeFormat;
   
};

//------------------------------------------------------------------------
#endif //_DM_TREE_PLUGIN_ROOT_NODE_CLASS_H
