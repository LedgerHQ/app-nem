/*******************************************************************************
*   NEM Wallet
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

#include "transaction.h"
#include "ui/transaction/review_menu.h"
#include "ui/other/loading.h"

extern action_t approval_action;
extern action_t rejection_action;

void on_approval_menu_result(unsigned int result) {
    switch (result) {
        case OPTION_SIGN:
            execute_async(approval_action, "Signing...");
            break;
        case OPTION_REJECT:
            rejection_action();
            break;
        default:
            rejection_action();
    }
}

void review_transaction(result_t *transaction, action_t onApprove, action_t onReject) {
    approval_action = onApprove;
    rejection_action = onReject;

    display_review_menu(transaction, on_approval_menu_result);
}
