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
#include "sign_transaction.h"
#include "os.h"
#include "io.h"
#include "global.h"
#include "constants.h"
#include "nem_helpers.h"
#include "idle_menu.h"
#include "review_menu.h"
#include "transaction.h"

#define PREFIX_LENGTH 4
#define ED25519_SIGNATURE_LENGTH 64

parse_context_t parseContext;

void sign_transaction(void) {
    uint8_t privateKeyData[64];
    cx_ecfp_private_key_t privateKey;
    int error = SWO_PARAMETER_ERROR_NO_INFO;

    if (signState != PENDING_REVIEW) {
        reset_transaction_context();
        display_idle_menu();
        return;
    }

    // Abort if we accidentally end up here again after the transaction has already been signed
    if (parseContext.data == NULL) {
        display_idle_menu();
        return;
    }

    io_seproxyhal_io_heartbeat();
    CX_CHECK(os_derive_bip32_with_seed_no_throw(HDW_ED25519_SLIP10,
                                                CX_CURVE_Ed25519,
                                                transactionContext.bip32Path,
                                                transactionContext.pathLength,
                                                privateKeyData,
                                                NULL,
                                                (unsigned char *) "ed25519-keccak seed",
                                                19));
    CX_CHECK(cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519,
                                               privateKeyData,
                                               NEM_PRIVATE_KEY_LENGTH,
                                               &privateKey));
    io_seproxyhal_io_heartbeat();
    CX_CHECK(cx_eddsa_sign_no_throw(&privateKey,
                                    transactionContext.algo,
                                    transactionContext.rawTx,
                                    transactionContext.rawTxLength,
                                    G_io_apdu_buffer,
                                    IO_APDU_BUFFER_SIZE));

    if (io_send_response_pointer(G_io_apdu_buffer, ED25519_SIGNATURE_LENGTH, SWO_SUCCESS) < 0) {
        goto end;
    }
    display_review_done(true);
end:
    explicit_bzero(privateKeyData, sizeof(privateKeyData));
    explicit_bzero(&privateKey, sizeof(privateKey));
    // Always reset transaction context after a transaction has been signed
    reset_transaction_context();
}

void reject_transaction(void) {
    if (signState != PENDING_REVIEW) {
        reset_transaction_context();
        display_idle_menu();
        return;
    }

    io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
    // Reset transaction context and display back the original UX
    reset_transaction_context();

    display_review_done(false);
}

bool isFirst(uint8_t p1) {
    return (p1 & P1_MASK_ORDER) == 0;
}

bool hasMore(uint8_t p1) {
    return (p1 & P1_MASK_MORE) != 0;
}

int handle_packet_content(const command_t *cmd) {
    uint16_t totalLength = PREFIX_LENGTH + parseContext.length + cmd->lc;
    if (totalLength > MAX_RAW_TX) {
        // Abort if the user is trying to sign a too large transaction
        return SWO_WRONG_DATA_LENGTH;
    }

    // Append received data to stored transaction data
    memcpy(parseContext.data + parseContext.length, cmd->data, cmd->lc);
    parseContext.length += cmd->lc;

    if (hasMore(cmd->p1)) {
        // Reply to sender with status OK
        signState = WAITING_FOR_MORE;
        io_send_sw(SWO_SUCCESS);
    } else {
        // No more data to receive, finish up and present transaction to user
        signState = PENDING_REVIEW;

        transactionContext.rawTxLength = parseContext.length;

        // Try to parse the transaction. If the parsing fails, throw an exception
        // to cause the processing to abort and the transaction context to be reset.
        if (parse_txn_context(&parseContext)) {
            // Mask real cause behind generic error (INCORRECT_DATA)
            return SWO_INCORRECT_DATA;
        }

        review_transaction(&parseContext.result, sign_transaction, reject_transaction);
    }
    return 0;
}

int handle_first_packet(command_t *cmd) {
    int error = SWO_PARAMETER_ERROR_NO_INFO;
    uint32_t i;
    if (!isFirst(cmd->p1)) {
        return SWO_INCORRECT_DATA;
    }

    // Reset old transaction data that might still remain
    reset_transaction_context();
    parseContext.data = transactionContext.rawTx;

    if (cmd->lc < 1) {
        return SWO_WRONG_DATA_LENGTH;
    }

    transactionContext.pathLength = cmd->data[0];
    cmd->data++;
    cmd->lc--;
    if (cmd->lc < 4 * transactionContext.pathLength) {
        return SWO_WRONG_DATA_LENGTH;
    }
    if ((transactionContext.pathLength < 0x01) ||
        (transactionContext.pathLength > MAX_BIP32_PATH)) {
        return SWO_INCORRECT_DATA;
    }

    for (i = 0; i < transactionContext.pathLength; i++) {
        transactionContext.bip32Path[i] = U4BE(cmd->data, 0);
        cmd->data += 4;
        cmd->lc -= 4;
    }
    error = get_network_type(transactionContext.bip32Path, &transactionContext.network_type);
    if (error != SWO_SUCCESS) {
        return error;
    }
    if (transactionContext.network_type == MAINNET || transactionContext.network_type == TESTNET) {
        transactionContext.algo = CX_KECCAK;
    } else {
        transactionContext.algo = CX_SHA3;
    }
    return handle_packet_content(cmd);
}

int handle_subsequent_packet(const command_t *cmd) {
    if (isFirst(cmd->p1)) {
        return SWO_INCORRECT_DATA;
    }

    return handle_packet_content(cmd);
}

int handle_sign(const command_t *cmd) {
    switch (signState) {
        case IDLE:
            return handle_first_packet((command_t *) cmd);
        case WAITING_FOR_MORE:
            return handle_subsequent_packet(cmd);
        default:
            return SWO_INCORRECT_DATA;
    }
    return 0;
}
