/*******************************************************************************
*   NEM Wallet
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

#include "review_menu.h"
#include <os_io_seproxyhal.h>
#include <ux.h>
#include "common.h"
#include "nem/format/readers.h"
#include "nem/format/fields.h"
#include "nem/format/format.h"
#include "glyphs.h"

char fieldName[MAX_FIELDNAME_LEN];
char fieldValue[MAX_FIELD_LEN];

result_t *transaction;
result_action_t approval_menu_callback;

const ux_flow_step_t* ux_review_flow[MAX_FIELD_COUNT + 3];

static void update_content(int stackSlot);

UX_STEP_NOCB_INIT(
        ux_review_flow_step,
        bnnn_paging,
        update_content(stack_slot),
        {
            fieldName,
            fieldValue
        });

UX_STEP_VALID(
        ux_review_flow_sign,
        pn,
        approval_menu_callback(OPTION_SIGN),
        {
            &C_icon_validate_14,
            "Approve",
        });

UX_STEP_VALID(
        ux_review_flow_reject,
        pn,
        approval_menu_callback(OPTION_REJECT),
        {
            &C_icon_crossmark,
            "Reject",
        });

static void update_title(const field_t *field) {
    memset(fieldName, 0, MAX_FIELDNAME_LEN);
    resolve_fieldname(field, fieldName);
}


static void update_value(const field_t *field) {
    memset(fieldValue, 0, MAX_FIELD_LEN);
    format_field(field, fieldValue);
}

static void update_content(int stackSlot) {
    int stepIndex = G_ux.flow_stack[stackSlot].index;
    const field_t *field = &transaction->fields[stepIndex];
    update_title(field);
    update_value(field);
#ifdef HAVE_PRINTF
    PRINTF("\nPage %d - Title: %s - Value: %s\n", stepIndex, fieldName, fieldValue);
#endif
}

void display_review_menu(result_t *transactionParam, result_action_t callback) {
    transaction = transactionParam;
    approval_menu_callback = callback;

    for (int i = 0; i < transaction->numFields; ++i) {
        ux_review_flow[i] = &ux_review_flow_step;
    }

    ux_review_flow[transaction->numFields + 0] = &ux_review_flow_sign;
    ux_review_flow[transaction->numFields + 1] = &ux_review_flow_reject;
    ux_review_flow[transaction->numFields + 2] = FLOW_END_STEP;

    ux_flow_init(0, ux_review_flow, NULL);
}
