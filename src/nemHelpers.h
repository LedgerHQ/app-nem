/*******************************************************************************
*   NEM Wallet
*   (c) 2017 Ledger
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

#ifndef TESTING
#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"
#else
#include "lcx_ecfp.h"
#endif

#include <stdbool.h>
#include <stdint.h>

#define MAX_BIP32_PATH 5

static const int MAX_UX_CALLBACK_INTERVAL = 2;

//static const uint8_t MAX_PRINT_MESSAGE_LENGTH = 16; //16

static const uint8_t NEM_TESTNET = 152;
static const uint8_t NEM_MAINNET = 104;
static const uint8_t MIJIN_MAINNET = 96;
static const uint8_t MIJIN_TESTNET = 144;

static const int32_t MAIN_NETWORK_VERSION = 0x68000001;
static const int32_t TEST_NETWORK_VERSION = 0x98000001;
static const int32_t MINJIN_NETWORK_VERSION = 0x60000001;

//rootNamespaceRentalFeePerBlock = 1'000'000
//childNamespaceRentalFee = 1'000'000

/**
 * Nano S has 320 KB flash, 10 KB RAM, uses a ST31H320 chip.
 * This effectively limits the max size
 * So we can only sign transactions up to 490Bytes in size.
 * max size of a transaction, binary will not compile if we try to allow transactions over 490Bytes.
 */
// static const uint16_t MAX_TX_RAW_LENGTH = 512;
static const uint16_t MAX_TX_RAW_LENGTH = 490;

/** length of the APDU (application protocol data unit) header. */
static const uint8_t APDU_HEADER_LENGTH = 5;

/** offset in the APDU header which says the length of the body. */
static const uint8_t APDU_BODY_LENGTH_OFFSET = 4;

/*
mosaicId:
mosaicFullName:
divi:
levyType
levyMosaicId:
levyMosaicFullName:

*/

static inline uint32_t get_uint32_le(const uint8_t *data) {
    return (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];
}

uint8_t readNetworkIdFromBip32path(const uint32_t bip32Path[]);

void to_nem_public_key_and_address(cx_ecfp_public_key_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, uint8_t *outNemPublicKey, char *outNemAddress);
void public_key_to_address(uint8_t inNetworkId, const uint8_t *outNemPublicKey, char *outNemAddress);

/** returns the length of the transaction in the buffer. */
unsigned int get_apdu_buffer_length();

/** Clean the buffer of tx. */
void clean_raw_tx(unsigned char *raw_tx);
