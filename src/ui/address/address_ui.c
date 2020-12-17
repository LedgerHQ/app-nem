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
#include "address_ui.h"
#include <os_io_seproxyhal.h>
#include <ux.h>
#include "limitations.h"
#include "ui/main/idle_menu.h"
#include "nem/nem_helpers.h"
#include "glyphs.h"

extern char fieldValue[MAX_FIELD_LEN];

action_t approval_action;
action_t rejection_action;

UX_STEP_NOCB(
        ux_display_address_flow_1_step,
        pnn,
        {
            &C_icon_eye,
            "Verify",
            "address",
        });

UX_STEP_NOCB(
        ux_display_address_flow_2_step,
        bnnn_paging,
        {
            "Address",
            fieldValue,
        });

UX_STEP_VALID(
        ux_display_address_flow_3_step,
        pb,
        approval_action(),
        {
            &C_icon_validate_14,
            "Approve",
        });

UX_STEP_VALID(
        ux_display_address_flow_4_step,
        pb,
        rejection_action(),
        {
            &C_icon_crossmark,
            "Reject",
        });

UX_FLOW(ux_display_address_flow,
       &ux_display_address_flow_1_step,
       &ux_display_address_flow_2_step,
       &ux_display_address_flow_3_step,
       &ux_display_address_flow_4_step
);

void display_address_confirmation_ui(char* address, action_t onApprove, action_t onReject) {
    approval_action = onApprove;
    rejection_action = onReject;

    explicit_bzero(fieldValue, MAX_FIELD_LEN);
    strncpy(fieldValue, address, NEM_PRETTY_ADDRESS_LENGTH);
    ux_flow_init(0, ux_display_address_flow, NULL);
}
