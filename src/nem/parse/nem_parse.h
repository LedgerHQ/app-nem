/*******************************************************************************
*    NEM Wallet
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
#ifndef LEDGER_APP_NEM_NEMPARSE_H
#define LEDGER_APP_NEM_NEMPARSE_H

#include "limitations.h"
#include "nem/format/fields.h"
#include "nem/nem_helpers.h"

typedef struct result_t {
    uint8_t numFields;
    field_t fields[MAX_FIELD_COUNT];
} result_t;

typedef struct parse_context_t {
    uint8_t version;
    uint16_t transactionType;
    uint8_t *data;
    result_t result;
    uint32_t length;
    uint32_t offset;
} parse_context_t;

int parse_txn_context(parse_context_t *parseContext);

#endif //LEDGER_APP_NEM_NEMPARSE_H
