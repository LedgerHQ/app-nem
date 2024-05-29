/*******************************************************************************
*   NEM Wallet
*   (c) 2017 Ledger
*   (c) 2020 FDS
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
#include <os.h>
#include "entry.h"
#include "constants.h"
#include "global.h"
#include "messages/get_public_key.h"
#include "messages/sign_transaction.h"
#include "messages/get_remote_account.h"
#include "messages/get_app_configuration.h"

static unsigned char lastINS = 0;

bool apdu_parser(command_t *cmd, uint8_t *buf, size_t buf_len) {
    // Check minimum length and Lc field of APDU command
    if (buf_len < OFFSET_CDATA || buf_len - OFFSET_CDATA != buf[OFFSET_LC]) {
        return false;
    }

    cmd->cla = buf[OFFSET_CLA];
    cmd->ins = buf[OFFSET_INS];
    cmd->p1 = buf[OFFSET_P1];
    cmd->p2 = buf[OFFSET_P2];
    cmd->lc = buf[OFFSET_LC];
    cmd->data = (buf[OFFSET_LC] > 0) ? buf + OFFSET_CDATA : NULL;

    return true;
}

void handle_apdu(command_t *cmd, volatile unsigned int *flags, volatile unsigned int *tx) {
    unsigned short sw = 0;

    BEGIN_TRY {
        TRY {
            if (cmd->cla != CLA) {
                THROW(SW_CLA_NOT_SUPPORTED);
            }

            // Reset transaction context before starting to parse a new APDU message type.
            // This helps protect against "Instruction Change" attacks
            if (cmd->ins != lastINS) {
                reset_transaction_context();
            }

            lastINS = cmd->ins;

            switch (cmd->ins) {
                case INS_GET_PUBLIC_KEY:
                    handle_public_key(cmd->p1, cmd->p2, cmd->data, cmd->lc,
                                      flags, tx);
                    break;

                case INS_SIGN:
                    handle_sign(cmd->p1, cmd->p2, cmd->data, cmd->lc,
                                flags);
                    break;

                case INS_GET_REMOTE_ACCOUNT:
                    handle_remote_private_key(cmd->p1, cmd->p2, cmd->data, cmd->lc,
                                              flags, tx);
                    break;

                case INS_GET_APP_CONFIGURATION:
                    handle_app_configuration(tx);
                    break;

                default:
                    THROW(SW_INS_NOT_SUPPORTED);
                    break;
            }
        }
        CATCH_OTHER(e) {
            switch (e & 0xF000u) {
                case 0x6000:
                    // Wipe the transaction context and report the exception
                    sw = e;
                    reset_transaction_context();
                    break;
                case 0x9000:
                    // All is well
                    sw = e;
                    break;
                default:
                    // Internal error, wipe the transaction context and report the exception
                    sw = 0x6800u | (e & 0x7FFu);
                    reset_transaction_context();
                    break;
            }
            // Unexpected exception => report
            G_io_apdu_buffer[*tx] = sw >> 8u;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY {
        }
    }
    END_TRY;
}
