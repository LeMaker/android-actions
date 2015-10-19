#ifndef _DM_TREE_DEFAULT_ESN_CLASS_H
#define _DM_TREE_DEFAULT_ESN_CLASS_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

//------------------------------------------------------------------------
//            Header Name: dm_tree_default_ESN_class.H
//
//  General Description: This file contains the definition of 
//                       DMDefaultESN class
//------------------------------------------------------------------------
// Revision History:
//    Author           Modification     Tracking        Description 
//                        2003-2007                      refactoring
// c23495             11/29/2006     libgg67059  LOB support
// cdp180              03/16/2007    LIBll55345   Removing ACL check for internal calls                                  
//
// -----------------   ------------    ----------   ----------------------

//------------------------------------------------------------------------
//                       INCLUDE FILES
//------------------------------------------------------------------------
#include "dm_tree_default_leaf_node_class.H" //header file for class defn
#include "SyncML_DM_FileHandle.H"
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

//class Definition for Default External Storage Nodes
//IMPORTANT NOTE: A plug-in that supports Interior nodes can derive from
//this class.Only DM_XXX methods are implemented in this class.
// The Plug-in MUST implement functions for Initialization,CreateObject 
//callback,SessionEndcallback and GetClassUUID methods.

class DMDefaultESN:public DMDefaultLeafNode
{
public:   
   //default constructor
   DMDefaultESN(CPCHAR pbFileName);
   ~DMDefaultESN();
      
   //function to add an INTERIOR node for which no plug-in 
   //registered
   virtual SYNCML_DM_RET_STATUS_T Add(DMAddData & oAddData);
     
   //To delete a node's data :COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T Delete(CPCHAR pbUri);
    
   //To get node's data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T Get(CPCHAR pbUri, DMGetData & oReturnData);
                
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
   virtual SYNCML_DM_RET_STATUS_T Rollback();               

   virtual SYNCML_DM_RET_STATUS_T Commit();

    BOOLEAN IsSetComplete() const { return m_bSetComplete;}
    BOOLEAN IsDirty() const { return m_bDirty;}
    BOOLEAN NeedLogging() const { return m_bNeedLogging;}
   CPCHAR GetInternalStorageFileName(void) const;
   CPCHAR GetOriginalInternalFileName(void) const;
   SYNCML_DM_RET_STATUS_T OpenInternalStorageFile();  
   SYNCML_DM_RET_STATUS_T CloseInternalFile(void);  
   SYNCML_DM_RET_STATUS_T GetFirstChunk(DmtDataChunk&  chunkData); 
   SYNCML_DM_RET_STATUS_T GetNextChunk(DmtDataChunk& chunkData); 
   SYNCML_DM_RET_STATUS_T SetFirstChunk(DmtDataChunk& chunkData);  
   SYNCML_DM_RET_STATUS_T SetNextChunk(DmtDataChunk& chunkData);  
   SYNCML_DM_RET_STATUS_T SetLastChunk(DmtDataChunk& chunkData);  

private:
   DMString abStorageName;
   DMString abOriginalName;
   UINT32 totalSize;
   BOOLEAN  m_bSetComplete;
   BOOLEAN  m_bDirty;
   DMFileHandler *fileHandle;
   UINT32 offset; 
   BOOLEAN  m_bNeedLogging;


};
//------------------------------------------------------------------------
#endif //_DM_TREE_DEFAULT_ESN_CLASS_H


