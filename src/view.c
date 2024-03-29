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

#include "view.h"
#include "view_templates.h"
#include "view_ctl.h"

#include "glyphs.h"
#include "bagl.h"

#include <string.h>
#include <stdio.h>

#define TRUE  1
#define FALSE 0

ux_state_t ux;
enum UI_STATE view_uiState;

void view_tx_show();
void view_sign_transaction(unsigned int unused);
void reject(unsigned int unused);

//------ View elements
const ux_menu_entry_t menu_main[];
const ux_menu_entry_t menu_about[];

const ux_menu_entry_t menu_transaction_info[] = {
        {NULL, view_tx_show, 0, NULL, "View transaction", NULL, 0, 0},
        {NULL, view_sign_transaction, 0, NULL, "Sign transaction", NULL, 0, 0},
        {NULL, reject, 0, &C_icon_back, "Reject", NULL, 60, 40},
        UX_MENU_END
};

const ux_menu_entry_t menu_address_info[] = {
        {NULL, view_sign_transaction, 0, NULL, "Export public key", NULL, 0, 0},
        {NULL, reject, 0, &C_icon_back, "Reject", NULL, 60, 40},
        UX_MENU_END
};

const ux_menu_entry_t menu_main[] = {
        {NULL, NULL, 0, &C_icon_app, "Use wallet to", "view accounts", 33, 12},
        {menu_about, NULL, 0, NULL, "About", NULL, 0, 0},
        {NULL, os_sched_exit, 0, &C_icon_dashboard, "Quit app", NULL, 50, 29},
        UX_MENU_END
};

const ux_menu_entry_t menu_about[] = {
        {NULL, NULL, 0, NULL, "Version", APPVERSION, 0, 0},
        {menu_main, NULL, 2, &C_icon_back, "Back", NULL, 61, 40},
        UX_MENU_END
};
//------ View elements

//------ Event handlers
viewctl_delegate_update ehUpdateTx = NULL;
delegate_sign_tx ehSignTx = NULL;
delegate_reject_tx ehRejectTx = NULL;

void view_set_event_handlers(viewctl_delegate_update ehUpdate,
                                delegate_sign_tx ehSign,
                                delegate_reject_tx ehReject) {
    ehSignTx = ehSign;
    ehRejectTx = ehReject;
    ehUpdateTx = ehUpdate;
}

// ------ Event handlers

void io_seproxyhal_display(const bagl_element_t *element) {
    io_seproxyhal_display_default((bagl_element_t *) element);
}

void view_tx_show(unsigned int unused) {
    UNUSED(unused);

    viewctl_start(ehUpdateTx,
                  NULL,
                  view_display_tx_menu,
                  0);
}

void view_addr_show(unsigned int unused) {
    UNUSED(unused);

    viewctl_start(ehUpdateTx,
                  NULL,
                  view_addr_menu,
                  0);
}

/////////////////////////////////

void view_sign_transaction(unsigned int unused) {
    UNUSED(unused);

    if (ehSignTx != NULL) {
        ehSignTx();
    }
}

void reject(unsigned int unused) {
    UNUSED(unused);

    if (ehRejectTx != NULL) {
        ehRejectTx();
    }
}

void view_init(void) {
    UX_INIT();
    view_uiState = UI_IDLE;
}

void view_idle(unsigned int ignored) {
    view_uiState = UI_IDLE;
    UX_MENU_DISPLAY(0, menu_main, NULL);
}

void view_display_tx_menu(unsigned int ignored) {
    view_uiState = UI_TRANSACTION;
    UX_MENU_DISPLAY(0, menu_transaction_info, NULL);
}

void view_addr_menu(unsigned int ignored) {
    view_uiState = UI_ADDRESS;
    UX_MENU_DISPLAY(0, menu_address_info, NULL);
}