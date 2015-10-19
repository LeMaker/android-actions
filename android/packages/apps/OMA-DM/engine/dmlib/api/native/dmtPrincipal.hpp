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

#ifndef __DMTPRINCIPAL_H__
#define __DMTPRINCIPAL_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
 \file dmtPrincipal.hpp
 \brief The dmtPrincipal.hpp header file contains DmtPrincipal class definition. \n
               This class represents actors from the security viewpoint. Every DmtTree \n
                is associated with a DMTPrincipal,  and they are also used in DmtAcl.
*/

#include "jem_defs.hpp"

/**
  * DmtPrincipal represents actors from the security viewpoint.
  * Every DmtTree is associated with a DMTPrincipal,
  * and they are also used in DmtAcl.
  * \par Category: General  
  * \par Persistence: Transient
  * \par Security: Non-Secure
  * \par Migration State: FINAL
  */
class DmtPrincipal
{ 
private:
  DMString  m_strPrincipal;

public:
 /**
  * Default constructor - no memory allocation or any other resources have been performed.
  */
  DmtPrincipal() {}

/**
  * Constructor receive object DmtPrincipal as a parameter. The memory for DMT Principal name will be allocated.
  * \param oCopyFrom [in] - reference to DmtPrincipal object
  */
  DmtPrincipal( const DmtPrincipal& oCopyFrom ) {m_strPrincipal = oCopyFrom.m_strPrincipal; }

 /**
  * Constructor receive DmtPrincipal name as a parameter. The memory for DMT Principal name will be allocated.
  * \param princip [in] - DMT principal as a string
  */
  DmtPrincipal(CPCHAR princip) {m_strPrincipal = princip;}

  /**
  * Comparison operator (equally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param p [in] -  pointer to DmtPrincipal object
  * \return boolean result of comparison
  * \par Prospective Clients:
  * Internal Classes.
  */
  BOOLEAN operator==( const DmtPrincipal &p ) const { return m_strPrincipal == p.m_strPrincipal;}


 /**
  * Retrieves  name of DMT principle
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return principal as ad DMString.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  const DMString& getName() const
  {
     return m_strPrincipal;
  }

 /**
  * Assigns DMT principle to the Provided string 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param princip [in] - principal as a string
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void assign(CPCHAR princip) {m_strPrincipal = princip;}
    
};

#endif
