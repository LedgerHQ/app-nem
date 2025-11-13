/*******************************************************************************
 *    NEM Wallet
 *    (c) 2020 Ledger
 *    (c) 2020 FDS
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
#include "remote_ui.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "limitations.h"
#include "idle_menu.h"
#include "nem_helpers.h"
#include "nbgl_use_case.h"
#include "display.h"

extern action_t approval_action;
extern action_t rejection_action;

// called when long press button on 3rd page is long-touched or when reject footer is touched
static void review_choice(bool confirm) {
    if (confirm) {
        approval_action();
    } else {
        rejection_action();
    }
}

void display_remote_account_confirmation_ui(action_t onApprove, action_t onReject) {
    approval_action = onApprove;
    rejection_action = onReject;

    nbgl_useCaseChoice(&ICON_APP_HOME,
                       "Export delegated\nharvesting key?",
                       NULL,
                       "Approve",
                       "Reject",
                       review_choice);
}

void display_remote_account_done(bool validated) {
    if (validated) {
        nbgl_useCaseStatus("KEY\nEXPORTED", true, display_idle_menu);
    } else {
        nbgl_useCaseStatus("Request rejected", false, display_idle_menu);
    }
}
