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

#ifndef DM_UA_HANDLECOMMAND_H
#define DM_UA_HANDLECOMMAND_H

/*==================================================================================================

    Header Name: dm_ua_handlecommand.h

    General Description: This contains declaration of C functions corresponding to
                         SYNCML_DM_HandleCommand class.

==================================================================================================*/

extern "C" {
#include "sml.h"
}

#include "syncml_dm_data_types.h"
#include "dmSessionDefs.h"
#include "dm_tree_typedef.h"

/*==================================================================================================
  Function declarations
  These functions are declared for Callback Functions to be implemented by the Receiver.
  Callback Functions are declared in the SyncML toolkit "syncml_toolkit/sml/ghdr/sml.h".
==================================================================================================*/
extern Ret_t HandleStartMessage(InstanceID_t    id, 
                                VoidPtr_t       userData,
                                SmlSyncHdrPtr_t pContent);

extern Ret_t HandleEndMessage(InstanceID_t id,
                              VoidPtr_t    userData,
                              Boolean_t    final);

extern Ret_t HandleAddCommand(InstanceID_t id, 
                              VoidPtr_t    userData,
                              SmlAddPtr_t  pContent);

extern Ret_t HandleAlertCommand(InstanceID_t   id,
                                VoidPtr_t      userData,
                                SmlAlertPtr_t pContent);

extern Ret_t HandleDeleteCommand(InstanceID_t   id,
                                 VoidPtr_t      userData,
                                 SmlDeletePtr_t pContent);

extern Ret_t HandleCopyCommand(InstanceID_t id,
                               VoidPtr_t    userData,
                               SmlCopyPtr_t param);

extern Ret_t HandleGetCommand(InstanceID_t id,
                              VoidPtr_t    userData,
                              SmlGetPtr_t  pContent);

extern Ret_t HandleExecCommand(InstanceID_t id,
                               VoidPtr_t    userData,
                               SmlExecPtr_t  pContent);

extern Ret_t HandleReplaceCommand(InstanceID_t id,
                                  VoidPtr_t    userData,
                                  SmlReplacePtr_t  pContent);

extern Ret_t HandleStartSequenceCommand(InstanceID_t id,
                                        VoidPtr_t    userData,
                                        SmlSequencePtr_t  pContent);

extern Ret_t HandleEndSequenceCommand(InstanceID_t id,
                                      VoidPtr_t    userData);

extern Ret_t HandleStartAtomicCommand(InstanceID_t id,
                                      VoidPtr_t    userData,
                                      SmlAtomicPtr_t  pContent);

extern Ret_t HandleEndAtomicCommand(InstanceID_t id,
                                    VoidPtr_t    userData);

extern Ret_t HandleStatusCommand(InstanceID_t id,
                                 VoidPtr_t    userData,
                                 SmlStatusPtr_t  pContent);

extern SYNCML_DM_RET_STATUS_T VerifyProtocolVersion( SmlSyncHdrPtr_t pContent);

bool VerifyAlertItems(SmlAlertPtr_t  pContent);

#endif /* DM_UA_HANDLECOMMAND_H */
