#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "stdio.h"
#include "dmt.hpp"

#include "jem_defs.hpp"
#include "dmt.hpp"

#include "plugin/dmtRWPlugin.hpp"
#include "SyncML_DM_FileHandle.H"
#include "xpl_File.h"


//---------------------------Declaration----------------------
class testRWPluginTree;
class testRWPluginNode;

typedef JemSmartPtr< testRWPluginTree > PtestRWPluginTree;
typedef JemSmartPtr< testRWPluginNode >PtestRWPluginNode;

// PlugIn Tree
class testRWPluginTree : public DmtRWPluginTree
{

public:
   testRWPluginTree(const char * rootPath);
   ~testRWPluginTree();
    SYNCML_DM_RET_STATUS_T DeleteNode( const char* path );
    SYNCML_DM_RET_STATUS_T CreateInteriorNode( const char* path, PDmtNode& ptrCreatedNode );
    SYNCML_DM_RET_STATUS_T CreateLeafNode( const char* path, PDmtNode& ptrCreatedNode, const DmtData& value,  BOOLEAN isESN);
    SYNCML_DM_RET_STATUS_T CreateInteriorNodeInternal( const char* path, PDmtNode& ptrCreatedNode, const DMStringVector & childNodeNames);
    SYNCML_DM_RET_STATUS_T CreateLeafNodeInternal( const char* path, PDmtNode& ptrCreatedNode, const DmtData& value, BOOLEAN isESN );

};

class testRWPluginNode : public DmtRWPluginNode
{ 
public:
   testRWPluginNode(    //Leaf Node
      PDmtPluginTree ptrTree, 
	   const char* path, 
	   const DmtData & oData,
	   BOOLEAN isESN
   );

   testRWPluginNode(    //Interior Node
      PDmtPluginTree ptrTree, 
	   const char* path, 
	   const DMStringVector & childNodeNames
   );
    SYNCML_DM_RET_STATUS_T GetValue( DmtData& oData ) const;
    SYNCML_DM_RET_STATUS_T SetValue( const DmtData& value );
#ifdef LOB_SUPPORT
    SYNCML_DM_RET_STATUS_T GetFirstChunk(DmtDataChunk&  chunkData); 
    SYNCML_DM_RET_STATUS_T GetNextChunk(DmtDataChunk& chunkData); 
    SYNCML_DM_RET_STATUS_T SetFirstChunk(DmtDataChunk& chunkData);  
    SYNCML_DM_RET_STATUS_T SetNextChunk(DmtDataChunk& chunkData);  
    SYNCML_DM_RET_STATUS_T SetLastChunk(DmtDataChunk& chunkData);  
    SYNCML_DM_RET_STATUS_T Delete();
    SYNCML_DM_RET_STATUS_T Commit();
#endif
protected:
	 ~testRWPluginNode();
  private:
#ifdef LOB_SUPPORT
     SYNCML_DM_RET_STATUS_T SetChunkData(DmtDataChunk&  chunkData); 
     SYNCML_DM_RET_STATUS_T GetChunkData(DmtDataChunk&  chunkData); 
     SYNCML_DM_RET_STATUS_T OpenInternalStorageFile();
     SYNCML_DM_RET_STATUS_T CloseInternalFile();  
	   DMString abStorageName;
	   UINT32 totalSize;
	   DMFileHandler *fileHandle;
	   UINT32 offset; 
#endif
};

testRWPluginTree::testRWPluginTree(const char * rootPath):DmtRWPluginTree()
{
  Init(rootPath);
}
testRWPluginTree::~testRWPluginTree()
{
}

SYNCML_DM_RET_STATUS_T testRWPluginTree::DeleteNode( const char* path )
{
  PDmtErrorDescription e= DmtRWPluginTree::DeleteNode(path);
  return e.GetErrorCode();
}

SYNCML_DM_RET_STATUS_T testRWPluginTree::CreateInteriorNodeInternal( const char* path, PDmtNode& ptrCreatedNode, const DMStringVector & childNodeNames )
{
   PtestRWPluginNode pNode;
//Interior node   
   pNode=new testRWPluginNode(this, 
      path,   //RootNode is interior node
      childNodeNames
   );
   PDmtErrorDescription e = this->SetNode(path, 
      PDmtNode(pNode.GetPtr())
   );
   ptrCreatedNode = pNode;
   return e.GetErrorCode();
}

SYNCML_DM_RET_STATUS_T testRWPluginTree::CreateLeafNodeInternal( const char* path, PDmtNode& ptrCreatedNode, const DmtData& value, BOOLEAN isESN )
{
   PtestRWPluginNode pNode;
//Leaf node   
    pNode=new testRWPluginNode(
               this, 
               path, 
               value,
               isESN
            );
   PDmtErrorDescription e = this->SetNode(path, 
               PDmtNode(pNode.GetPtr())
            );
//  return GetNode( path, ptrCreatedNode );
   ptrCreatedNode = pNode;
   return e.GetErrorCode();

}

SYNCML_DM_RET_STATUS_T testRWPluginTree::CreateInteriorNode( const char* path, PDmtNode& ptrCreatedNode )
{
  return DmtRWPluginTree::CreateInteriorNode(path, ptrCreatedNode);
}
SYNCML_DM_RET_STATUS_T testRWPluginTree::CreateLeafNode( const char* path, PDmtNode& ptrCreatedNode, const DmtData& value,  BOOLEAN isESN )
{
  return  DmtRWPluginTree::CreateLeafNode(path, ptrCreatedNode, value, isESN);

}

/*testRWPluginNode::testRWPluginNode(
      PDmtPluginTree ptrTree, 
      const char* path 
) : DmtRWPluginNode((DmtRWPluginTree *) ((DmtPluginTree *)(ptrTree)), path, DmtData(""))
{
}
*/
testRWPluginNode::testRWPluginNode( 
   PDmtPluginTree ptrTree, 
   const char* path, 
   const DmtData & oData,
   BOOLEAN isESN    
)
{
  Init(ptrTree, path, oData, isESN);
#ifdef LOB_SUPPORT
  abStorageName = NULL;
  totalSize = 0L;
  fileHandle = NULL;
  offset = 0;
#endif
}
testRWPluginNode::testRWPluginNode(    //Interior Node
   PDmtPluginTree ptrTree, 
	const char* path, 
	const DMStringVector & childNodeNames
) 
{
  Init(ptrTree, path, childNodeNames);
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::~destructor
// DESCRIPTION     : 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
testRWPluginNode::~testRWPluginNode()
{
#ifdef LOB_SUPPORT
	CloseInternalFile();
	abStorageName = NULL;
#endif
}
SYNCML_DM_RET_STATUS_T testRWPluginNode::GetValue( DmtData& oData ) const
{
  return  DmtPluginNode::GetValue(oData);
}

SYNCML_DM_RET_STATUS_T testRWPluginNode::SetValue( const DmtData& value )
{
  return DmtRWPluginNode::SetValue(value);
}
#ifdef LOB_SUPPORT
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::GetChunkData
// DESCRIPTION     : Get  chunk data
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::GetChunkData(DmtDataChunk&  chunkData) 
{
	SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
	UINT32 remainlLen = totalSize- offset;
	UINT32 getLen = 0L;
	UINT8 *bufp;

	retStatus = OpenInternalStorageFile();
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;

	if(fileHandle == NULL)
	{
		chunkData.SetChunkData(NULL, 0L);
		chunkData.SetReturnLen(getLen);
		return retStatus;
	}
	chunkData.GetChunkData(&bufp);
	if(remainlLen <0|| bufp == NULL)
		return SYNCML_DM_INVALID_PARAMETER;

	if(remainlLen == 0)
	{
		chunkData.SetChunkData(NULL, 0L);
		chunkData.SetReturnLen(remainlLen);
		CloseInternalFile();
		return SYNCML_DM_SUCCESS;
	}

	getLen = chunkData.GetChunkSize();
	if(getLen > remainlLen)
		getLen = remainlLen;

	chunkData.GetChunkData(&bufp); 	// the chunk data is available 	
	if(bufp == NULL)
		return SYNCML_DM_INVALID_PARAMETER;
    	if(fileHandle->seek(XPL_FS_SEEK_SET, offset) != SYNCML_DM_SUCCESS) 
	        return  SYNCML_DM_IO_FAILURE;
    	if(fileHandle->read(bufp, getLen) != SYNCML_DM_SUCCESS) 
	        return  SYNCML_DM_IO_FAILURE;
	chunkData.SetReturnLen(getLen);
	offset += getLen;

	return retStatus;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::GetFirstChunk
// DESCRIPTION     : Get next chunk
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::GetFirstChunk(DmtDataChunk& chunkData)
{
	SYNCML_DM_RET_STATUS_T retStatus = DmtRWPluginNode::GetFirstChunk(chunkData);
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;
	return GetChunkData(chunkData);
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::GetNextChunk
// DESCRIPTION     : Get next chunk
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::GetNextChunk(DmtDataChunk& chunkData)
{
	SYNCML_DM_RET_STATUS_T retStatus = DmtRWPluginNode::GetNextChunk(chunkData);
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;
	return GetChunkData(chunkData);
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::SetChunkData
// DESCRIPTION     : Set  chunk data for an ESN
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::SetChunkData(DmtDataChunk& chunkData)  
{
	SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
	UINT32 dataLen;
	UINT8 *bufp;

	chunkData.GetChunkDataSize(dataLen); 

	// No internal file created yet
	if(abStorageName.length() == 0) {
	   	retStatus = DMFileHandler::createTempESNFileName(abStorageName);
		abStorageName += ".lob";
		if(retStatus != SYNCML_DM_SUCCESS)
			return retStatus;
	}
	else// Replace previous data
		{
			// Set first trunk
			if(offset == 0)
			{	totalSize = 0L;

				// Remove the current data file
  				retStatus = OpenInternalStorageFile();
				if(retStatus != SYNCML_DM_SUCCESS)
					return retStatus;

				retStatus = fileHandle->deleteFile();
				if(retStatus != SYNCML_DM_SUCCESS)
					return retStatus;
				delete fileHandle;
				fileHandle = NULL;

			}
	}

	if(dataLen != 0)
	{
		retStatus = OpenInternalStorageFile();
		if(retStatus != SYNCML_DM_SUCCESS)
			return retStatus;

		chunkData.GetChunkData(&bufp); 	// the chunk data is available 	
		if(fileHandle == NULL  ||bufp == NULL)
			return SYNCML_DM_INVALID_PARAMETER;

	    		if(fileHandle->seek(XPL_FS_SEEK_SET, offset) != SYNCML_DM_SUCCESS) 
		        return  SYNCML_DM_IO_FAILURE;
		    	if(fileHandle->write(bufp, dataLen) != SYNCML_DM_SUCCESS) 
	      		  return  SYNCML_DM_IO_FAILURE;
	}
	totalSize = offset + dataLen;
	offset += dataLen;
	
	m_oAttr.SetSize(totalSize);
	chunkData.SetReturnLen(dataLen);
	return retStatus;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::SetFirstChunk
// DESCRIPTION     : Set first chunk
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::SetFirstChunk(DmtDataChunk& chunkData)  
{
	SYNCML_DM_RET_STATUS_T retStatus = DmtRWPluginNode::SetFirstChunk(chunkData);
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;
 	return SetChunkData(chunkData);
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::SetNextChunk
// DESCRIPTION     : Get first chunk
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::SetNextChunk(DmtDataChunk& chunkData)
{
	SYNCML_DM_RET_STATUS_T retStatus = DmtRWPluginNode::SetNextChunk(chunkData);
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;
 	return SetChunkData(chunkData);
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::SetLastChunk
// DESCRIPTION     : Set last chunk of data
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::SetLastChunk(DmtDataChunk& chunkData)
{
	SYNCML_DM_RET_STATUS_T retStatus = DmtRWPluginNode::SetLastChunk(chunkData);
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;

	retStatus = SetChunkData(chunkData);;
	if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;

	retStatus = CloseInternalFile();
	return retStatus;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::CloseInternalFile
// DESCRIPTION     : Open intenal storage file
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::CloseInternalFile()
{
  SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
  if (fileHandle != NULL)
 { 
     fileHandle->close();
     delete fileHandle;
     fileHandle = NULL;
  }
  offset = 0;
  return retStatus;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::Commit
// DESCRIPTION     : Commit changes of the node
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::Commit()
{
	SYNCML_DM_RET_STATUS_T retStatus = DmtRWPluginNode::Commit();
	if(retStatus != SYNCML_DM_SUCCESS)
	  return retStatus;
	 return 	CloseInternalFile();
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : testRWPluginNode::Delete
// DESCRIPTION     : Delete the node 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
// 
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T testRWPluginNode::Delete()
{
  SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
  if(abStorageName.length() != 0)
  {
  	retStatus = OpenInternalStorageFile();
	  if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;
	
	  retStatus = fileHandle->deleteFile();
	  if(retStatus != SYNCML_DM_SUCCESS)
		return retStatus;
	  delete fileHandle;
	  fileHandle = NULL;
	  abStorageName = NULL;
  }
  return retStatus;
}

//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtRWPluginTree::IsESNSetComplete
// DESCRIPTION     : Open intenal storage file
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : 
//--------------------------------------------------------------------------------------------
// 
SYNCML_DM_RET_STATUS_T testRWPluginNode::OpenInternalStorageFile()  
{
  SYNCML_DM_RET_STATUS_T retStatus = SYNCML_DM_SUCCESS;
    struct  stat st;
  
  // If the file is not opened before
  if(fileHandle == NULL) 
  {
   if(abStorageName.length() != 0) {
	
    INT32 modeFlag = XPL_FS_FILE_RDWR;
    // If file does not exist use write mode instead of read/write to prevent file I/O error
//    if (!XPL_FS_Exist(abStorageName.c_str()))
    if(stat(abStorageName.c_str(), &st) < 0)
    {
		modeFlag = XPL_FS_FILE_WRITE;
    }
    fileHandle = new DMFileHandler(abStorageName.c_str());
    if (fileHandle->open(modeFlag) != SYNCML_DM_SUCCESS)
       return SYNCML_DM_IO_FAILURE;
    totalSize = fileHandle->size();
   }
   else
   	totalSize = 0;	
  }
   return retStatus;
}
#endif


extern "C" 
SYNCML_DM_RET_STATUS_T DMT_PluginLib_Data_GetPluginTree(
	CPCHAR pluginRootNodePath,
	DMStringMap & mapParameters,	//For the Tree
	PDmtAPIPluginTree & pPluginTree	//root tree for the current path
)
{
//   PDmtPluginTree pMyTree=PDmtRWPluginTree(new DmtRWPluginTree(pluginRootNodePath));
   PDmtPluginTree pMyTree= PDmtPluginTree (new testRWPluginTree(pluginRootNodePath));

   PtestRWPluginNode pNode;

   DMStringVector oChildren;
   oChildren.push_back("branch1");
//   oChildren.push_back("branch2");

//Interior node   
   pNode=new testRWPluginNode(pMyTree, 
      "",   //RootNode is interior node
      oChildren
   );
   pMyTree->SetNode("", 
      PDmtNode(pNode.GetPtr())
   );
   DMStringVector oChildren1;
   oChildren1.push_back("char");
   oChildren1.push_back("int");
   oChildren1.push_back("lob");
   oChildren1.push_back("lobbin");
   oChildren1.push_back("FloatLeaf");
   oChildren1.push_back("DateLeaf");
   oChildren1.push_back("TimeLeaf");

//Interior node   
   pNode=new testRWPluginNode(pMyTree, 
      "branch1",   //RootNode is interior node
      oChildren1
   );
   pMyTree->SetNode("branch1", 
      PDmtNode(pNode.GetPtr())
   );

//Leaf node
    pNode=new testRWPluginNode(
               pMyTree, 
               "branch1/char", 
               DmtData("char_value"),
               false
            );
     pMyTree->SetNode("branch1/char", 
               PDmtNode(pNode.GetPtr())
            );
 // Integer node
    pNode=new testRWPluginNode(
               pMyTree, 
               "branch1/int", 
               DmtData(1234),
               false
            );
	pMyTree->SetNode("branch1/int", 
               PDmtNode(pNode.GetPtr())
            );
//ESN node of char type
	pNode=new testRWPluginNode(
				  pMyTree, 
				  "branch1/lob", 
				  DmtData(),
				  true
			   );
	pMyTree->SetNode("branch1/lob", 
				  PDmtNode(pNode.GetPtr())
			   );
 // Binary ESN node
	pNode=new testRWPluginNode(
						 pMyTree, 
						 "branch1/lobbin", 
						 DmtData(NULL, 0),
						 true
					  );
	pMyTree->SetNode("branch1/lobbin", 
						 PDmtNode(pNode.GetPtr())
					  );

   pNode=new testRWPluginNode(
			  pMyTree, 
			  "branch1/FloatLeaf", 
			  DmtData("-1.23456e+2", SYNCML_DM_DATAFORMAT_FLOAT),
			  false
		   );
	pMyTree->SetNode("branch1/FloatLeaf", 
			  PDmtNode(pNode.GetPtr())
		   );
   
   pNode=new testRWPluginNode(
			  pMyTree, 
			  "branch1/DateLeaf", 
			  DmtData("2005-10-18", SYNCML_DM_DATAFORMAT_DATE),
			  false
		   );
	pMyTree->SetNode("branch1/DateLeaf", 
			  PDmtNode(pNode.GetPtr())
		   );
   
   pNode=new testRWPluginNode(
			  pMyTree, 
			  "branch1/TimeLeaf", 
			  DmtData("11:38:58", SYNCML_DM_DATAFORMAT_TIME),
			  false
		   );
	pMyTree->SetNode("branch1/TimeLeaf", 
			  PDmtNode(pNode.GetPtr())
		   );

   pPluginTree=pMyTree;
   return SYNCML_DM_SUCCESS;
}

extern "C" 
int DMT_PluginLib_GetAPIVersion(void)
{
   return DMT_PLUGIN_VERSION_1_1;
}

