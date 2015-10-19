#include "dmStringUtil.h"
#include "xpl_Logger.h"
#include "dmt.hpp"
#include "dmMemory.h"

extern "C" {
#include "xpt-b64.h"
}

DmtData::DmtData()
{
  metaFormat = SYNCML_DM_DATAFORMAT_UNDEFINED;
}

/**
*  Combine STRING, DATE and TIME type objects through a single 
*  contructor
*/
DmtData::DmtData(CPCHAR szStr):metaFormat(SYNCML_DM_DATAFORMAT_STRING)
{
  SetString(szStr, SYNCML_DM_DATAFORMAT_STRING);
}

DmtData::DmtData(CPCHAR szStr, SYNCML_DM_DATAFORMAT_T type):
                 metaFormat(type)
{
  SetString(szStr, type);
}

DmtData::DmtData(INT32 integer)
{
  SetInt(integer);
}

DmtData::DmtData(BOOLEAN b)
{
  SetBoolean(b);
}

DmtData::DmtData(BOOLTYPE b)
{
  SetBoolean(b?1:0);
}

DmtData::DmtData( const UINT8 * bin, INT32 len)
{
  SetBinary(bin,len);
}

DmtData::DmtData( const DMStringVector& aChildren )
{
  SetNodeValue(aChildren);
}

DmtData::~DmtData()
{
}

SYNCML_DM_RET_STATUS_T DmtData::SetString(CPCHAR szStr, 
                                SYNCML_DM_DATAFORMAT_T type)
{
  // Extend this method to initialize other text/plain type
  // DM data. 
  //
  if (szStr==NULL)
  {
    metaFormat = SYNCML_DM_DATAFORMAT_NULL;
  } else
  {
    m_strValue = szStr;
    if ( szStr[0] && m_strValue.GetBuffer() == NULL )
    {
       XPL_LOG_DM_API_Error(("DmtData::SetString : unable allocate memory\n"));
       metaFormat = SYNCML_DM_DATAFORMAT_UNDEFINED;
       return SYNCML_DM_DEVICE_FULL;
    }   
    else if ( type == SYNCML_DM_DATAFORMAT_INT ) 
    {
       m_nValue =  DmAtoi(szStr);
    }   
    else if ( type == SYNCML_DM_DATAFORMAT_BOOL ) 
    {
       m_bValue =  DmStrcasecmp(szStr, "false");
    }   
    else if ( type == SYNCML_DM_DATAFORMAT_FLOAT ||  
              type == SYNCML_DM_DATAFORMAT_DATE  || 
              type == SYNCML_DM_DATAFORMAT_TIME   ) 
    {
       m_strValue.trim();
    }   
    metaFormat = type;
  }
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtData::SetBoolean(BOOLEAN bValue)
{
  m_bValue = bValue;
  metaFormat = SYNCML_DM_DATAFORMAT_BOOL;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtData::SetInt(INT32 nValue)
{
  m_nValue = nValue;
  char numbuf[MAX_INT_STRING_LENGTH];
  DmSprintf( numbuf, "%d", m_nValue );
  m_strValue = numbuf;
  metaFormat = SYNCML_DM_DATAFORMAT_INT;
  return SYNCML_DM_SUCCESS;

}

SYNCML_DM_RET_STATUS_T DmtData::SetFloat(CPCHAR sFloat)
{
  return SetString(sFloat, SYNCML_DM_DATAFORMAT_FLOAT);
}


SYNCML_DM_RET_STATUS_T DmtData::SetDate(CPCHAR sDate)
{
  return SetString(sDate, SYNCML_DM_DATAFORMAT_DATE);
}

SYNCML_DM_RET_STATUS_T DmtData::SetTime(CPCHAR sTime)
{
  return SetString(sTime, SYNCML_DM_DATAFORMAT_TIME);
}

SYNCML_DM_RET_STATUS_T DmtData::SetBinary(const UINT8 * buf, INT32 len)
{
  if ( buf && len > 0 )
  {
    m_pBinValue.set_size( len );
    if ( m_pBinValue.size() != len )
    {
       XPL_LOG_DM_API_Error(("DmtData::SetBinary : unable allocate memory\n"));
       m_pBinValue.clear();
       metaFormat = SYNCML_DM_DATAFORMAT_UNDEFINED;
       return SYNCML_DM_DEVICE_FULL;
    }
    memcpy( m_pBinValue.get_data(), buf, len );
  }
  else
    m_pBinValue.clear();
  
  metaFormat = SYNCML_DM_DATAFORMAT_BIN;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtData::SetNodeValue( const DMStringVector& aChildren )
{

   m_astrNodeValue = aChildren;
   if ( m_astrNodeValue.size() != aChildren.size() )
   {
       XPL_LOG_DM_API_Error(("DmtData::SetNodeValue : unable allocate memory\n"));
       metaFormat = SYNCML_DM_DATAFORMAT_UNDEFINED;
       m_astrNodeValue.clear();
       return SYNCML_DM_DEVICE_FULL;
   } 
   metaFormat = SYNCML_DM_DATAFORMAT_NODE;
   return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T DmtData::Set( const DmtData & oData )
{
    SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;
    switch (oData.GetType()) 
    {
        case SYNCML_DM_DATAFORMAT_STRING:
        case SYNCML_DM_DATAFORMAT_DATE:
        case SYNCML_DM_DATAFORMAT_TIME:
            res = SetString(oData.GetStringValue(), oData.GetType());
            break;
          
        case SYNCML_DM_DATAFORMAT_BIN:
        {
            const DMVector<UINT8> & value = oData.GetBinaryValue();
            res = SetBinary(((DMVector<UINT8> &)value).get_data(),value.size());
        }    
            break;
             
        case SYNCML_DM_DATAFORMAT_NODE:
            res = SetNodeValue(oData.GetNodeValue());
            break;

        case SYNCML_DM_DATAFORMAT_FLOAT:
        case SYNCML_DM_DATAFORMAT_BOOL:
        case SYNCML_DM_DATAFORMAT_INT:    
        case SYNCML_DM_DATAFORMAT_NULL:
        case SYNCML_DM_DATAFORMAT_UNDEFINED:  
            *this = oData;
         break;
            
   }
   return res;  
}

SYNCML_DM_RET_STATUS_T DmtData::AddNodeValue( const DMString & sChild )
{
 
   INT32 size = m_astrNodeValue.size() + 1;
   m_astrNodeValue.push_back(sChild);
   if ( size != m_astrNodeValue.size() )
   {
       XPL_LOG_DM_API_Error(("DmtData::SetNodeValue : unable allocate memory\n"));
       return SYNCML_DM_DEVICE_FULL;
   }
   metaFormat = SYNCML_DM_DATAFORMAT_NODE;
   return SYNCML_DM_SUCCESS;
}

const DMString & DmtData::GetStringValue() const
{
    return m_strValue;
}

const DMVector<UINT8> & DmtData::GetBinaryValue() const
{
    return m_pBinValue;
}

const DMStringVector & DmtData::GetNodeValue() const
{
    return m_astrNodeValue; 
}

SYNCML_DM_RET_STATUS_T DmtData::GetString( DMString& str ) const
{  
  if (metaFormat == SYNCML_DM_DATAFORMAT_BIN)
  {
     /* Base64 encode for the ./CSIM/Provobj node. */
    UINT32 dataSize = m_pBinValue.size();

    if (dataSize == 0) {
        str = "";
        XPL_LOG_DM_API_Error(("DmtData::GetString() on empty BIN node returning empty string"));
        return SYNCML_DM_SUCCESS;
    }

    UINT32 encLen = base64GetSize(dataSize);
    XPL_LOG_DM_API_Error(("DmtData::GetString() dataSize=%d encLen=%d\n", dataSize, encLen));
    XPL_LOG_DM_API_Error(("DmtData::GetString() [0]=%02x [1]=%02x [%d]=%02x\n", m_pBinValue.get_data()[0],
        m_pBinValue.get_data()[1], dataSize - 1, m_pBinValue.get_data()[dataSize - 1]));

    UINT8* pEncData = (UINT8 *)DmAllocMem(encLen+1);
    memset(pEncData, 0, encLen+1);
    UINT32 offset = 0;
    int totalSize = base64Encode(pEncData, encLen, (DataBuffer_t)m_pBinValue.get_data(),
        (BufferSize_t *)&dataSize, (BufferSize_t *)&offset, 0, NULL);

    str = DMString( reinterpret_cast<CPCHAR>(pEncData) );
    DmFreeMem(pEncData);
    XPL_LOG_DM_API_Error(("DmtData::GetString() on BIN node returning: %s\n", str.c_str()));
    return SYNCML_DM_SUCCESS;
  }

  if (metaFormat == SYNCML_DM_DATAFORMAT_NODE)
  {
     /* NODE conversion to String not allowed */
    return SYNCML_DM_INVALID_PARAMETER;
  }

  if ( (metaFormat == SYNCML_DM_DATAFORMAT_STRING) ||
       (metaFormat == SYNCML_DM_DATAFORMAT_INT) ||
       (metaFormat == SYNCML_DM_DATAFORMAT_FLOAT) ||
       (metaFormat == SYNCML_DM_DATAFORMAT_DATE) ||
       (metaFormat == SYNCML_DM_DATAFORMAT_TIME) )
  {
    str = m_strValue;
    return SYNCML_DM_SUCCESS;
  }

  if (metaFormat == SYNCML_DM_DATAFORMAT_BOOL)
  {
    str = m_bValue ? "true" : "false";
    return SYNCML_DM_SUCCESS;
  }

   if (metaFormat == SYNCML_DM_DATAFORMAT_NULL)
  {
    str = "";
    return SYNCML_DM_SUCCESS;
  }
  

  /* unknown type - string conversiob is not allowed */
  return SYNCML_DM_INVALID_PARAMETER;
}

SYNCML_DM_RET_STATUS_T DmtData::GetBoolean( BOOLEAN& bValue ) const
{
  bValue = FALSE;
  if (metaFormat != SYNCML_DM_DATAFORMAT_BOOL)
    return SYNCML_DM_INVALID_PARAMETER;

  bValue = m_bValue;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtData::GetBoolean( BOOLTYPE& bValue ) const
{
  bValue = FALSE;
  if (metaFormat != SYNCML_DM_DATAFORMAT_BOOL)
    return SYNCML_DM_INVALID_PARAMETER;

  bValue = m_bValue?TRUE:FALSE;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T  DmtData::GetInt( INT32& nValue ) const
{
  if (metaFormat != SYNCML_DM_DATAFORMAT_INT)
    return SYNCML_DM_INVALID_PARAMETER;

  nValue = m_nValue;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T  DmtData::GetFloat( DMString& sFloat ) const
{
  if (metaFormat != SYNCML_DM_DATAFORMAT_FLOAT)
    return SYNCML_DM_INVALID_PARAMETER;

  sFloat = m_strValue;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T  DmtData::GetDate( DMString& sDate ) const
{
  if (metaFormat != SYNCML_DM_DATAFORMAT_DATE)
    return SYNCML_DM_INVALID_PARAMETER;

  sDate = m_strValue;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T  DmtData::GetTime( DMString& sTime ) const
{
  if (metaFormat != SYNCML_DM_DATAFORMAT_TIME)
    return SYNCML_DM_INVALID_PARAMETER;

  sTime = m_strValue;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtData::GetBinary(DMVector<UINT8>& buffer ) const
{
  if (metaFormat != SYNCML_DM_DATAFORMAT_BIN) 
    return SYNCML_DM_INVALID_PARAMETER;

  buffer = m_pBinValue;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtData::GetNodeValue( DMStringVector& aChildren ) const
{
  if (metaFormat != SYNCML_DM_DATAFORMAT_NODE) 
    return SYNCML_DM_INVALID_PARAMETER;

  aChildren = m_astrNodeValue;
  return SYNCML_DM_SUCCESS;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtData::GetSize
// DESCRIPTION     : Get data size
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
// 
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtData::GetSize(INT32 &dataSize) const
{
  SYNCML_DM_RET_STATUS_T res = SYNCML_DM_SUCCESS;
  switch(metaFormat)
  {
	case SYNCML_DM_DATAFORMAT_STRING:
				dataSize = m_strValue.length();
				 break;
			   
	case SYNCML_DM_DATAFORMAT_BIN:
				 dataSize = m_pBinValue.size();
				 break;
				  
	case SYNCML_DM_DATAFORMAT_NODE:
				dataSize = m_astrNodeValue.size();
				 break;
		
	case SYNCML_DM_DATAFORMAT_BOOL:
	case SYNCML_DM_DATAFORMAT_INT:    
	case SYNCML_DM_DATAFORMAT_NULL:
	case SYNCML_DM_DATAFORMAT_FLOAT:
	case SYNCML_DM_DATAFORMAT_DATE:
	case SYNCML_DM_DATAFORMAT_TIME:
	case SYNCML_DM_DATAFORMAT_UNDEFINED:  
				{	DMString strValue;
					res = GetString(strValue);
					if(res == SYNCML_DM_SUCCESS)
					   dataSize = strValue.length();
				}				
			  	break;
				 
	}
	return res;
}

