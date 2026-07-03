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
#include "io.h"
#include "get_public_key.h"
#include "global.h"
#include "nem_helpers.h"
#include "idle_menu.h"
#include "address_ui.h"
#include "buffer.h"

typedef struct {
    uint8_t bip32PathLength;
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint8_t networkType;
    uint8_t algo;
} KeyData_t;

uint8_t nem_publickey[NEM_PUBLIC_KEY_LENGTH];
char nem_address[NEM_PRETTY_ADDRESS_LENGTH + 1];

int set_result_get_publickey(void) {
    uint32_t tx = 0;

    // address
    G_io_apdu_buffer[tx++] = NEM_PRETTY_ADDRESS_LENGTH;
    memmove(G_io_apdu_buffer + tx, nem_address, NEM_PRETTY_ADDRESS_LENGTH);
    tx += NEM_PRETTY_ADDRESS_LENGTH;

    // publicKey
    G_io_apdu_buffer[tx++] = NEM_PUBLIC_KEY_LENGTH;
    memcpy(G_io_apdu_buffer + tx, nem_publickey, NEM_PUBLIC_KEY_LENGTH);
    tx += NEM_PUBLIC_KEY_LENGTH;
    return io_send_response_pointer(G_io_apdu_buffer, tx, SWO_SUCCESS);
}

void on_address_confirmed(void) {
    if (set_result_get_publickey() < 0) {
        return;
    }
    display_address_confirmation_done(true);
}

void on_address_rejected(void) {
    io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
    display_address_confirmation_done(false);
}

/**
 * Extracts key data used for calculating public key, from APDU parameters, and returns it in
 * 'keyData'.
 *
 */
static int extract_parameters(const command_t *cmd, KeyData_t *keyData) {
    uint8_t *dataBuffer = (uint8_t *) cmd->data;
    // check length of data is correct
    if (cmd->lc < 1) {
        return SWO_WRONG_DATA_LENGTH;
    }

    // check that p1 is set to either to confirm or not to confirm transaction by user
    if ((cmd->p1 != P1_CONFIRM) && (cmd->p1 != P1_NON_CONFIRM)) {
        return SWO_WRONG_P1_P2;
    }

    // Read and convert path's data
    keyData->bip32PathLength = *(dataBuffer++);
    if (cmd->lc < 1 + 4 * keyData->bip32PathLength + 1) {
        return SWO_WRONG_DATA_LENGTH;
    }
    if ((keyData->bip32PathLength < 1) || (keyData->bip32PathLength > MAX_BIP32_PATH)) {
        return SWO_INCORRECT_DATA;
    }

    for (int i = 0; i < keyData->bip32PathLength; i++) {
        keyData->bip32Path[i] = U4BE(dataBuffer, 0);
        dataBuffer += 4;
    }

    // prepare output
    keyData->networkType = *dataBuffer;
    keyData->algo = get_algo(keyData->networkType);
    return SWO_SUCCESS;
}

/**
 * Calculates and returns a public key which corresponds to bip32 path in 'keyData'
 *
 */
static int get_public_key(KeyData_t *keyData) {
    uint8_t privateKeyData[NEM_RAW_PRIVATE_KEY_LENGTH];
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_public_key_t publicKey;
    int error = SWO_PARAMETER_ERROR_NO_INFO;

    // ensure a I/O channel is not timing out
    io_seproxyhal_io_heartbeat();

    CX_CHECK(os_derive_bip32_with_seed_no_throw(HDW_ED25519_SLIP10,
                                                CX_CURVE_Ed25519,
                                                keyData->bip32Path,
                                                keyData->bip32PathLength,
                                                privateKeyData,
                                                NULL,
                                                (unsigned char *) "ed25519-keccak seed",
                                                19));
    CX_CHECK(cx_ecfp_init_private_key_no_throw(CX_CURVE_Ed25519,
                                               privateKeyData,
                                               NEM_PRIVATE_KEY_LENGTH,
                                               &privateKey));
    io_seproxyhal_io_heartbeat();
    CX_CHECK(cx_ecfp_generate_pair2_no_throw(CX_CURVE_Ed25519,
                                             &publicKey,
                                             &privateKey,
                                             1,
                                             keyData->algo));
    io_seproxyhal_io_heartbeat();
    error = nem_public_key_and_address(&publicKey,
                                       keyData->networkType,
                                       keyData->algo,
                                       (uint8_t *) &nem_publickey,
                                       (char *) &nem_address,
                                       sizeof(nem_address));
    io_seproxyhal_io_heartbeat();
end:
    explicit_bzero(privateKeyData, sizeof(privateKeyData));
    explicit_bzero(&privateKey, sizeof(privateKey));
    return error;
}

int handle_public_key(const command_t *cmd) {
    // extract key data used for calculating public key, from APDU parameters
    KeyData_t keyData = {0};
    int error = SWO_PARAMETER_ERROR_NO_INFO;

    error = extract_parameters(cmd, &keyData);
    if (SWO_SUCCESS != error) {
        return error;
    }

    error = get_public_key(&keyData);
    if (SWO_SUCCESS != error) {
        return error;
    }

    if (cmd->p1 == P1_NON_CONFIRM) {
        if (set_result_get_publickey() < 0) {
            error = SWO_COMMAND_ERROR_NO_INFO;
        } else {
            error = SWO_SUCCESS;
        }
    } else {
        display_address_confirmation_ui(nem_address, on_address_confirmed, on_address_rejected);
        error = 0;
    }

    return error;
}
