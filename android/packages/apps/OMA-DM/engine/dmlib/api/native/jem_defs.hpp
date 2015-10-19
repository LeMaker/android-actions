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

#ifndef __JEM_DEFS_H__
#define __JEM_DEFS_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/**  
  \file jem_defs.hpp
  \brief Contains JemBaseObject class and JemSmartPtr template class definition

       The jem_defs.hpp header file contains JemBaseObject class and  JemSmartPtr \n
       template class definition. \n
       This is the utility definition for the Dmt C++ API. It defineds Basic types as byte and boolean.\n
       Also some standard collection helper class as Date that are used in the API.\n
       The <b>JemSmartPtr</b> class is a Smart pointer; works with classes derived from JemBaseObject. \n
       The Smart pointer class keeps tracking counter references to the object and deletes it  (via "DecRef" method) \n
       if no one longer keeps a pointer to this object.\n
       The <b>JemBaseObject</b> class  is a base object for any ref-counted object.
*/

#include "dmtDefs.h"
#include "dmstring.h"
#include "dmvector.h"

 /**Definition for the "byte" as unsigned char*/
typedef UINT8 byte;
 /**Definition for the "boolean" as unsigned char*/
typedef BOOLEAN boolean;
 /**Definition for the "JemDate" as INT64*/
typedef INT64 JemDate;

  /**
  * Gets "safe" string. If string is NULL then an empy string will be returned.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par API Migration State:  FINAL
  * \param str [in] - string to be evaluated
  * \return original (if not null) or an empty string
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
inline CPCHAR DmtGetSafeStrPtr( CPCHAR str )
{
  return (str ? str : "" );
}

/**
* Smart pointer; works with classes derived from JemBaseObject. 
* Smartpointer class keeps tracking refcounter of the object and deletes it 
* (via "DecRef" method) if no one longer keeps a pointer to this object.
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
template<typename T> class JemSmartPtr 
{
  T * m_pData;
public:
 /**Definition for the element_type*/
    typedef T element_type;
	
  /**
  * Default constructor - no memory allocation performed.
  */
    JemSmartPtr(): m_pData(NULL) {}

/**
 * Copy constructor. The memory for the "cp"  will be allocated.
 * \param cp [in] - source object
 */

    JemSmartPtr( const JemSmartPtr<element_type> & cp): m_pData(cp.m_pData) { _AddRef();}

/**
 * Constructor. The memory will be allocated.
 * \param p [in] - pointer to an element
 */
    JemSmartPtr( element_type * p): m_pData( p ) { _AddRef(); }

  /**
  * Assignment  operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param pData [in] - pointer to object 
  * \return constant reference to Smart pointer
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    const JemSmartPtr<element_type>& operator=( element_type * pData)
    {
        if (m_pData != pData){
            element_type* pOldData = m_pData;
            m_pData = pData;
            _AddRef(); 
            pOldData->DecRef();
        }
        return *this;
    }

  /**
  * Assignment  operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param cp [in] - constant reference to object 
  * \return constant reference to Smart pointer
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    const JemSmartPtr<element_type>& operator=(const JemSmartPtr<element_type>& cp)
    { 
        return operator=(cp.m_pData); 
    }

  /** 
  * Destructor - freeing all dynamic resources 
  */
    ~JemSmartPtr()
    { 
        _Release(); 
    }

  /**
  * Casting to type operator
  * \return pointer to an element
  */
    operator element_type*() const { return m_pData; }

  /**
  * Casting to type operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return reference to an element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    element_type& operator*() const
    { 
        return *m_pData; 
    }

  /**
  * Member selection via pointer operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return pointer to an element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    element_type* operator->() const
    { 
        return m_pData; 
    }

  /**
  * Comparison operator (equally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param p [in] - pointer to an element for comparison 
  * \return boolean result of comparison
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    BOOLEAN operator==(element_type* p) const { return m_pData == p; }
  
  /**
  * Comparison operator (equally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptr [in] - constant reference to Smart pointer for comparison 
  * \return boolean result of comparison
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    BOOLEAN operator==(const JemSmartPtr<element_type>& ptr) const { return operator==(ptr.m_pData);}
  
  /**
  * Comparison operator (unequally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param p [in] - pointer to an element for comparison 
  * \return boolean result of comparison
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    BOOLEAN operator!=(element_type* p) const { return !(operator==(p));}

  /**
  * Comparison operator (unequally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptr [in] - constant reference to Smart pointer for comparison 
  * \return boolean result of comparison
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    BOOLEAN operator!=(const JemSmartPtr<element_type> & ptr) const { return !(operator==(ptr));}
  
  /**
  * Retrieves pointer to an element
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return pointer to an element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    element_type* GetPtr() const { return m_pData; }



private:
  void _Release() { m_pData->DecRef(); }
  void _AddRef()  { m_pData->AddRef(); }
};

  /**
  * Reverse comparison operators for JemSmartPtr (equally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param  p [in] - Smart pointer
  * \param  ptr [in] - reference to Smart pointer
  * \return boolean result of comparison
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<typename T> inline BOOLEAN operator==(T* p, const JemSmartPtr<T>& ptr)
{
  return ptr == p;
}

  /**
  * Reverse comparison operators for JemSmartPtr (unequally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param  p [in] - Smart pointer
  * \param  ptr [in] - reference to Smart pointer
  * \return boolean result of comparison
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<typename T> inline BOOLEAN operator!=(T* p, const JemSmartPtr<T>& ptr)
{
	return ptr != p;
}


/**
* Base object for any ref-counted object. 
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class JemBaseObject
{
  INT64 m_nRefCount;

protected:
  /**
  * Protected destructor, since this is reference counted object and nobody should delete it manually. 
  */
  virtual ~JemBaseObject(){}
  
public:
	
  /**
  * Default constructor - no memory allocation performed.
  */
  JemBaseObject();

  /**
  * Operator New
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nSize [in] - requested size
  * \return void pointer
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void* operator new( size_t nSize )
    { return DmtMemAlloc( nSize ); }

  /**
  * Operator Delete
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param p [in] - void pointer
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  inline void  operator delete( void* p )
    { DmtMemFree( p ); }

  /**
  * Increments reference counter
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par API Migration State: FINAL
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void AddRef();
  
  /**
  * Decrements reference counter; deletes object if it's the last reference
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par API Migration State: FINAL
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void DecRef();
};

/**  Defines PJemBaseObject  as  a smart pointer for DmtNode class */
typedef JemSmartPtr<JemBaseObject> PJemBaseObject;

// some additional smart pointers:
class DmtTree;
class DmtNode;

/**  Defines PDmtNode  as  a smart pointer for DmtNode class */
typedef JemSmartPtr<DmtTree> PDmtTree;

/** Defines PDmtTree as a smartpointer type for DmtTree class*/
typedef JemSmartPtr<DmtNode> PDmtNode;


#ifndef NULL
 /** Define NULL */
  #define NULL ((void*) 0)
#endif

////////////////////////////////////////////////////////////////////
// inline functions for JemBaseObject

  /**
  * Default constructor for the class JemBaseObject - no memory allocation performed.
  */
inline JemBaseObject::JemBaseObject()
{ 
  m_nRefCount = 0; 
}

  /**
  * Increments reference counter
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
inline void JemBaseObject::AddRef() 
{
  if ( this == NULL )
    return;

  m_nRefCount++; 
}

//decrement reference (if == 0 then delete object).
  /**
  * Decrements reference counter; delete object if it's the last reference
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

inline void JemBaseObject::DecRef()
{
  if ( this == NULL )
    return;

  if ( --m_nRefCount <= 0 ) 
    delete this; 
}

#endif
