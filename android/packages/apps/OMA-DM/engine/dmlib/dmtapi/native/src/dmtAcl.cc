#include "dmt.hpp"
#include "dm_uri_utils.h"
#include "dmConfigItem.h"

DmtAcl::DmtAcl()
{
}


DmtAcl::DmtAcl(CPCHAR szAcl )
{
  if ( !szAcl )
    return; // empty
    
  // parse incoming string
  DMString strAcl, strAclReminder = szAcl;
  
  while ( DmStringParserGetItem( strAcl, strAclReminder, '&' ) )
  {
    DMString strCmd;
    if ( !DmStringParserGetItem( strCmd, strAcl, '=' ) )
      continue;
      
    SYNCML_DM_ACL_PERMISSIONS_T nPermision = 0;
    
    if ( strCmd == "Add" )
      nPermision = SYNCML_DM_ACL_ADD;
    else if ( strCmd == "Delete" )
      nPermision = SYNCML_DM_ACL_DELETE;
    else if ( strCmd == "Exec" )
      nPermision = SYNCML_DM_ACL_EXEC;
    else if ( strCmd == "Get" )
      nPermision = SYNCML_DM_ACL_GET;
    else if ( strCmd == "Replace" )
      nPermision = SYNCML_DM_ACL_REPLACE;
    else 
      continue;  // unknown cmd
      
    DMString strHost;
    
    while ( DmStringParserGetItem( strHost, strAcl, '+' ) )
      AddPermission( DmtPrincipal( strHost.c_str() ), nPermision );
    
  }
}


static inline void AddCommand( DMString& str, CPCHAR szCmd, const DMString& strPrincipals )
{
  if ( strPrincipals[0] == 0 )
    return;
    
  if ( !str[0] == 0 )
    str += "&";
  str += szCmd;
  str += strPrincipals;
}

DMString DmtAcl::toString() const
{
  DMString strAdd, strDelete, strExec, strGet, strReplace;
  
  for ( DMMap<DmtPrincipal, int>::POS it = m_mapAcls.begin();
    it != m_mapAcls.end(); it++ ){
      
    const DmtPrincipal& first = m_mapAcls.get_key( it );
    SYNCML_DM_ACL_PERMISSIONS_T second = m_mapAcls.get_value( it );

    if ( second & SYNCML_DM_ACL_ADD )
       DMConfigItem::AttachProperty( strAdd, '+', first.getName() );

    if ( second & SYNCML_DM_ACL_DELETE )
      DMConfigItem::AttachProperty( strDelete, '+', first.getName() );
    
    if ( second & SYNCML_DM_ACL_EXEC )
      DMConfigItem::AttachProperty( strExec, '+', first.getName() );
    
    if ( second & SYNCML_DM_ACL_GET )
      DMConfigItem::AttachProperty( strGet, '+', first.getName() );
    
    if ( second & SYNCML_DM_ACL_REPLACE )
      DMConfigItem::AttachProperty( strReplace, '+', first.getName() );
  }
  
  DMString str;
  
  AddCommand( str, "Add=", strAdd );
  AddCommand( str, "Delete=", strDelete );
  AddCommand( str, "Exec=", strExec );
  AddCommand( str, "Get=", strGet );
  AddCommand( str, "Replace=", strReplace );
  
  return str;
}


void DmtAcl::GetPrincipals( DMVector<DmtPrincipal>& aPrincipals )const
{
  for ( DMMap<DmtPrincipal, int>::POS it = m_mapAcls.begin();
    it != m_mapAcls.end(); it++ )
    aPrincipals.push_back( m_mapAcls.get_key( it ) );
}


SYNCML_DM_ACL_PERMISSIONS_T DmtAcl::GetPermissions( const DmtPrincipal& principal) const
{
  SYNCML_DM_ACL_PERMISSIONS_T n = 0;

  if ( m_mapAcls.lookup( principal, n ) )
    return n;

  return 0; // not found
}

void DmtAcl::SetPermission( const DmtPrincipal& principal, SYNCML_DM_ACL_PERMISSIONS_T permissions)
{
  m_mapAcls.put( principal, permissions );
}

void DmtAcl::AddPermission( const DmtPrincipal& principal, SYNCML_DM_ACL_PERMISSIONS_T permissions )
{
  UINT16 nCurPermission = GetPermissions( principal );

  m_mapAcls.put( principal ,permissions | nCurPermission);
}

void DmtAcl::DeletePermission( const DmtPrincipal& principal, SYNCML_DM_ACL_PERMISSIONS_T permissions )
{
  SYNCML_DM_ACL_PERMISSIONS_T nCurPermission = GetPermissions( principal );

  m_mapAcls.put( principal ,nCurPermission & (~permissions ) );
}

BOOLEAN DmtAcl::IsPermitted( const DmtPrincipal& principal, SYNCML_DM_ACL_PERMISSIONS_T permissions ) const
{
  BOOLEAN retValue =  (GetPermissions( principal ) & permissions ) != 0 ;
  return retValue;
}


