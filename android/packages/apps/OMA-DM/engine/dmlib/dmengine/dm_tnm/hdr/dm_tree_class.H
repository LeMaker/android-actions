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

#ifndef _DM_TREE_CLASS_H
#define _DM_TREE_CLASS_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

//------------------------------------------------------------------------
//            Header Name: dm_tree_class.H
//
//  General Description: This file contains prototypes, data type
//                       definitions and constants required to access DMTNM
//                       API's and the definition of DMTree class
//------------------------------------------------------------------------

#include "dm_tree_plugin_root_node_class.H" 
#include "dmtPlugin.hpp"
#include "dmvector.h"
#include "dmdefs.h"
#include "dmMetaDataManager.h"
#include "dmPluginManager.h"
#include "SyncML_DM_Archiver.H"
#include "file_manager.h"
#include "dmLockCtxMgr.h"
#include "dmACLManager.h"
#include "dmSubscriptionManager.h"
#include "SYNCML_DM_TreeMount.H"

#ifdef LOB_SUPPORT
#include "dm_tree_default_ESN_class.H" //header file for class defn
#endif


//------------------------------------------------------------------------
//                        CONSTANTS
//------------------------------------------------------------------------


#define REPLACE_CMD_LENGTH_IN_ACL     8
#define GET_CMD_LENGTH_IN_ACL         4
#define ADD_CMD_LENGTH_IN_ACL         4
#define DELETE_CMD_LENGTH_IN_ACL      7
#define EXEC_CMD_LENGTH_IN_ACL        5

#define REPLACE_CMD_IN_ACL   "Replace="
#define GET_CMD_IN_ACL       "Get="
#define ADD_CMD_IN_ACL       "Add="
#define DELETE_CMD_IN_ACL    "Delete="
#define EXEC_CMD_IN_ACL      "Exec="

#define SYNCML_DM_PROP         ("?prop=")
#define SYNCML_DM_LIST         ("?list=")
#define SYNCML_DM_STRUCT       ("Struct")
#define SYNCML_DM_STRUCT_DATA  ("StructData")
#define SYNCML_DM_TNDS         ("TNDS")

#define SYNCML_DM_PROP_LENGTH  6
#define SYNCML_DM_LIST_LENGTH  6

#define SYNCML_DM_NULL_STRING        ("null")
#define SYNCML_DM_NULL_STRING_LENGTH 4

#define FORMAT_LENGTH 10

#define SIZE_LENGTH   10
#define VERSION_LENGTH 5
#define TSTAMP_LENGTH 20

#define FIRST_SERVER_INDEX_IN_LOOKUP_TABLE   1
#define MAX_SERVER_INDEX_IN_LOOKUP_TABLE     (SYNCML_DM_MAX_SERVER_ALLOWED)

#define ROOT_NODE_NAME  (".")
#define ROOT_NODE_ACL   ("Add=*&Replace=*&Get=*&Delete=*")  //YXU

#define SYNCML_DM_ASCII_EXCLAMATION ('!')
#define SYNCML_DM_ASCII_DOLLAR      ('$')
#define SYNCML_DM_ASCII_MINUS       ('-')
#define SYNCML_DM_ASCII_ZERO        ('0')
#define SYNCML_DM_ASCII_NINE        ('9')
#define SYNCML_DM_ASCII_EQUAL_TO    ('=')
#define SYNCML_DM_ASCII_ATTHERATE   ('@')
#define SYNCML_DM_ASCII_CAPITAL_A   ('A')
#define SYNCML_DM_ASCII_CAPITAL_D   ('D')
#define SYNCML_DM_ASCII_CAPITAL_E   ('E')
#define SYNCML_DM_ASCII_CAPITAL_F   ('F')
#define SYNCML_DM_ASCII_CAPITAL_G   ('G')
#define SYNCML_DM_ASCII_CAPITAL_N   ('N')
#define SYNCML_DM_ASCII_CAPITAL_R   ('R')
#define SYNCML_DM_ASCII_CAPITAL_S   ('S')
#define SYNCML_DM_ASCII_CAPITAL_V   ('V')
#define SYNCML_DM_ASCII_CAPITAL_Z   ('Z')
#define SYNCML_DM_ASCII_UNDERSCORE  ('_')
#define SYNCML_DM_ASCII_80         0x80
#define SYNCML_DM_ASCII_FF         0xFF

#define SYNCML_DM_ASCII_SMALL_A     ('a')
#define SYNCML_DM_ASCII_SMALL_F     ('f')
#define SYNCML_DM_ASCII_SMALL_Z     ('z')
#define SYNCML_DM_ASCII_TILDE       ('~')

#define SYNCML_DM_ASCII_CAPITAL_CL   ("CL")
#define SYNCML_DM_AME      ("ame")
#define SYNCML_DM_ORMAT    ("ormat")
#define SYNCML_DM_IZE      ("ize")
#define SYNCML_DM_ERNO     ("erNo")
#define SYNCML_DM_EPLACE   ("eplace")
#define SYNCML_DM_ET       ("et")
#define SYNCML_DM_DD       ("dd")
#define SYNCML_DM_ELETE    ("elete")
#define SYNCML_DM_XEC      ("xec")
#define SYNCML_DM_TITLE    ("Title")
#define SYNCML_DM_TYPE     ("Type")
#define SYNCML_DM_TSTAMP   ("TStamp")

#define SYNCML_DM_SEG_LEN_AFTER_PERCENTAGE (2)
#define SYNCML_DMTNM_MAX_ENCODED_ACL_LENGTH (48)
#define SYNCML_DM_RELATIVE_URI_OFFSET_LENGTH (2)

//#define DM_ISP_LOCK  "/data/dm/tmp/isp"
#define DM_ISP_LOCK  "/data/data/com.android.omadm.service/files/dm/tmp/isp"

//------------------------------------------------------------------------
//                 STRUCTURES AND OTHER TYPEDEFS
//------------------------------------------------------------------------
struct LIST_STRUCT_OR_STRUCT_DATA_INFO_T
{
  LIST_STRUCT_OR_STRUCT_DATA_INFO_T() {Clear();}

  void Clear()  { 
    psCurrent = NULL; 
  }
  DMNode *psCurrent;
};

//------------------------------------------------------------------------
//                  GLOBAL VARIABLE DECLARATIONS
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                    FUNCTION PROTOTYPES
//------------------------------------------------------------------------
//------------------------------------------------------------------------
//                          CLASS
//------------------------------------------------------------------------

//Definition of DMTree class
class DMTree
{
 
  enum {
     CLEAN = -1,
     DIRTY = 0,
     REPLACED = 1,
     ADDED =2,
     DELETED = 1
  };

public:
    friend class DMTreeCacheCurrentTime;
    //Constructor
    DMTree();
      
    //Destructor
    ~DMTree();
       
    //Overloading new and delete operators
    inline void* operator new(size_t dwSize)
    {
       return (DmAllocMem(dwSize));
    }

    inline void operator delete(void *pvBuf)
    {
       DmFreeMem(pvBuf);
    }

     
    //Method that UA will call to let DMTNM initialize data members 
    //that should not be set/reset through constructor
    SYNCML_DM_RET_STATUS_T Init();

    SYNCML_DM_RET_STATUS_T DeInit(BOOLEAN bIsDestructor);
         
    //Method to Initiallize and get first item from list
    SYNCML_DM_RET_STATUS_T InitListAndGetListFirstItem(CPCHAR pbURI,
                                                          SYNCML_DM_GET_ON_LIST_T bGetOnList,
                                                          SYNCML_DM_GET_ON_LIST_RET_DATA_T& ppsReturnData);
    //Method to  get next item from list
    SYNCML_DM_RET_STATUS_T GetListNextItem(SYNCML_DM_GET_ON_LIST_RET_DATA_T& ppsReturnData);

    //method to initialise the list for serialization process
    SYNCML_DM_RET_STATUS_T InitSerializationList(DMNode * serializeRoot);

    //method to return properties for serialization one by one
    SYNCML_DM_SERIALIZATION_STATUS_T GetSerializationListNextItem(DMNode **ppsRetNode, 
                                                                      INT32& nEndTagsNumber );   


    //Method to get the name of the Parent node when the value of
    //the child node is given
    BOOLEAN GetParentOfKeyValue(CPCHAR pbvalueOfKey,
                                  CPCHAR pKey,
                                  CPCHAR pSubtreeURI,
                                  DMString& strResult);

    //To add a node
    SYNCML_DM_RET_STATUS_T Add( DMAddData & oAddData, 
                                                     SYNCML_DM_REQUEST_TYPE_T eRequestType );

    //To delete a node/subtree
    SYNCML_DM_RET_STATUS_T Delete(CPCHAR pbURI,
                                                         SYNCML_DM_REQUEST_TYPE_T eRequestType);

    //To exec a URI
    SYNCML_DM_RET_STATUS_T Exec(CPCHAR pbUri,
                                 CPCHAR pExecData,
                                 DMString & oExecResult,
                                 CPCHAR szCorrelator = NULL ); 

    //To get value of a node
   SYNCML_DM_RET_STATUS_T Get(CPCHAR pbUri, 
                                                   DMGetData & oReturnData, 
                                                   SYNCML_DM_REQUEST_TYPE_T eRequestType);


    //To get attributes of a node
    SYNCML_DM_RET_STATUS_T GetAttributes(CPCHAR pbUri,
                                                                      DmtAttributes& attrs,
                                                                      SYNCML_DM_REQUEST_TYPE_T eRequestType);


    //To replace a node's value/property value
    SYNCML_DM_RET_STATUS_T Replace( DMAddData & oReplaceData, 
                                                             SYNCML_DM_REQUEST_TYPE_T eRequestType );

    //To set links of a de-serialized node
    SYNCML_DM_RET_STATUS_T AddNode(DMNode **psNodeObject,
                                    const DMAddNodeProp & oAddNodeProperties,
                                    DMNode* pNewNode = NULL );

    //Invoked by Archiver to get a pointer to the root node 
    //Archiver then uses this object to traverse the tree
    //(using different traversal mechanism)
    inline DMNode* GetRootNode() { return( m_psRoot); }


    //To validate and Parse URI
    SYNCML_DM_URI_RESULT_T URIValidateAndParse(CPCHAR pbUri) const;

    //To decode the ACL to get back actual server names
    static SYNCML_DM_RET_STATUS_T DecodeACL(const UINT8 *pbEncodedACL,
                                                UINT8 **ppbDecodedACL,
                                                UINT16 *pwDecodedACLLength);

    //Detach the node from tree
    void DetachNodeFromTree(DMNode *);

    //Move to public as it may be used by notification manager
    //This method will return a pointer to the node(which is the 
    //last segment in the URI)
    DMNode* FindNodeByURI(CPCHAR pURI);
         
    SYNCML_DM_RET_STATUS_T  getChildren( CPCHAR uri, 
                                           DMMap<DMString, UINT32>& childrenMap, 
                                           DMTNM_NODE_TYPE nodeType,
                                           SYNCML_DM_REQUEST_TYPE_T eRequestType);
    
    SYNCML_DM_RET_STATUS_T  getLeafChildren( CPCHAR uri,
                                                DMMap<DMString, UINT32>& childrenMap,
                                                SYNCML_DM_REQUEST_TYPE_T eRequestType);        
    
    SYNCML_DM_RET_STATUS_T  getInteriorChildren( CPCHAR uri, 
                                                   DMMap<DMString, UINT32>& childrenMap,
                                                   SYNCML_DM_REQUEST_TYPE_T eRequestType);        
    
    SYNCML_DM_RET_STATUS_T  setLeafChildren( CPCHAR uri, 
                                                DMMap<DMString, UINT32>& childrenMap );
    
    SYNCML_DM_RET_STATUS_T  rollback( DMMap<DMString, UINT32> oldChildrenMap, 
                                       DMMap<DMString, UINT32>& newChildrenMap, 
                                       INT8 actionList[], 
                                       INT8 deleteList[] );
    
    SYNCML_DM_RET_STATUS_T  handleNewChildren( DMMap<DMString, UINT32> oldChildrenMap, 
                                                    DMMap<DMString, UINT32>& newChildrenMap, 
                                                    INT8 actionList[],
                                                    INT8 deleteList[] );   
    
    SYNCML_DM_RET_STATUS_T  handleOldChildren( DMMap<DMString, UINT32> oldChildrenMap, 
                                                   DMMap<DMString, UINT32>& newChildrenMap, 
                                                   INT8 actionList[],
                                                   INT8 deleteList[] );                                                        
                                            
    // return the associated Archive that the node is in.
    SyncML_DM_Archive * GetArchive(const DMNode * node);

    /* SessionStart functions call this to set the server Id. */
    SYNCML_DM_RET_STATUS_T SetServerId (CPCHAR p_ServerId);

    /* The DMTNM calls this to get the current server Id. */
    inline CPCHAR GetServerId() const { return (m_strPrincipal.c_str() );}

    /* Set CcnRef. */
    SYNCML_DM_RET_STATUS_T SetConRef (CPCHAR p_ConRef);

    /* Get COnRef */
    inline CPCHAR GetConRef() const { return (m_strConRef.c_str() );}

    // default leaf/interior node creation (except plug-ins)
    static DMNode* CreateNodeObj( SYNCML_DM_FORMAT_T bFormat, BOOLEAN isESN, CPCHAR pbFileName);

    
    DMNode* CreateSkeletonNode( CPCHAR szURI );

    static BOOLEAN GetLastSegmentOfURI( DMString& strURI, DMString& strLastSegment );
    
    void DeleteNodesFromTree(DMNode * startNode);
    
    UINT16 readOneWordFromTree(DMNode * pNode, UINT16 defaultValue);
    
    void UnloadArchive(DMNode * psStartNode );
    
    void  SubstituteNode( DMNode* pOldNode, DMNode* pNewNode );

    inline const DmtOPINodeData* GetCachedOPINodeData() { return &m_oOPICacheData;}
    SYNCML_DM_RET_STATUS_T SetOPINodeData( CPCHAR szURI, const DmtOverlayPluginData& oData );
    
    SYNCML_DM_RET_STATUS_T GetAccNodeValue (CPCHAR pParentName,
                                            CPCHAR pSubNode,
                                            DMGetData & oAccData);

    SYNCML_DM_RET_STATUS_T GetDefAccountAddrInfo(CPCHAR pAccProfileName,
                                                 DMGetData& oAddr,
                                                 DMGetData& oAddrType,
                                                 DMGetData& oPortNbr);

    SYNCML_DM_RET_STATUS_T GetServerAuthInfo(CPCHAR pAccProfileName,
                                             CPCHAR pAuthType,
                                             DMGetData& oAuthName,
                                             DMGetData& oAuthSecret,
                                             DMGetData& oAuthData,
                                             DMString&  oAuthDataUri);

    SYNCML_DM_RET_STATUS_T GetHttpAuthInfo(CPCHAR pAccProfileName,
                                             DMGetData& oAuthType,
                                             DMGetData& oAuthName,
                                             DMGetData& oAuthSecret,
                                             DMGetData& oAuthData,
                                             DMString&  oAuthDataUri);

    SYNCML_DM_RET_STATUS_T GetClientAuthInfo(CPCHAR pAccProfileName,
                                             CPCHAR pAuthType,
                                             DMGetData& oAuthName,
                                             DMGetData& oAuthSecret,
                                             DMGetData& oAuthData,
                                             DMString&  oAuthDataUri,
                                             DMGetData& oAuthType );

    SYNCML_DM_RET_STATUS_T GetAuthInfo(CPCHAR pAccProfileName,
                                       CPCHAR pAuthLevel,
                                       CPCHAR pAuthType,
                                       DMGetData& oAuthType,
                                       DMGetData& oAuthName,
                                       DMGetData& oAuthSecret,
                                       DMGetData& oAuthData,
                                       DMString&  oAuthDataUri);

    SYNCML_DM_RET_STATUS_T GetLastClientAuthType(CPCHAR pAccProfileName,
                                                 DMGetData& oClientAuthType);

    SYNCML_DM_RET_STATUS_T SetLastClientAuthType(CPCHAR pAccProfileName,
                                                 CPCHAR pClientAuthType);


    // Utility method used to convert SYNCML_DM_FORMAT_T type format to string
    static SYNCML_DM_RET_STATUS_T ConvertFormat(SYNCML_DM_FORMAT_T format, 
                                  DMString & strFormat);

    // Utility method used to convert string format to SYNCML_DM_FORMAT_T type
    static SYNCML_DM_FORMAT_T ConvertFormatStr(const DMString& formatStr);

    BOOLEAN IsVersion_12() { return m_bVersion_1_2; }

    // Verify if URI is blocked - feature isn't available
    BOOLEAN IsUriEnabled(CPCHAR szURI)	const;

#ifdef LOB_SUPPORT
    SYNCML_DM_RET_STATUS_T IsESN(CPCHAR pbURI, BOOLEAN &isESN);
#endif
    DMMetaDataManager & GetMetaDataManager();
    DMPluginManager &  GetPluginManager();
    SyncML_DM_Archiver &  GetArchiver();
    DMLockContextManager &  GetLockContextManager();
    DMSubscriptionManager &  GetSubscriptionManager(); 

    void CheckMemoryAging();
    SYNCML_DM_RET_STATUS_T ReleaseLock( INT32 nLockID ,SYNCML_DM_COMMAND_T command = SYNCML_DM_RELEASE );
    SYNCML_DM_RET_STATUS_T SaveFile(SYNCML_DM_FILE_TYPE_T eFileType);
    SYNCML_DM_RET_STATUS_T RevertFile(SYNCML_DM_FILE_TYPE_T eFileType);
    SYNCML_DM_RET_STATUS_T Flush();
    void  GetTreeMountEntry (CPCHAR& p_Uri,  
                                         CPCHAR& p_TreePath,  
                                          UINT16  index ) const;

    SYNCML_DM_RET_STATUS_T RecoverPlugin();
    CPCHAR                 GetWritableFileSystemFullPath( DMString & path );
    BOOLEAN                IsInitialized() const;
    UINT16                 GetMaxPathDepth() const;
    UINT16                 GetMaxTotalPathLength() const;
    UINT16                 GetMaxPathSegmentLength() const;

   
private:
    //FOR getStruct to free the list
    inline void FreeListStruct() {  psListInfo.Clear(); }


    //To find the next sibling  while traversing the child list
    DMNode* FindNodeInNextSiblingsList(DMNode *psFindNode,
                                            CPCHAR pbURISegment) const;

      
    //To insert a newly added child in the Child list
    SYNCML_DM_RET_STATUS_T InsertNodeIntoNextSiblingsList(DMNode *psNextSiblingStartNode,
                                                          DMNode *psInsertNode);

    BOOLEAN ParentExistsForPluginNode( CPCHAR pURI, DMNode *psNode) const;

    //To parse the ACL string and replace with server Indices
    SYNCML_DM_RET_STATUS_T ParseACL(UINT8* pACL);
     

    SYNCML_DM_RET_STATUS_T CheckDeleteForNode(DMNode *psDeletingNode,
                                                                        CPCHAR pbURI);

    SYNCML_DM_RET_STATUS_T DeleteNode(DMNode *psDeletingNode,
                                      CPCHAR pbURI,
                                      DMStringVector & aDeletedChildren);

    //To check if the server is allowed to do the DM operation
    SYNCML_DM_RET_STATUS_T IsValidServer(CPCHAR pURI,
                                         SYNCML_DM_ACCESS_TYPE_T accessType,
                                         SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                         BOOLEAN bCheckAccess,
                                         BOOLEAN bIsCheckLocal,
                                         DMMetaPCharVector* pChildDependNodes = NULL);

    //To get all the child names for GET on Interior node
    SYNCML_DM_RET_STATUS_T GetChildren( CPCHAR szURI, 
                                      const DMNode *psNode, 
                                      SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                      DMString& strChildren ) const; 

    //To get number of child on Interior node
    UINT16 GetChildrenCount(const DMNode *psNode) const; 
       
    //validates the segment (grammar of node name) 
    BOOLEAN IsValidSegment(CPCHAR pbSegment, UINT8 bSegmentLength);

    SYNCML_DM_RET_STATUS_T CheckDeleteForPluginNodes(SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                                                                 CPCHAR pbURI, 
                                                                                 DMNode *psProxyNode,
                                                                                 DMStringVector & aChildren);
       
    SYNCML_DM_RET_STATUS_T CheckDeleteForPluginNode(SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                                                                CPCHAR pbURI, 
                                                                                PDmtNode ptrNode, 
                                                                                DMStringVector & aChildren,
                                                                                BOOLEAN bIsParent);
    
    BOOLEAN GetPluginURI(CPCHAR szURI, DMString& strURI, DMString& strShortURI ) const;
    
    BOOLEAN LoadSkeletonParentArchive( DMNode* pNode );
    
    SYNCML_DM_RET_STATUS_T GetListItemData(SYNCML_DM_GET_ON_LIST_RET_DATA_T& ppsReturnData);

    DMNode*  GetOPINode( CPCHAR szURI );
    
    void  ResetOPICache();
    SYNCML_DM_RET_STATUS_T  OnOPiDelete( CPCHAR szURI );
    SYNCML_DM_RET_STATUS_T  OnOPiAdd( CPCHAR szURI, DmtOverlayPluginData& data );
    void  CheckOpiSync( DMNode* pNode, char* szURI, const char* szLastSegment );
    void  OpiSync( DMNode* pNode, const char* szURI );
    void  CheckForIndirectUpdates( CPCHAR szURI, const DMMetaPCharVector& asChildDepend, DMNode* inNode );
    void  CheckURIForIndirectUpdates( CPCHAR szChild, CPCHAR szURI, DMNode* inNode );
#ifdef LOB_SUPPORT
    void  ResetESNCache();
    void RemoveESNCache(CPCHAR szURI );
    DMNode*  GetESN( CPCHAR szURI );
    void  SetESNCache( CPCHAR szURI , DMNode* pESN);
   void   LogDeleteForESN(DMNode *psDeletingNode);
#endif
    void                   LoadMaxValues();

    SYNCML_DM_RET_STATUS_T InternalInit();
    void                   InternalDeInit();
    SYNCML_DM_RET_STATUS_T RecoverPluginFromFile( const DMString&  file_bak_name );
    SYNCML_DM_RET_STATUS_T ReadCommandFromFile( DMFileHandler    *fileHandle,
                                                DMBuffer&        cmdURI );

    BOOLEAN NeedCheckParent( const DMString&  strURI,
                             DMNode *psReplacingNode,
                             BOOLEAN bInPlugin,
                             SYNCML_DM_FORMAT_T  bFormat,
                             SYNCML_DM_REQUEST_TYPE_T eRequestType);

    SYNCML_DM_RET_STATUS_T ReplaceACLProperty( const DMString&  strURI,
                                               BOOLEAN          bInPlugin, 
                                               const DMString&  strPluginURI, 
                                               DMAddData&       oReplaceData,
                                               DMNode*          psReplacingNode );

    SYNCML_DM_RET_STATUS_T ReplaceTitleProperty( const DMString&  strURI,
                                                 BOOLEAN          bInPlugin, 
                                                 const DMString&  strPluginURI, 
                                                 DMAddData&       oReplaceData,
                                                 DMNode*          psReplacingNode );

    SYNCML_DM_RET_STATUS_T AddNodeInternal( DMAddData & oAddData, 
                                            SYNCML_DM_REQUEST_TYPE_T eRequestType );

    SYNCML_DM_RET_STATUS_T ReplaceNodeInternal( const DMString&        strURI,
                                                DMAddData&             oReplaceData,
                                                DMNode*                psReplacingNode,
                                                DMMetaPCharVector&     asChildDepend );

    SYNCML_DM_RET_STATUS_T PrepareNamePropertyReplace( const DMString&        strURI,
                                                       BOOLEAN                bInPlugin, 
                                                       DMAddData&             oReplaceData,
                                                       DMNode*                psReplacingNode );

    SYNCML_DM_RET_STATUS_T CheckReplacingNodeName( DMAddData& oReplaceData,
                                                   DMNode*    psReplacingNode );

    SYNCML_DM_RET_STATUS_T ReplaceProperty( const DMString&         strURI,
                                            BOOLEAN                bInPlugin, 
                                            const DMString&        strPluginURI, 
                                            DMAddData&             oReplaceData,
                                            DMNode*                psReplacingNode,
                                            SYNCML_DM_RET_STATUS_T wURIValidateRetCode );

    SYNCML_DM_RET_STATUS_T ReplaceNameProperty( const DMString&         strURI,
                                                 BOOLEAN                bInPlugin, 
                                                 const DMString&        strPluginURI, 
                                                 DMAddData&             oReplaceData,
                                                 DMNode*                psReplacingNode,
                                                 SYNCML_DM_RET_STATUS_T wURIValidateRetCode );

    SYNCML_DM_RET_STATUS_T FixACL( const DMString& strURI,
                                   DMAddData&      oReplaceData );

    SYNCML_DM_RET_STATUS_T SetACL(CPCHAR szURI);	

    BOOLEAN VerifyArchiveReadAccess(CPCHAR szURI);
    BOOLEAN VerifyArchiveWriteAccess(CPCHAR szURI);

    SYNCML_DM_RET_STATUS_T AddAutoNodes(CPCHAR szURI, 
                                    SYNCML_DM_REQUEST_TYPE_T eRequestType,
                                    DMToken & oAutoNodes);

    SYNCML_DM_RET_STATUS_T LogEvent(SYNCML_DM_COMMAND_T type,
                                    CPCHAR pbURI,
                                    CPCHAR szNewName = NULL);	

    SYNCML_DM_RET_STATUS_T LogEvent(CPCHAR pbURI,
                                    const DMStringVector & aDeletedChildren);

#ifdef LOB_SUPPORT
    SYNCML_DM_RET_STATUS_T  LogESNCommandForArchiver(CPCHAR pbURI,
                                                      DMNode * inNode);
#endif

private:
#ifndef DM_NO_LOCKING    
  DMCriticalSection m_csInitLock;
#endif  

  SYNCML_DM_TreeMount       m_oTreeMountObj;
  mutable DMMetaDataManager m_oMDFObj;
  DMPluginManager             m_oPluginManager;
  SyncML_DM_Archiver        m_oArchiver;
  CMultipleFileManager      m_oFileManager;
  DMLockContextManager       m_oLockContextManager;
  DMAclManager              m_oACLObj;
  DMSubscriptionManager          m_oEvtObj;
  CEnv                      m_oEnv;
  SYNCML_DM_RET_STATUS_T    m_init_status;  

  DMString                  m_strPrincipal; /* Server ID */
  DMString			m_strConRef;

  DMNode                    *m_psRoot;   //The root node object

  // the following 3 members are used for OPI support and cached by "FindNode" function
  DmtOPINodeData        m_oOPICacheData;
  PDMPlugin                  m_ptrCacheOPI;
  DMOverlayDataPluginNode   *m_pOPINode;
  BOOLEAN                   m_bVersion_1_2;	
    
#ifdef LOB_SUPPORT
    DMNode                  *m_pESN;   //The External Storage Node
    DMString                m_strESNPath; //URI of the ESN
#endif

  LIST_STRUCT_OR_STRUCT_DATA_INFO_T psListInfo;
  UINT16  m_wMaxDepth;
  UINT16  m_wMaxTotLen;
  UINT16  m_wMaxSegLen;
  INT32    m_nRefCount; // number of users

  static const UINT8        *m_pDataFormatTable[];
  XPL_CLK_CLOCK_T           m_currentTime; // cached current time value
};

//Definition of DMTreeLastAccessedTime class
//can be used to cache system call to XPL_CLK_GetClock, 
//which may occur quite frequently to update LastAccessedTime of archives
class DMTreeCacheCurrentTime
{
	BOOLEAN m_resetCacheOnDestruct;
	DMTree* m_tree;

public:
	DMTreeCacheCurrentTime(DMTree* tree)
	{
		m_resetCacheOnDestruct = (tree->m_currentTime==0);
		m_tree = tree;
		if (m_resetCacheOnDestruct)
			tree->m_currentTime = XPL_CLK_GetClock();	

	}
	~DMTreeCacheCurrentTime()
	{
		if (m_resetCacheOnDestruct)
			m_tree->m_currentTime = 0;
	}
};

#ifdef __cplusplus
extern "C" {
#endif
extern DMTree               dmTreeObj;
#ifdef __cplusplus
}
#endif

//---------------------------------------------------------------------
#endif  // _DM_TREE_CLASS_H
