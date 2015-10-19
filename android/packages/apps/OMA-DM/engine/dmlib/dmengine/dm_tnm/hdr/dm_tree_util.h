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

#ifndef _DM_TREE_UTIL_H
#define _DM_TREE_UTIL_H

/*========================================================================
        Header Name: dm_tree_util.h

    General Description: This file gives the definitions of UTILITY 
                      functions provided  by the DMTNM module.
========================================================================*/

#include "dm_tree_typedef.h" /* for defn of plug-in get ret structure */
#include "dm_tree_class.H"
#include "dmt.hpp"

#define FDR_URI "./ManagedObjects/LAWMO/Operations/FactoryReset"

SYNCML_DM_RET_STATUS_T dmBuildData(SYNCML_DM_FORMAT_T format, 
                                      const DMBuffer & oData, 
                                      DmtData & oDmtData);

SYNCML_DM_RET_STATUS_T dmConvertDataMap(CPCHAR path, 
                                              const DMMap<DMString, DmtData>& mapNodes,
                                              DMMap<DMString, UINT32>& newChildrenMap);

void dmFreeGetMap( DMMap<DMString, UINT32>& getDataMap);

void dmFreeAddMap( DMMap<DMString, UINT32>& addDataMap);

#ifdef __cplusplus
extern "C" {
#endif

extern DMTree dmTreeObj;
SYNCML_DM_RET_STATUS_T dmGetNodeValue(CPCHAR uriPath, DmtData& oDmtData);

#ifdef __cplusplus
}
#endif

#endif  /*DM_TREE_UTIL_H*/
