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

#ifndef __DMSTRING_H__
#define __DMSTRING_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/**  
 \file dmstring.h
 \brief The dmstring.h header file contains String type definition (class String)
*/

#include "xpl_Types.h"

/**
* A simple String class similar to Java String and STL string with limited functionality
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*
*/
class DMString
{
 public:
  /**
  * Default constructor - no memory allocation performed.
  */
  DMString() {m_pStr = (char*)XPL_NULL;}

  /**
  * Construct a string based on input string (similar to strdup()). The memory for the size of parameter "szStr" will be allocated.
  * \param szStr [in] - pointer to null terminated string
  */
  DMString(CPCHAR szStr);

  /**
  * Construct a string based on input string and length. The memory for the "length" bytes  will be allocated.
  * \param szStr [in] - pointer to string
  * \param nLen [in] - an integer argument presents string length
  */
  DMString(CPCHAR szStr, int nLen);
  
  /**
  * Copy constructor. The memory for the size of parameter "str" will be allocated.
  * \param str [in] - constant reference to DMString
  */
  DMString(const DMString& str);

  /** 
  * Destructor - freeing all dynamic resources 
  */
  ~DMString();

  /**
  * Allows direct modification of the buffer.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \warning Be carefully, don't overrun the buffer !!!
  * \param  nPos [in] - integer index
  * \param  c [in] - new character
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  void SetAt(INT32 nPos, char c );

  /**
  * Replaces all old character occurrences with the new character
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param  oldCh [in] - old character which should be replaced
  * \param  newCh [in] - a new character
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  void replaceAll(char oldCh, char newCh);

  /**
  * Assignment operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param str [in] - constant reference to DMString
  * \return reference to constant DMString (itself)
  * \par Prospective Clients:
  * Internal Classes.
  */
  inline DMString & operator=(const DMString& str) { return (*this) = str.m_pStr;}
  /**
  * Assignment operator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szStr [in] - pointer to null terminated string
  * \return reference to constant DMString
  * \par Prospective Clients:
  * Internal Classes.
  */
  DMString & operator=(CPCHAR szStr);

  /**
  * Saves casting DMString to CPCHAR. 
  * \par Important Notes:
  * - Note: this never returns null; and empty string ("") will be returned instead
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return  pointer to null terminated string.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  inline CPCHAR c_str() const { return m_pStr ? m_pStr : "";}

  /**
  * ()
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return  constant character pointer, never returns null - empty string "" returned instead.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  inline operator const char* () const { return c_str();}

  /**
  * Comparison operator (equally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param s2 [in] - reference to constant DMString
  * \return boolean result of comparison
  * \par Prospective Clients:
  * Internal Classes.
  */
  inline BOOLEAN operator==(const DMString & s2) const {return this->operator == (s2.m_pStr);}
  
  /**
  * Comparison operator (equally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szStr [in] - pointer to null terminated string
  * \return boolean result of comparison
  * \par Prospective Clients:
  * Internal Classes.
  */
  BOOLEAN operator==(CPCHAR szStr) const;
  
  /**
  * Comparison operator (unequally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param s2 [in] - reference to constant DMString
  * \return boolean result of comparison
  * \par Prospective Clients:
  * Internal Classes.
  */
  inline BOOLEAN operator!=(const DMString & s2) const {return this->operator != (s2.m_pStr);}
  
  /**
  * Comparison operator (unequally)
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szStr [in] - pointer to null terminated string
  * \return boolean result of comparison
  * \par Prospective Clients:
  * Internal Classes.
  */
  inline BOOLEAN operator!=(CPCHAR szStr) const {return ! operator == ( szStr );}

/**
  * Comparison operator. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param str2 [in] - pointer to DMString to be compared with
  * \return integer result of comparison <br>
  *  Result may be: <br>
  *  less then zero - this string is less then parameter str2 <br>
  *  greater then zero - this string is greater then parameter str2 <br>
  *  equal to  zero - this string and parameter str2 are equal<br>
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  int CompareNoCase( const DMString& str2 ) const;

  /** 
  * String concatenation
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param s2 [in] -reference to constant DMString 
  * \return reference to constant DMString result of concatenation
  * \par Prospective Clients:
  * Internal Classes.
  */
  inline DMString& operator += (const DMString & s2) {return (*this) += s2.m_pStr;}
  
  /**
  * String concatenation
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szStr [in] - pointer to null terminated string
  * \return reference to constant DMString result of concatenation
  * \par Prospective Clients:
  * Internal Classes.
  */
  DMString& operator += (CPCHAR szStr);

  /**
  * String concatenation
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szStr [in] - pointer to null terminated string
  * \return reference to constant DMString result of concatenation
  * \par Prospective Clients:
  * Internal Classes.
  */
  DMString& operator += (char c);

  /**
  * Gets  buffer 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return character pointer to this String
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  inline char* GetBuffer() {return m_pStr;}

  
  /**
  * Allocates  buffer for the provided length
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param nLen [in] - buffer length
  * \return character pointer
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  char* AllocateBuffer( int nLen );
  
  /**
  * Gets length for String
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return length of the string (integer)
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  INT32 length() const;
  
  /**
  * Verifis if String is empty
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return true if string is empty
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  inline BOOLEAN empty() const {return m_pStr == XPL_NULL; }
  
  /**
  * Sets  string/substring  based on the input string and given length
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param pStr [in] - pointer to string
  * \param nLen [in] - an integer argument presents  length
  * \return boolean result of operation
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  BOOLEAN assign(CPCHAR pStr, INT32 nLen);
  
  /**
  * Gets rid of the white space at the beginning and at the end of the string.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void trim();

  /**
  * Clears the string contents.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void clear();
 
  /** 
  * URI encoding/decoding support. Performs encoding of non-standard URI symbols according to the RFC 2396
  * \warning This function is for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return false if fails (out of memory)
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  BOOLEAN Encode();
  
  /** 
  * Performs URI string decoding according to the RFC2396. <br> 
  * \warning This function is for internal usage only!!!
  * \par Important Notes:
  * -Note: If function fails, string can be partially changed, since inplace decoding has been already performed.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return false if fails (invalid string).
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  BOOLEAN Decode();

  /** 
  * Removes reference to string from class, but does not deallocate memory. 
  * \warning This function is for internal usage only!!!
  * \par Important Notes:
  * -Note:  Should be used in pair with function attach()
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return pointer to null terminated string
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  CPCHAR detach();

  /** 
  * Resets pointer to string
  * \warning This function is for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szStr [in] - pointer to null terminated string
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void attach(CPCHAR szStr);

/** 
  *  Removes rest of the string sourceStr after first occurrence on the separator 
  * \warning This function is for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sourceStr [in] - string that should be truncated 
  * \param seperator [in] - separator that point to part that should be removed
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  void RemoveSufix(CPCHAR sourceStr,  int seperator);
   
 private:
  char  *m_pStr; 
};

/** 
  *  Overload operator; concatenates two strings
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param str1 [in] - first string
  * \param str2 [in] - second string
  * \return result concatenation str1 and str2
  * \par Prospective Clients:
  * Internal Classes.
  */
inline 
const DMString operator +( const DMString& str1, const DMString str2)
{
  DMString str3( str1 );
  
  return str3 += str2;
}

#endif
