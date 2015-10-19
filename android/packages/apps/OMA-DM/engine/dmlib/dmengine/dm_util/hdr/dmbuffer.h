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

#ifndef __DMBUFFER_H__
#define __DMBUFFER_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "xpl_Types.h"
#include "dmstring.h"

/**
 * A simple Buffer class.
 */
class DMBuffer
{
 public:
  DMBuffer();
  ~DMBuffer();

  DMBuffer & operator=(const DMBuffer& pBuffer); 
  
  inline UINT8* getBuffer() const {return m_pBuf;}
  
  inline UINT32 getSize() const {return m_nSize;}
  
  UINT8 * allocate(UINT32 nCapacity);
  
  UINT8 * assign(CPCHAR szStr);
  
  UINT8 * assign(const UINT8 * pBuffer, INT32 size);
  void  append(const UINT8 * pBuffer, INT32 size);
  
  inline UINT8 * assign(CPCHAR szStr, INT32 size) { return assign((UINT8*)szStr,size); }
  
  void clear();
  void free();
  void reset();
  
  void setSize(UINT32 size);

  void copyTo(UINT8 **ppBuffer) const;

  void copyTo(INT32 offset,INT32 size, DMBuffer& pBuffer) const;

  inline void copyTo(char ** ppStr) const { copyTo((UINT8**)ppStr); }

  void copyTo(char * pStr) const;

  void copyTo(DMString & sStr) const;

  BOOLEAN compare(CPCHAR pStr) const; 

  BOOLEAN compare(CPCHAR pStr, UINT32 len) const; 

  BOOLEAN empty() { return ((m_nSize == 0) ? TRUE : FALSE); }

  void attach(UINT8 * pBuffer, INT32 nCapacity);

  UINT8 * detach();

  
 private:
  UINT8 *m_pBuf;
  UINT32 m_nSize;
  UINT32 m_nCapacity;
};

#endif   //End of include File
