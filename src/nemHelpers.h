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

#include "os.h"
#include "cx.h"
#include "os_io_seproxyhal.h"
#include <stdbool.h>

#define MAX_BIP32_PATH 5
#define MAX_PRINT_MESSAGE_LENGTH 11
#define MAX_PRINT_DETAIL_NAME_LENGTH 15
#define MAX_PRINT_EXTRA_INFOR_LENGTH 17
#define MAX_PRINT_DETAIL_NAME_SCREEN 11
#define MAX_PRINT_EXTRA_INFO_SCREEN 10
#define NEM_ADDRESS_LENGTH 41
#define MAX_UX_CALLBACK_INTERVAL 2

#define NEM_TESTNET 152
#define NEM_MAINNET 104
#define MIJIN_MAINNET 96
#define MIJIN_TESTNET 144

// static const int32_t MAIN_NETWORK_VERSION = 0x68000001;
// static const int32_t TEST_NETWORK_VERSION = 0x98000001;
// static const int32_t MINJIN_NETWORK_VERSION = 0x60000001;

//rootNamespaceRentalFeePerBlock = 1'000'000
//childNamespaceRentalFee = 1'000'000

#define NEMV1_TRANSFER 0x101
#define NEMV1_IMPORTANCE_TRANSFER 0x801
#define NEMV1_MULTISIG_MODIFICATION 0x1001
#define NEMV1_MULTISIG_SIGNATURE 0x1002
#define NEMV1_MULTISIG_TRANSACTION 0x1004
#define NEMV1_PROVISION_NAMESPACE 0x2001
#define NEMV1_MOSAIC_DEFINITION 0x4001
#define NEMV1_MOSAIC_SUPPLY_CHANGE 0x4002
#define NEMV1_MOSAIC_SUPPLY 0x4002

#define TRANSFER 0x4154
#define REGISTER_NAMESPACE 0x414E
#define ADDRESS_ALIAS 0x424E
#define MOSAIC_ALIAS 0x434E
#define MOSAIC_DEFINITION 0x414D
#define MOSAIC_SUPPLY_CHANGE 0x424D
#define MODIFY_MULTISIG_ACCOUNT 0x4155
#define AGGREGATE_COMPLETE 0x4141
#define AGGREGATE_BONDED 0x4241
#define LOCK 0x4148
#define SECRET_LOCK 0x4152
#define SECRET_PROOF 0x4252
#define MODIFY_ACCOUNT_PROPERTY_ADDRESS 0x4150
#define MODIFY_ACCOUNT_PROPERTY_MOSAIC 0x4250
#define MODIFY_ACCOUNT_PROPERTY_ENTITY_TYPE 0x4350

/**
 * Nano S has 320 KB flash, 10 KB RAM, uses a ST31H320 chip.
 * This effectively limits the max size
 * So we can only sign transactions up to 490Bytes in size.
 * max size of a transaction, binary will not compile if we try to allow transactions over 490Bytes.
 */
// static const uint16_t MAX_TX_RAW_LENGTH = 512;
#define MAX_TX_RAW_LENGTH 490

/** length of the APDU (application protocol data unit) header. */
#define APDU_HEADER_LENGTH 5

/** offset in the APDU header which says the length of the body. */
#define APDU_BODY_LENGTH_OFFSET 4

uint8_t readNetworkIdFromBip32path(uint32_t bip32Path[]);
uint8_t *reverseBytes(uint8_t *sourceArray, uint16_t len);
void uint2Ascii(uint8_t *inBytes, uint8_t len, char *out);
void print_amount(uint64_t amount, uint8_t divisibility, char *asset, char *out);

uint16_t getUint16(uint8_t *buffer);
uint32_t getUint32(uint8_t *data);
uint64_t getUint64(uint8_t *data);
void to_nem_public_key_and_address(cx_ecfp_public_key_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, uint8_t *outNemPublicKey, unsigned char *outNemAddress);
void public_key_to_address(uint8_t inNetworkId, uint8_t *outNemPublicKey, unsigned char *outNemAddress);

/** returns the length of the transaction in the buffer. */
unsigned int get_apdu_buffer_length();

/** Clean the buffer of tx. */
void clean_raw_tx(unsigned char *raw_tx);

int compare_strings(char str1[], char str2[]);

int string_length(char str[]);

/** Convert 1 hex number to 2 characters */
char hex2Ascii(uint8_t input);

void parse_transfer_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

void parse_mosaic_definition_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

void parse_mosaic_supply_change_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

void parse_provision_namespace_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

void parse_aggregate_modification_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count,
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    bool isMultisig,
    uint8_t networkId
);

void parse_multisig_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH],
    uint8_t networkId
);

void parse_multisig_signature_tx (unsigned char raw_tx[],
    unsigned int* ux_step_count,
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char fullAddress[NEM_ADDRESS_LENGTH]
);

//Catapult
void parse_catapult_transfer_tx (
    unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

void parse_catapult_provision_namespace_tx (
    unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

void parse_catapult_mosaic_definition_tx (
    unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig
);

void parse_catapult_aggregate_complete_tx (
    unsigned char raw_tx[],
    unsigned int* ux_step_count, 
    char txTypeName[30],
    char detailName[MAX_PRINT_DETAIL_NAME_SCREEN][MAX_PRINT_DETAIL_NAME_LENGTH],
    char extraInfo[MAX_PRINT_EXTRA_INFO_SCREEN][MAX_PRINT_EXTRA_INFOR_LENGTH],
    char extraInfo_0[NEM_ADDRESS_LENGTH],
    bool isMultisig
);