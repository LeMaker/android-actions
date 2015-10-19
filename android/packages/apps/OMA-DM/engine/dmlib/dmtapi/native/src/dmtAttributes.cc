#include "dmt.hpp"


DmtAttributes::DmtAttributes()
{
    m_nVersion = -1;
    m_nSize = -1;
    m_nTimestamp = -1;
}

DmtAttributes::DmtAttributes( CPCHAR name,
                               CPCHAR format,
                               CPCHAR title,
                               CPCHAR type,
                               INT32 version,
                               INT32 size,
                               const JemDate& timestamp,
                               const DmtAcl& acl)
{
    Set(name,format,title,type,version,size,timestamp,acl);
}



SYNCML_DM_RET_STATUS_T DmtAttributes::Set( const DmtAttributes & oAttr )
{
    return Set( oAttr.GetName(),    
                oAttr.GetFormat(),   
                oAttr.GetTitle(),   
                oAttr.GetType(), 
                oAttr.GetVersion(), 
                oAttr.GetSize(), 
                oAttr.GetTimestamp(),  
                oAttr.GetAcl());
}

SYNCML_DM_RET_STATUS_T DmtAttributes::Set( CPCHAR name,
                                CPCHAR format,
                                CPCHAR title,
                                CPCHAR type,
                                INT32 version,
                                INT32 size,
                                const JemDate& timestamp,
                                const DmtAcl& acl)
{
    m_nVersion = version;
    m_nSize = size;
    m_nTimestamp = timestamp;
    m_oAcl = acl;

    if(SetName(name) !=SYNCML_DM_SUCCESS )
		return SYNCML_DM_DEVICE_FULL;
		
    if(SetFormat(format) !=SYNCML_DM_SUCCESS )
			return SYNCML_DM_DEVICE_FULL;

    if(SetTitle(title) !=SYNCML_DM_SUCCESS )
		return SYNCML_DM_DEVICE_FULL;


    if ( type && type[0] )
    {
        m_strType = type;
        if ( m_strType == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }
    else
        m_strType = NULL;

    return SYNCML_DM_SUCCESS;
    
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtAttributes::SetName
// DESCRIPTION     : Set name of the Node 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtAttributes::SetName( CPCHAR name)
{
    if ( name && name[0] )
    {
        m_strName = name;
        if ( m_strName == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }
    else
        m_strName = NULL;

    return SYNCML_DM_SUCCESS;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtAttributes::SetFormat
// DESCRIPTION     : Set format of the Node 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtAttributes::SetFormat( CPCHAR format)
{
    if ( format && format[0] )
    {
        m_strFormat = format;
        if ( m_strFormat == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }
    else
        m_strFormat = NULL;
    return SYNCML_DM_SUCCESS;
}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtAttributes::SetTitle
// DESCRIPTION     : Set title of the Node 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : SYNCML_DM_RET_STATUS_T : Returns SYNCML_DM_SUCCESS if success, otherwise fails
//  
//--------------------------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T DmtAttributes::SetTitle( CPCHAR title)
{
    if ( title && title[0] )
    {
        m_strTitle = title;
        if ( m_strTitle == NULL )
            return SYNCML_DM_DEVICE_FULL;
    }
    else
        m_strTitle = NULL;
    return SYNCML_DM_SUCCESS;

}
//--------------------------------------------------------------------------------------------
// FUNCTION        : DmtAttributes::SetSize
// DESCRIPTION     : Set size of the Node 
// ARGUMENTS PASSED: 
//                  
// RETURN VALUE    : None
//  
//--------------------------------------------------------------------------------------------
void DmtAttributes::SetSize( INT32 size)
{
    m_nSize = size;
}

