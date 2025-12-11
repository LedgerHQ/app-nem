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
#include "get_remote_account.h"
#include "global.h"
#include "nem_helpers.h"
#include "idle_menu.h"
#include "remote_ui.h"

typedef struct {
    uint8_t bip32PathLength;
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint8_t askOnEncrypt;
    uint8_t askOnDecrypt;
    uint8_t encrypt;
} KeyData_t;

#if defined(IOCUSTOMCRYPT)
uint8_t nem_remote_private_key[NEM_PRIVATE_KEY_LENGTH];
#else
uint8_t nem_remote_private_key[NEM_PRIVATE_KEY_LENGTH * 2];
#endif

int set_result_get_delegated_harvesting_key() {
    uint32_t tx = 0;

    // privatekey
    G_io_apdu_buffer[tx++] = NEM_PRIVATE_KEY_LENGTH;
    memcpy(G_io_apdu_buffer + tx, nem_remote_private_key, NEM_PRIVATE_KEY_LENGTH);
    tx += NEM_PRIVATE_KEY_LENGTH;
    return io_send_response_pointer(G_io_apdu_buffer, tx, SWO_SUCCESS);
}

void on_privatekey_confirmed() {
    if (set_result_get_delegated_harvesting_key() < 0) {
        return;
    }
    display_remote_account_done(true);
}

void on_privatekey_rejected() {
    io_send_sw(SWO_CONDITIONS_NOT_SATISFIED);
    display_remote_account_done(false);
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
    if (cmd->lc < 1 + 4 * keyData->bip32PathLength) {
        return SWO_WRONG_DATA_LENGTH;
    }
    if ((keyData->bip32PathLength < 1) || (keyData->bip32PathLength > MAX_BIP32_PATH)) {
        return SWO_INCORRECT_DATA;
    }

    for (int i = 0; i < keyData->bip32PathLength; i++) {
        keyData->bip32Path[i] = U4BE(dataBuffer, 0);
        dataBuffer += 4;
    }
    keyData->encrypt = 1;
    return SWO_SUCCESS;
}

/**
 * Calculates and returns a public key which corresponds to bip32 path in 'keyData'
 *
 */
static int get_harvesting_key(KeyData_t *keyData) {
    uint8_t privateKeyData[NEM_RAW_PRIVATE_KEY_LENGTH];
    cx_ecfp_private_key_t privateKey;
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
    error = nem_get_remote_private_key(privateKey.d,
                                       sizeof(privateKey.d),
                                       (const uint8_t *) ACC_KEY,
                                       32,
                                       (const uint8_t *) ACC_VALUE,
                                       64,
                                       keyData->encrypt,
                                       keyData->askOnEncrypt,
                                       keyData->askOnDecrypt,
                                       nem_remote_private_key,
                                       sizeof(nem_remote_private_key));
    io_seproxyhal_io_heartbeat();
end:
    explicit_bzero(privateKeyData, sizeof(privateKeyData));
    explicit_bzero(&privateKey, sizeof(privateKey));
    return error;
}

int handle_remote_private_key(const command_t *cmd) {
    // extract key data used for calculating public key, from APDU parameters
    KeyData_t keyData = {0};
    int error = SWO_PARAMETER_ERROR_NO_INFO;

    error = extract_parameters(cmd, &keyData);
    if (SWO_SUCCESS != error) {
        return error;
    }

    error = get_harvesting_key(&keyData);
    if (SWO_SUCCESS != error) {
        return error;
    }

    if (cmd->p1 == P1_NON_CONFIRM) {
        if (set_result_get_delegated_harvesting_key() < 0) {
            error = SWO_COMMAND_ERROR_NO_INFO;
        } else {
            error = SWO_SUCCESS;
        }
    } else {
        display_remote_account_confirmation_ui(on_privatekey_confirmed, on_privatekey_rejected);
        error = 0;
    }
    return error;
}
