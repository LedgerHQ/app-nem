/*******************************************************************************
*   NEM Wallet
*   (c) 2017 Ledger
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
#ifndef LEDGER_APP_NEM_ENTRY_H
#define LEDGER_APP_NEM_ENTRY_H

/**
 * Structure with fields of APDU command.
 */
typedef struct {
    uint8_t cla;    /// Instruction class
    uint8_t ins;    /// Instruction code
    uint8_t p1;     /// Instruction parameter 1
    uint8_t p2;     /// Instruction parameter 2
    uint8_t lc;     /// Length of command data
    uint8_t *data;  /// Command data
} command_t;

bool apdu_parser(command_t *cmd, uint8_t *buf, size_t buf_len);

void handle_apdu(command_t *cmd, volatile unsigned int *flags, volatile unsigned int *tx);

#endif //LEDGER_APP_NEM_ENTRY_H
