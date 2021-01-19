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
#include <os_io_seproxyhal.h>
#include <ux.h>
#include "limitations.h"
#include "idle_menu.h"
#include "nem_helpers.h"
#include "glyphs.h"

extern char fieldValue[MAX_FIELD_LEN];

extern action_t approval_action;
extern action_t rejection_action;

UX_STEP_NOCB(
        ux_display_remote_account_flow_1_step,
        bnnn_paging,
        {
            "Export delegated",
            "harvesting key?",
        });

UX_STEP_VALID(
        ux_display_remote_account_flow_2_step,
        pb,
        approval_action(),
        {
            &C_icon_validate_14,
            "Approve",
        });

UX_STEP_VALID(
        ux_display_remote_account_flow_3_step,
        pb,
        rejection_action(),
        {
            &C_icon_crossmark,
            "Reject",
        });

UX_FLOW(ux_display_remote_account_flow,
       &ux_display_remote_account_flow_1_step,
       &ux_display_remote_account_flow_2_step,
       &ux_display_remote_account_flow_3_step
);

void display_remote_account_confirmation_ui(action_t onApprove, action_t onReject) {
    approval_action = onApprove;
    rejection_action = onReject;

    ux_flow_init(0, ux_display_remote_account_flow, NULL);
}
