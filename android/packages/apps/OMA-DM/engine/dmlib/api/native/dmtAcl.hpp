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

#ifndef __DMTACL_H__
#define __DMTACL_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*! \file dmtAcl.hpp
      \brief  The dmtAcl.hpp header file contains DmtAcl class definition. \n
                DmtAcl models the standard ACL attribute Acl composes of Principals associated with \n 
                accessrights Principals are server identifications. 
*/

#include "jem_defs.hpp"
#include "dmtDefs.h"
#include "dmtPrincipal.hpp"

/**
 * DmtAcl models the standard ACL attribute Acl composes of Principals associated with \n 
 * access rights Principals are server identifications. <P>
 * 
 * <b>Access rights are</b>\n
 *    ADD\n
 *    DELETE\n
 *    EXEC\n
 *    GET\n
 *    REPLACE \n\n
 *  
 * They are bit-wised together so we can use an integer to represent combinations of  Acl Rights.  
 * \par Category: General  
 * \par Persistence: Transient
 * \par Security: Non-Secure
 * \par Migration State: FINAL
 */
class DmtAcl 
{ 
public:    
  /**
   * SyncML DM basic permissions for the servers to access DMT. The "COPY" permission is not supported. 
   */
  enum {
   /** "Add" Permission (permission to add new node to DMT)*/
    ADD    = SYNCML_DM_ACL_ADD,
    /** "Delete" Permission (permission to remove node from DMT)*/
    DELETE = SYNCML_DM_ACL_DELETE,
    /** "Execute" Permission (permission to launch executable plug-ins )*/
    EXEC   = SYNCML_DM_ACL_EXEC,
    /** "Get" Permission (permission to get DMT node's value)*/
    GET    = SYNCML_DM_ACL_GET,
    /** "Replace" Permission (permission to change DMT node's value)*/
    REPLACE= SYNCML_DM_ACL_REPLACE
  };
public:    

  /**
  * Default constructor - no memory allocation performed.
  **/
  DmtAcl();

  /**
  * Constructor receives ACLs are in the Syncml DM Format. The memory for the size of parameter "strAcl" will be allocated.
  * \par Example of ACL format:<br>
  * Get=ServerB&Replace=ServerB&Delete=ServerB.
  * \param strAcl [in] - ACLs are in Syncml DM Format
  */
  DmtAcl(CPCHAR strAcl );

  /**
  * ACL Format conversion
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return ACLs in the Syncml DM Format
  * \par Example of ACL format: <br>
  * Get=ServerB&Replace=ServerB&Delete=ServerB.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  DMString toString() const;
  
  /** 
  * Gets principals covered by the ACL.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param aPrincipals [out] -vector of all principals covered by the ACL (correspond to "servers" in SyncML parlance)
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  void GetPrincipals( DMVector<DmtPrincipal>& aPrincipals ) const;


  /** 
  * Gets permission for the principal
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DM server Id (principal)
  * \return permissions as an integer  
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  SYNCML_DM_ACL_PERMISSIONS_T GetPermissions( const DmtPrincipal& principal) const;  

  /** 
  * Sets permission for the principal
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DM server Id (principal)
  * \param permissions [in] - permissions Bit-wised for each individual permission. 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   void SetPermission( const DmtPrincipal& principal, SYNCML_DM_ACL_PERMISSIONS_T permissions);

  /** 
   * Adds permission for the principal
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DM server Id (principal)
  * \param permissions [in] -  individual  or group of permissions. 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   void AddPermission( const DmtPrincipal& principal, SYNCML_DM_ACL_PERMISSIONS_T permissions);

  /** 
   * Deletes permission  for the principal
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DM server Id (principal)
  * \param permissions [in] -  individual  or group of permissions. 
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   void DeletePermission( const DmtPrincipal& principal, SYNCML_DM_ACL_PERMISSIONS_T permissions );

  /** 
  * Retrieves if action permitted for the principal
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DM server Id (principal)
  * \param permissions [in] -  individual  or group of permissions. 
  * \return true if the operation for such principal is permitted.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
   BOOLEAN IsPermitted( const DmtPrincipal& principal, SYNCML_DM_ACL_PERMISSIONS_T permissions) const;
  
  private:  
   DMMap<DmtPrincipal, SYNCML_DM_ACL_PERMISSIONS_T> m_mapAcls;
};

#endif
