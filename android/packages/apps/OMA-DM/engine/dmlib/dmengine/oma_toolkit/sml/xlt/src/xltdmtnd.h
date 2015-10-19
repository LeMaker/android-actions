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

/*--------------------------------------------------------------------------------------------------

   Header Name: smldmtnd.h

   General Description: OMA DM TND specific type definitions   

--------------------------------------------------------------------------------------------------*/

#ifndef _XLT_DMTND_H
#define _XLT_DMTND_H

/*************************************************************************/
/*  Definitions                                                          */
/*************************************************************************/

#include "xlttagtbl.h"
#include "xltdec.h"
#include "xltenc.h"

Ret_t buildDmTndNodesCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);
Ret_t buildDmTndCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);
Ret_t buildMgmtTreeCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);
Ret_t dmtndEncBlock(XltTagID_t tagId, XltRO_t reqOptFlag, const VoidPtr_t pContent, SmlEncoding_t enc, BufferMgmtPtr_t pBufMgr, SmlPcdataExtension_t attFlag);

#endif	  /* _XLT_DMTND_H */
