/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef RTK_INCLUDE_BLUETOOTH_MP_OPCODE_H
#define RTK_INCLUDE_BLUETOOTH_MP_OPCODE_H

enum _BT_MP_OPCODE{

    BT_MP_OP_HCI_SEND_CMD = 0x00,
    BT_MP_OP_DUT_MODE_CONFIGURE = 0x01,



    BT_MP_OP_USER_DEF_GetPara = 0x80, 
    BT_MP_OP_USER_DEF_SetPara1,
    BT_MP_OP_USER_DEF_SetPara2,
    BT_MP_OP_USER_DEF_SetHit,
    BT_MP_OP_USER_DEF_SetDacTable,
    BT_MP_OP_USER_DEF_SetGainTable,
    BT_MP_OP_USER_DEF_Exec,
    BT_MP_OP_USER_DEF_ReportTx,
    BT_MP_OP_USER_DEF_ReportRx,
    BT_MP_OP_USER_DEF_REG_RF,
    BT_MP_OP_USER_DEF_REG_MD

};

#endif /* ANDROID_INCLUDE_BLUETOOTH_H */
