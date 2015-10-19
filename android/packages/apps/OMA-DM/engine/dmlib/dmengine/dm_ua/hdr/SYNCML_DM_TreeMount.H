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

#ifndef SYNCML_DM_TREEMOUNT_H
#define SYNCML_DM_TREEMOUNT_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SYNCML_DM_TreeMount.H

    General Description: This file contains declaration of the SYNCML_DM_TreeMount_ListItem and 
                         SYNCML_DM_TreeMount classes.

==================================================================================================*/

#include "dm_tree_node_class.H"
#include "dmvector.h"
#include "dmdefs.h"   

class CEnv;
class DMTree;

/* SYNCML_DM_TreeMount class provides functions to add, delete and retrieve tree mount
 * information. */
class SYNCML_DM_TreeMount 
{
public:
                              SYNCML_DM_TreeMount();
                              
  /* DMArchiver calls this method to retrieve tree mount information to deserialize the tree. */
  void GetTreeMountEntry (CPCHAR &p_Uri,  CPCHAR& p_TreePath,  UINT16   index) const ;

  BOOLEAN IsMountPointEnabled(CPCHAR pUri) const ;
  
  SYNCML_DM_RET_STATUS_T MountTree( CEnv* env, DMTree* tree );

  void UnMountTree(void) ;

private:
  /* UA API calls this function to add the tree information into the tree mount list. */
  SYNCML_DM_RET_STATUS_T TreeAddToMountList (CPCHAR pUri, CPCHAR pTreePath) ;
  
private:
  DMStringVector              m_astrURIs;
  DMStringVector              m_astrPaths;
  DMTree                      *m_pTree;
};

/*================================================================================================*/
#endif /* SYNCML_DM_TreeMount_H */
