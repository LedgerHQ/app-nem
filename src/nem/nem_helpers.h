/*******************************************************************************
*   NEM Wallet
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
#ifndef LEDGER_APP_NEM_NEMHELPERS_H
#define LEDGER_APP_NEM_NEMHELPERS_H

#ifndef FUZZ
#include <os.h>
#include <cx.h>
#include <os_io_seproxyhal.h>
#endif
#include <stdbool.h>

#define NEM_TXN_TRANSFER 0x0101
#define NEM_TXN_IMPORTANCE_TRANSFER 0x0801
#define NEM_TXN_MULTISIG_AGGREGATE_MODIFICATION 0x1001
#define NEM_TXN_MULTISIG_SIGNATURE 0x1002
#define NEM_TXN_MULTISIG 0x1004
#define NEM_TXN_PROVISION_NAMESPACE 0x2001
#define NEM_TXN_MOSAIC_DEFINITION 0x4001
#define NEM_TXN_MOSAIC_SUPPLY_CHANGE 0x4002

/* max amount is max int64 scaled down: "922337203685.4775807" */
#define AMOUNT_MAX_SIZE 21
#define NEM_ADDRESS_LENGTH 40
#define NEM_PRETTY_ADDRESS_LENGTH 40
#define NEM_PUBLIC_KEY_LENGTH 32
#define NEM_PRIVATE_KEY_LENGTH 32
#define NEM_TRANSACTION_HASH_LENGTH 32

#define TESTNET 152 //0x98
#define MAINNET 104 //0x68
#define MIJIN_MAINNET 96 //0x60
#define MIJIN_TESTNET 144 //0x90

#define STR_NEM "nem"
#define STR_XEM "xem"

uint8_t get_network_type(const uint32_t bip32Path[]);
uint8_t get_algo(uint8_t network_type);
#ifndef FUZZ
void nem_public_key_and_address(cx_ecfp_public_key_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, uint8_t *outPublicKey, char *outAddress, uint8_t outLen);
void nem_get_remote_private_key(const uint8_t *privateKey, unsigned int priKeyLen, const char* key, unsigned int keyLen, uint8_t askOnEncrypt, uint8_t askOnDecrypt, uint8_t *out, unsigned int outLen);
void nem_public_key_to_address(const uint8_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, char *outAddress, uint8_t outLen);
#endif

#endif //LEDGER_APP_NEM_NEMHELPERS_H
