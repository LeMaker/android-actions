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

#ifndef __DMVECTOR_H__
#define __DMVECTOR_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
  \file dmvector.h
  \brief Contains DMVector and DMMap template classes definition

       The dmvector.hpp header file contains DMVector and DMMap template classes definition. \n
       <b>DMVector</b> template class is simple Collection Class without overhead of STL or QT\n
       and id similar to Java's ArrayList\n
       <b>DMMap</b>  template class is simple Collection Class without overhead of STL or QT\n
       DMMap template class is similar to Java's HashMap. Potentially can be slow, especially with \n
       large number of keys, since linear search is used.
*/

#include <string.h>
#include "dmstring.h"
#include <new>

#ifdef PLATFORM_X86
#include <assert.h>
/** Definition for DMASSERT for platform compatibility*/
#define DMASSERT(expression)  assert(expression)
#else
/** Definition for DMASSERT for platform compatibility*/
#define DMASSERT(expression)  
#endif

  /**
  * Memory allocation routine. Used for debugging.
  * \warning This method for internal usage only!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Migration State: FINAL
  * \param nSize  [in] - buffer size as an integer
  * \param szFile [in] -  file name
  * \param nLine [in] -  line number 
  * \return void pointer or null if no memory available
  * \par Prospective Clients:
  * For internal usage only
  */
extern "C" void*  DmtMemAllocEx( size_t nSize, const char* szFile, int nLine );

/**
  * Memory deallocation routine
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Migration State: FINAL
  * \param p [in]  -  pointer to memory that should be freed
  * \par Prospective Clients:
  * For internal usage only
  */
extern "C" void   DmtMemFree( void* p );

/**
  * Memory allocation macro. 
  * \warning This method for internal usage only!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Migration State: FINAL
  * \param size buffer size as an integer
  * \par Prospective Clients:
  * For internal usage only
  */
#define DmtMemAlloc(size) DmtMemAllocEx(size,__FILE__,__LINE__)


/**
* Simple Collection Class without overhead of STL or QT
* DMVector template class is similar to Java's ArrayList
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
template<class type>
class DMVector
{
 public:

  /**
  * Default constructor - no memory allocation performed.
  */
  DMVector();
  
  /**
  * Construct a DMVector object as copy of a given one as a parameter. The memory for the size of parameter "oCopyFrom" will be allocated.
  * \param oCopyFrom [in] - pointer to the DMVector that should be copied.
  */
  DMVector( const DMVector& oCopyFrom );

  /* *
  * Destructor - freeing all dynamic resources 
  */
  ~DMVector();
  
  /*
  * Assignment operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param str [in] - constant reference to DMVector
  * \return reference to the DMVector object (itself)
  * \par Prospective Clients:
  * Internal Classes.
  */
  DMVector& operator = ( const DMVector& oCopyFrom );
  
  /**
  * Retrieve number of elements in the vector
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return number of elements
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  int   size() const { return m_nCount; }

  /*
  * Sets size of the vector
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return number of elements
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  int   set_size( int nNewSize );
  
  /**
  * Appends a new element to array. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptr [in] - new element to add
  * \return index of a new element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  int   push_back(const type& ptr);
  
    /**
  * Removes element from array. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param index  - element index; must be valid index.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void  remove(int index);

  /**
  * Looks for an  element in array. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptr [in] - reference to element
  * \return index of an element if found, otherwise (-1) .
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  int   find(const type& ptr) const;

  
  /**
  *  Detaches element. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return pointer to allocated element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  type* detach();

  /**
  * Const version of [] operator; allows to access any const method of element without making a copy of it.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nIndex [in] - element index
  * \return element of a vector
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */  
  const type& operator [] (int nIndex ) const;

  /**
  * Non const version of [] operator; allows to access any method of element  without making a copy of it and assign a new value.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nIndex [in]  - element index
  * \return element of a vector
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  type& operator [] (int nIndex );

  /**
  * Retrieves element
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return pointer to the element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  type* get_data() {return m_ptrs;}

  /**
  * Retrieves element
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return constant pointer to element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  const type* get_data() const {return m_ptrs;}

  /**
  * Erases all elements from the collection
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void clear() {set_size(0);}

  
private:
  // helper functions
  int   Reallocate( int nNewSize );
  void  Destroy() {set_size(0);}

  void  DestructElement( int nIndex );
  void  ConstructElement( int nIndex );

  int   m_nAllocated, m_nCount;
  type * m_ptrs;
};


/**
* Simples Collection Class without overhead of STL or QT
* DMMap template class is similar to Java's HashMap
* Potentially can be slow, especially with large number of keys, 
* since linear search is used.
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
template<class Key, class Value>
class DMMap
{
 public:
/** Definition for position in DMMap*/
  typedef int POS;

  /**
  * Default constructor - no memory allocation performed.
  */
  DMMap();

  /* *
  * Destructor - freeing all dynamic resources 
  */
  ~DMMap();

/**
  * Assignment operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oCopyFrom [in] - constant reference to DMMap
  * \return reference to the DMMap object (itself)
  * \par Prospective Clients:
  * Internal Classes.
  */
  DMMap& operator = ( const DMMap& oCopyFrom );

  /**
  * Retrieves number of elements in the map.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return number of elements
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  int   size() const;

  /**
  * Looks up for the element for the given key. Set value if an element has been found.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] -element key 
  * \param rValue [out] -element value 
  * \return "true" if element has been found otherwise - "false"
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */  
  bool  lookup( const Key & key, Value& rValue) const;

/**
  * Retrieves a value based on key. The key must exist in the map.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] - element key
  * \return constant value
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  const Value&  get(const Key & key) const;

  /**
  * Retrieves a value based on key. The key must exist in the map.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] - element key
  * \return value
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  Value&  get(const Key & key);

 /**
  * Adds or replaces value for the given key.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] -element key 
  * \param value [in] -element value 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void  put(const Key & key, const Value & value);

  /**
  * Removes element for the given key
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] -element key 
  * \return "true" in case of success, otherwise - "false"
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  bool  remove(const Key & key);
  
  /**
  * Erases all elements from the collection
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void clear() { m_aKeys.set_size( 0 ); m_aValues.set_size( 0);}

  /**
  * Retrieves first position
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return first position; can be "end()" if map is empty
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  POS begin() const {return 0;}
  
  /**
  * Retrieves "end" of the map position
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return "end" position
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  POS end() const  {return m_aKeys.size();}

  /**
  * Returns constant value by given position
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param n [in] - given position
  * \return position value
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  const Value&  get_value(POS n) const {return m_aValues[n];}
  
  /**
  * Returns value by given position
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param n [in] - given position
  * \return position value
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  Value&  get_value(POS n) {return m_aValues[n];}
  
 /**
  * Returns key for given position
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param n [in] - given position
  * \return  key for the given position
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  const Key&  get_key(POS n) const {return m_aKeys[n];}

    
 private:
  DMVector<Key>   m_aKeys;
  DMVector<Value> m_aValues;
};



////////////////////////////////////////////////////////////
// implementation   DMVector
//----------------------------------------------
/**
* Default constructor - constructs empty array. No memory alllocation is performed.
*/
template<class type>
inline DMVector<type>::DMVector()
{
  m_nAllocated = m_nCount = 0;
  m_ptrs = NULL;
}
  
  /**
  * Copy constructor; makes exact copy of the source array. The memory for the size of parameter "oCopyFrom" will be allocated.
  * \param oCopyFrom  - source object
  */
template<class type>
inline DMVector<type>::DMVector( const DMVector<type>& oCopyFrom )
{
  m_nCount = 0;
  m_ptrs = NULL;

  *this = oCopyFrom;
}
 
  /** 
  * Destructor - freeing all dynamic resources 
  */
template<class type>
inline DMVector<type>::~DMVector()
{
  Destroy();
}

  /**
  * Const version of [] operator; allows to access any const method of element without making a copy of it.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nIndex [in] - element index
  * \return element of a vector
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class type>
inline const type& DMVector<type>::operator [] (int nIndex ) const 
{
  DMASSERT( nIndex >= 0 && nIndex < m_nCount );
  return m_ptrs[nIndex];
}

  /**
  * Non const version of [] operator; allows to access any method of element  without making a copy of it and assign a new value.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nIndex [in]  - element index
  * \return element of a vector
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class type>
inline type& DMVector<type>::operator [] (int nIndex ) 
{
  DMASSERT( nIndex >= 0 && nIndex < m_nCount );
  return m_ptrs[nIndex];
}

  /**
  * Assignment  operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oCopyFrom [in] - reference to DMVector instance
  * \return reference to DMVector
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class type>
DMVector<type>& DMVector<type>::operator = ( const DMVector& oCopyFrom )
{
  // free current
  Destroy();
  if ( !set_size( oCopyFrom.m_nCount ) )
    return *this;
    

  for ( int i = 0; i < oCopyFrom.m_nCount; i++ )
    (*this)[i] = oCopyFrom[i];
  
  return *this;
}

template<class type>
inline void DMVector<type>::DestructElement( int nIndex )
{
  m_ptrs[nIndex].~type();
}

template<class type>
inline void DMVector<type>::ConstructElement( int nIndex )
{
  ::new((void*)(m_ptrs + nIndex)) type;
}

  /**
  * Resizes the array. This function actually construct elements calling default constructor.  Elements can be reassign via [] operator.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nNewSize [in] - new array size. If new size is greater than current, new elements are constructed. 
  * If new size is smaller than current, extra elements are destructed.
  * \return success (1) or failure (0)
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class type>
int DMVector<type>::set_size( int nNewSize )
{
  // validate parameter
  if ( nNewSize < 0 )
    return 0; 

  // destroy deleted elements
  int nIndex = 0;
  for ( nIndex = m_nCount - 1; nIndex >= nNewSize; nIndex-- )
    DestructElement( nIndex );

  if ( !Reallocate( nNewSize ) )
  { // out of memory case
    if ( m_nCount > nNewSize )
      m_nCount = nNewSize;

    return 0;
  }

  // construct new elements
  for ( nIndex = m_nCount; nIndex < nNewSize; nIndex++ )
    ConstructElement( nIndex );
  
  m_nCount = nNewSize;

  return 1;
}


/*
 * Private function
 * Reallocates main storage, doesn't shrink the storage except when called with 0
 */
template<class type>
int DMVector<type>::Reallocate( int nNewSize )
{
  // reallocate main storage
  // don't shrink the storage except when called with 0

  if ( !nNewSize )
  {
    if ( m_ptrs )
      DmtMemFree( m_ptrs ); 
    m_ptrs = NULL;
    m_nAllocated = 0;
    return 1;
  }

  if ( nNewSize < m_nAllocated )
    return 1; // nothing to do

  int nNewAlloc = (nNewSize + 3) /4 * 4;

  type* pNewPtr = (type*)DmtMemAlloc( nNewAlloc* sizeof(type) );

  if ( !pNewPtr )
    return 0;
  
  memcpy( pNewPtr, m_ptrs, sizeof(type) * m_nCount );

  DmtMemFree( m_ptrs ); m_ptrs = pNewPtr;
  m_nAllocated = nNewAlloc;

  return 1;
}

  /**
  * Appends a new element to array. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptr [in] - new element to add
  * \return index of a new element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class type>
int DMVector<type>::push_back(const type& ptr)
{
  if ( !set_size( m_nCount + 1 ) )
    return m_nCount-1;

  (*this)[m_nCount-1] = ptr;
  return m_nCount-1;
}


  /**
  *  Detaches element. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return pointer to allocated element
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class type>
type* DMVector<type>::detach()
{
  type* ptr = m_ptrs;
  m_nCount = 0;
  m_nAllocated = 0;
  m_ptrs = NULL;
  return ptr;
}

  /**
  * Removes element from array. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param index  - element index; must be valid index.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

template<class type>
void DMVector<type>::remove(int index)
{
  if ( index < 0 || index >= m_nCount )
    return; // invlaid index - ignore it

  DestructElement( index );
  memmove( m_ptrs + index, m_ptrs + index + 1, (m_nCount - index - 1) * sizeof(type) );
  m_nCount--;
}

  /**
  * Looks for an  element in array. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptr [in] - reference to element
  * \return index of an element if found, otherwise (-1) .
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class type>
int DMVector<type>::find(const type& ptr) const
{
  for ( int nPos = 0; nPos < m_nCount; nPos++ )
  {
    if ( (*this)[nPos] == ptr )
      return nPos;
  }
  return -1;
}



////////////////////////////////////////////////////////////
// implementation   DMMap
//----------------------------------------------

  /**
  * Default constructor - no memory allocation performed.
  */
template<class Key, class Value>
inline DMMap<Key,Value>::DMMap()
{
}
  /** 
  * Destructor - freeing all dynamic resources 
  */
template<class Key, class Value>
inline DMMap<Key,Value>::~DMMap()
{
}

  
/**
  * Assignment operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oCopyFrom [in] - constant reference to DMMap
  * \return reference to the DMMap object (itself)
  * \par Prospective Clients:
  * Internal Classes.
  */
 
template<class Key, class Value>
DMMap<Key,Value>& DMMap<Key,Value>::operator = ( const DMMap& oCopyFrom )
{
  m_aKeys = oCopyFrom.m_aKeys;
  m_aValues = oCopyFrom.m_aValues;
  return *this;
}


  /**
  * Retrieves number of elements in the map.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return number of elements
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class Key, class Value>
inline int DMMap<Key,Value>::size() const
{
  return m_aKeys.size();
}

  /**
  * Retrieves a value based on key. The key must exist in the map.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] - element key
  * \return constant value
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class Key, class Value>
inline const Value& DMMap<Key,Value>::get(const Key & key) const
{
  int index=m_aKeys.find(key);
  DMASSERT( index >= 0 );

  return m_aValues[index];
}

  /**
  * Retrieves a value based on key. The key must exist in the map.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] - element key
  * \return value
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class Key, class Value>
inline Value& DMMap<Key,Value>::get(const Key & key)
{
  int index=m_aKeys.find(key);
  DMASSERT( index >= 0 );

  return m_aValues[index];
}

  /**
  * Adds or replaces value for the given key.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] -element key 
  * \param value [in] -element value 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class Key, class Value>
inline void  DMMap<Key,Value>::put(const Key & key, const Value & value)
{
  int index=m_aKeys.find(key);
  if (index<0)
  {
     int nSize = m_aKeys.size();
     
     if ( m_aKeys.push_back(key) != nSize )
      return;
     
     if ( m_aValues.push_back(value) != nSize )
     {
       m_aKeys.remove(nSize);
       return;
     }
  } 
  else
  {
     m_aValues[index] = value;
  }
}

/**
  * Removes element for the given key
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] -element key 
  * \return "true" in case of success, otherwise - "false"
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class Key, class Value>
inline bool DMMap<Key,Value>::remove(const Key & key)
{
  int index=m_aKeys.find(key);
  if (index>=0)
  {
     m_aKeys.remove(index);
     m_aValues.remove(index);
     return true;
  }
  return false;
}   

  /**
  * Looks up for the element for the given key. Set value if an element has been found.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param key [in] -element key 
  * \param rValue [out] -element value 
  * \return "true" if element has been found otherwise - "false"
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
template<class Key, class Value>
inline bool DMMap<Key,Value>::lookup( const Key & key, Value& rValue) const
{
  int index=m_aKeys.find(key);
  
  if (index < 0)
    return false;

  rValue = m_aValues[index];
  return true;
}

/**
* This is a helper class. Due to gcc limitations, declaring separate class instead of typedef
* produces smaller binary
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DMStringVector : public DMVector<DMString>
{
};
  
#endif
