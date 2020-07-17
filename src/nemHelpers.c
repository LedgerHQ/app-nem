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
#else
#include <string.h>
#include "lcx_hash.h"
#include "lcx_ripemd160.h"
#include "lcx_sha3.h"
#endif
#include "base32.h"
#include "nemHelpers.h"

#include <ctype.h>
#include <inttypes.h>

#define MAX_SAFE_INTEGER 9007199254740991

#ifndef TESTING
uint8_t readNetworkIdFromBip32path(const uint32_t bip32Path[]) {
    uint8_t outNetworkId;
    switch(bip32Path[2]) {
        case 0x80000068: 
            outNetworkId = 104; //N
            break;
        case 0x80000098:
           outNetworkId = 152; //T
           break;
        case 0x80000060:
            outNetworkId = 96; //M
            break;
        case 0x80000090:
            outNetworkId = 144; //S
            break;
        default:
            THROW(0x6a80);
    }
    return outNetworkId;
}
#endif

uint8_t *reverseBytes(const uint8_t *sourceArray, uint16_t len){
    uint8_t outArray[len];
    for (uint8_t j=0; j<len; j++) {
        outArray[j] = sourceArray[len - j -1];
    }
    return outArray;
}

#ifndef TESTING
void to_nem_public_key_and_address(cx_ecfp_public_key_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, uint8_t *outNemPublicKey, char *outNemAddress) {
    uint8_t i;
    for (i=0; i<32; i++) {
        outNemPublicKey[i] = inPublicKey->W[64 - i];
    }

    if ((inPublicKey->W[32] & 1) != 0) {
        outNemPublicKey[31] |= 0x80;
    }    

    cx_sha3_t hash1;
    cx_sha3_t temphash;
    
    if (inAlgo == CX_SHA3) {
        cx_sha3_init(&hash1, 256);
        cx_sha3_init(&temphash, 256);
    }else{ //CX_KECCAK
        cx_keccak_init(&hash1, 256);
        cx_keccak_init(&temphash, 256);
    }
    unsigned char buffer1[32];
    cx_hash(&hash1.header, CX_LAST, outNemPublicKey, 32, buffer1, sizeof(buffer1));
    unsigned char buffer2[20];
    cx_ripemd160_t hash2;
    cx_ripemd160_init(&hash2);
    cx_hash(&hash2.header, CX_LAST, buffer1, 32, buffer2, sizeof(buffer2));
    unsigned char rawAddress[50];
    //step1: add network prefix char
    rawAddress[0] = inNetworkId;   //104,,,,,
    //step2: add ripemd160 hash
    memmove(rawAddress + 1, buffer2, sizeof(buffer2));
    
    unsigned char buffer3[32];
    cx_hash(&temphash.header, CX_LAST, rawAddress, 21, buffer3, sizeof(buffer3));
    //step3: add checksum
    memmove(rawAddress + 21, buffer3, 4);
    base32_encode(rawAddress, sizeof(rawAddress), outNemAddress, 40);
}

void public_key_to_address(uint8_t inNetworkId, const uint8_t *inNemPublicKey, char *outNemAddress) {
    cx_sha3_t hash1;
    cx_sha3_t temphash;
    cx_keccak_init(&hash1, 256);
    cx_keccak_init(&temphash, 256);

    unsigned char buffer1[32];
    cx_hash(&hash1.header, CX_LAST, inNemPublicKey, 32, buffer1, sizeof(buffer1));
    unsigned char buffer2[20];
    cx_ripemd160_t hash2;
    cx_ripemd160_init(&hash2);
    cx_hash(&hash2.header, CX_LAST, buffer1, 32, buffer2, sizeof(buffer2));
    unsigned char rawAddress[50];
    //step1: add network prefix char
    rawAddress[0] = inNetworkId;   //104,,,,,
    //step2: add ripemd160 hash
    memmove(rawAddress + 1, buffer2, sizeof(buffer2));
    
    unsigned char buffer3[32];
    cx_hash(&temphash.header, CX_LAST, rawAddress, 21, buffer3, sizeof(buffer3));
    //step3: add checksum
    memmove(rawAddress + 21, buffer3, 4);
    base32_encode(rawAddress, sizeof(rawAddress), outNemAddress, 40);
}

unsigned int get_apdu_buffer_length() {
	unsigned int len0 = G_io_apdu_buffer[APDU_BODY_LENGTH_OFFSET];
	return len0;
}
#endif  // TESTING

void clean_raw_tx(unsigned char *raw_tx) {
    uint16_t i;
    for (i = 0; i < MAX_TX_RAW_LENGTH; i++) {
        raw_tx[i] = 0;
    }
}
