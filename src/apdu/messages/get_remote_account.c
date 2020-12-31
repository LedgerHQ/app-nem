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
#include "get_remote_account.h"
#include "global.h"
#include "nem_helpers.h"
#include "idle_menu.h"
#include "remote_ui.h"

uint8_t nem_remote_private_key[NEM_PRIVATE_KEY_LENGTH];

uint32_t set_result_get_delegated_harvesting_key() {
    uint32_t tx = 0;

    // privatekey
    G_io_apdu_buffer[tx++] = NEM_PRIVATE_KEY_LENGTH;
    memcpy(G_io_apdu_buffer + tx, nem_remote_private_key, NEM_PRIVATE_KEY_LENGTH);
    tx += NEM_PRIVATE_KEY_LENGTH;
    return tx;
}

void on_privatekey_confirmed() {
    uint32_t tx = set_result_get_delegated_harvesting_key();
    G_io_apdu_buffer[tx++] = 0x90;
    G_io_apdu_buffer[tx++] = 0x00;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
    // Display back the original UX
    display_idle_menu();
}

void on_privatekey_rejected() {
    G_io_apdu_buffer[0] = 0x69;
    G_io_apdu_buffer[1] = 0x85;
    // Send back the response, do not restart the event loop
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
    // Display back the original UX
    display_idle_menu();
}

void handle_remote_private_key(uint8_t p1, uint8_t p2, uint8_t *dataBuffer,
                            uint16_t dataLength, volatile unsigned int *flags,
                            volatile unsigned int *tx) {
    UNUSED(dataLength);
    uint8_t privateKeyData[NEM_PRIVATE_KEY_LENGTH];
    uint32_t bip32Path[MAX_BIP32_PATH];
    uint32_t i;
    uint8_t bip32PathLength = *(dataBuffer++);
    cx_ecfp_private_key_t privateKey;
    uint8_t algo;
    uint8_t encrypt = 0;
    uint8_t askOnEncrypt = 0;
    uint8_t askOnDecrypt = 0;
    uint8_t p2Chain = p2 & 0x3F;
    char key[32] = "Export delegated harvesting key?";
    UNUSED(p2Chain);
    if ((bip32PathLength < 1) || (bip32PathLength > MAX_BIP32_PATH)) {
        THROW(0x6a80);
    }
    if ((p1 != P1_CONFIRM) && (p1 != P1_NON_CONFIRM)) {
        THROW(0x6B00);
    }
    //Read and convert path's data
    for (i = 0; i < bip32PathLength; i++) {
        bip32Path[i] = (dataBuffer[0] << 24) | (dataBuffer[1] << 16) |
                       (dataBuffer[2] << 8) | (dataBuffer[3]);
        dataBuffer += 4;
    }
    uint8_t network_type = get_network_type(bip32Path);
    algo = get_algo(network_type);
    io_seproxyhal_io_heartbeat();
    BEGIN_TRY {
        TRY {
            os_perso_derive_node_bip32(CX_CURVE_256K1, bip32Path, bip32PathLength, privateKeyData, NULL);
            if (algo == CX_SHA3) {
                cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyData, NEM_PRIVATE_KEY_LENGTH, &privateKey);
            } else if (algo == CX_KECCAK) {
                //reverse privateKey
                uint8_t privateKeyDataR[NEM_PRIVATE_KEY_LENGTH];
                for (uint8_t j = 0; j < NEM_PRIVATE_KEY_LENGTH; j++) {
                    privateKeyDataR[j] = privateKeyData[NEM_PRIVATE_KEY_LENGTH - 1 - j];
                }
                cx_ecfp_init_private_key(CX_CURVE_Ed25519, privateKeyDataR, NEM_PRIVATE_KEY_LENGTH, &privateKey);
                explicit_bzero(privateKeyDataR, sizeof(privateKeyDataR));
            } else {
                THROW(0x6a80);
            }
            io_seproxyhal_io_heartbeat();
            nem_get_remote_private_key(privateKey.d, 32, key, strlen(key), askOnEncrypt, askOnDecrypt, nem_remote_private_key, 32);
            explicit_bzero(privateKeyData, sizeof(privateKeyData));
            explicit_bzero(&privateKey, sizeof(privateKey));
            io_seproxyhal_io_heartbeat();
        }
        CATCH_OTHER(e) {
            THROW(e);
        }
        FINALLY {
            explicit_bzero(privateKeyData, sizeof(privateKeyData));
            explicit_bzero(&privateKey, sizeof(privateKey));
        }
    }
    END_TRY;
    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_delegated_harvesting_key();
        THROW(0x9000);
    } else {
        display_remote_account_confirmation_ui(
                on_privatekey_confirmed,
                on_privatekey_rejected
        );
        *flags |= IO_ASYNCH_REPLY;
    }
}
