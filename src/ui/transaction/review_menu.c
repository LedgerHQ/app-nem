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
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "common.h"
#include "os.h"
#include "fields.h"
#include "app_format.h"
#include "idle_menu.h"
#include "nbgl_use_case.h"
#include "display.h"

result_t *transaction;
result_action_t approval_menu_callback;

static nbgl_contentTagValue_t pair = {0};
static nbgl_contentTagValueList_t pairList = {0};

#define MAX_TAG_VALUE_PAIRS_DISPLAYED 4

typedef struct review_argument_t {
    char name[MAX_FIELDNAME_LEN];
    char value[MAX_FIELD_LEN];
} review_argument_t;

static review_argument_t bkp_args[MAX_TAG_VALUE_PAIRS_DISPLAYED];

// called when long press button on 3rd page is long-touched or when reject footer is touched
static void review_choice(bool confirm) {
    approval_menu_callback(confirm ? OPTION_SIGN : OPTION_REJECT);
}

// function called by NBGL to get the pair indexed by "index"
static nbgl_contentTagValue_t *get_review_pair(uint8_t index) {
    const field_t *field = &transaction->fields[index];

    // Backup review argument as MAX_TAG_VALUE_PAIRS_DISPLAYED can be displayed
    // simultaneously and their content must be store on app side buffer as
    // only the buffer pointer is copied by the SDK and not the buffer content.
    uint8_t bkp_index = index % MAX_TAG_VALUE_PAIRS_DISPLAYED;

    resolve_fieldname(field, bkp_args[bkp_index].name);
    format_field(field, bkp_args[bkp_index].value);

    explicit_bzero(&pair, sizeof(nbgl_contentTagValue_t));
    pair.item = bkp_args[bkp_index].name;
    pair.value = bkp_args[bkp_index].value;

    PRINTF("\nPair %d - Title: %s - Value: %s\n", index, pair.item, pair.value);

    return &pair;
}

void display_review_menu(result_t *transactionParam, result_action_t callback) {
    transaction = transactionParam;
    approval_menu_callback = callback;

    explicit_bzero(&pairList, sizeof(nbgl_contentTagValueList_t));
    pairList.nbPairs = transaction->numFields;
    pairList.callback = get_review_pair;

    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &ICON_APP_HOME,
                       "Review transaction",
                       NULL,
                       "Sign transaction",
                       review_choice);
}

void display_review_done(bool validated) {
    if (validated) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_SIGNED, display_idle_menu);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED, display_idle_menu);
    }
}
