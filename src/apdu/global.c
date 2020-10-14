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
#include "global.h"
#include "messages/sign_transaction.h"

transaction_context_t transactionContext;
sign_state_e signState;

void reset_transaction_context() {
    explicit_bzero(&parseContext, sizeof(parse_context_t));
    explicit_bzero(&transactionContext, sizeof(transaction_context_t));
    signState = IDLE;
}
