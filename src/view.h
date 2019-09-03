/*******************************************************************************
*   Ledger Blue
*   (c) 2016 Ledger
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#pragma once

#include "os.h"
#include "cx.h"
#include "view_ctl.h"

enum UI_STATE {
    UI_IDLE,
    UI_TRANSACTION,
    UI_ADDRESS
};

extern enum UI_STATE view_uiState;

//------ Delegates definitions
typedef void (*delegate_reject_tx)();

typedef void (*delegate_sign_tx)();

void view_set_event_handlers(viewctl_delegate_update ehUpdate,
                                delegate_sign_tx ehSign,
                                delegate_reject_tx ehReject);

void view_init(void);
void view_idle(unsigned int ignored);

void view_display_tx_menu(unsigned int ignored);
void view_addr_menu(unsigned int ignored);

void view_tx_show(unsigned int unused);
void view_addr_show(unsigned int unused);