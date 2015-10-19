#ifndef _DM_TREE_DEFAULT_LEAF_NODE_CLASS_H
#define _DM_TREE_DEFAULT_LEAF_NODE_CLASS_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

//------------------------------------------------------------------------
//            Header Name: dm_tree_default_interior_node_class.H
//
//  General Description: This file contains the definition of 
//                       DMDefaultLeafNode class
//------------------------------------------------------------------------
// Revision History:
//    Author           Modification     Tracking        Description 
//  (core ID)             Date          Number         of Changes
// ----------------------------------------------------------------------
//                        2003-2007                      refactoring 
// cdp180              03/16/2007    LIBll55345   Removing ACL check for internal calls                                  
//
//------------------------------------------------------------------------
//                       INCLUDE FILES
//------------------------------------------------------------------------
#include "dm_tree_node_class.H"     //for DMNode class defn
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
//IMPORTANT NOTE: A plug-in that supports Interior nodes can derive from
//this class.Only DM_XXX methods are implemented in this class.
// The Plug-in MUST implement functions for Initialization,CreateObject 
//callback,SessionEndcallback and GetClassUUID methods.

class DMDefaultLeafNode:public DMNode
{
   DMBuffer psData;

public:   
   //default constructor
   DMDefaultLeafNode():DMNode(FALSE){};
      
   //function to add an INTERIOR node for which no plug-in 
   //registered
   virtual SYNCML_DM_RET_STATUS_T Add(DMAddData & oAddData);
     
   //To delete a node's data :COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T Delete(CPCHAR pbUri);
    
   //To get node's data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T Get(CPCHAR pbUri, DMGetData & oReturnData);

   virtual SYNCML_DM_RET_STATUS_T Find(CPCHAR pbUri) { return SYNCML_DM_SUCCESS; }

   virtual SYNCML_DM_RET_STATUS_T SetAddedNode(CPCHAR pbUri) { return SYNCML_DM_SUCCESS; }

   virtual SYNCML_DM_RET_STATUS_T RemoveAddedNode(CPCHAR pbUri) { return SYNCML_DM_SUCCESS; }	
                   
   //To get node's format of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetFormat(CPCHAR pbUri,
                                               SYNCML_DM_FORMAT_T *pdwRetPropertyData);

   //To get node's type of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetType(CPCHAR pbUri, DMString& strType);

   //To get node's size of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetSize(CPCHAR pbUri, UINT32 *pdwRetPropertyData);

   //To update node's new name in database: COMMAND NOT ALLOWED 
   //on interior node
   virtual SYNCML_DM_RET_STATUS_T Rename(CPCHAR pbUri, CPCHAR psNewNodeName);

   //To replace node's size of data: COMMAND NOT ALLOWED 
   //on interior node
   virtual SYNCML_DM_RET_STATUS_T Replace(DMAddData & oReplace);

   // To rollback the operation,not supported in Phase 1
   virtual SYNCML_DM_RET_STATUS_T Rollback(SYNCML_DM_COMMAND_T bDMCommand,
                                             CPCHAR pbUri);               

  
   virtual DMBuffer * getData() { return &psData; }
   virtual const DMBuffer * getData() const { return &psData; }

};
//------------------------------------------------------------------------
#endif //_DM_TREE_DEFAULT_INTERIOR_NODE_CLASS_H
