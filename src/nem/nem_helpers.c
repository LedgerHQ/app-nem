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
#include <string.h>
#include "base32.h"
#include "nem_helpers.h"

uint8_t get_network_type(const uint32_t bip32Path[]) {
    switch(bip32Path[2]) {
        case 0x80000068:
            return MAINNET; //N
        case 0x80000098:
            return TESTNET; //T
        case 0x80000060:
            return MIJIN_MAINNET; //M
        case 0x80000090:
            return MIJIN_TESTNET; //S
        default:
            THROW(0x6a80);
    }
}

uint8_t get_algo(uint8_t network_type) {
    if (network_type == MAINNET || network_type == TESTNET) {
        return CX_KECCAK;
    } else {
        return CX_SHA3;
    }
}

void sha_calculation(uint8_t algorithm, const uint8_t *in, uint8_t inlen, uint8_t *out, uint8_t outlen) {
    cx_sha3_t hash;
    if (algorithm == CX_KECCAK) {
        cx_keccak_init(&hash, 256);
    } else { //CX_SHA3
        cx_sha3_init(&hash, 256);
    }
    cx_hash(&hash.header, CX_LAST, in, inlen, out, outlen);
}

void ripemd(const uint8_t *in, uint8_t inlen, uint8_t *out, uint8_t outlen) {
    cx_ripemd160_t hash;
    cx_ripemd160_init(&hash);
    cx_hash(&hash.header, CX_LAST, in, inlen, out, outlen);
}

void nem_public_key_and_address(cx_ecfp_public_key_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, uint8_t *outPublicKey, char *outAddress, uint8_t outLen) {
    uint8_t buffer1[32];
    uint8_t buffer2[20];
    uint8_t rawAddress[32];

    for (uint8_t i=0; i<32; i++) {
        outPublicKey[i] = inPublicKey->W[64 - i];
    }
    if ((inPublicKey->W[32] & 1) != 0) {
        outPublicKey[31] |= 0x80;
    }
    sha_calculation(inAlgo, outPublicKey, 32, buffer1, sizeof(buffer1));
    ripemd(buffer1, 32, buffer2, sizeof(buffer2));
    //step1: add network prefix char
    rawAddress[0] = inNetworkId;   //152:,,,,,
    //step2: add ripemd160 hash
    memcpy(rawAddress + 1, buffer2, sizeof(buffer2));
    sha_calculation(inAlgo, rawAddress, 21, buffer1, sizeof(buffer1));
    //step3: add checksum
    memcpy(rawAddress + 21, buffer1, 4);
    base32_encode((const uint8_t *) rawAddress, 25, (char *) outAddress, outLen);
}

void nem_get_remote_private_key(const uint8_t *privateKey, unsigned int priKeyLen, const char* key, unsigned int keyLen, uint8_t askOnEncrypt, uint8_t askOnDecrypt, uint8_t *out, unsigned int outLen)
{
    int result;
    uint8_t data[260];
    memset(data, 0, sizeof(data));
    strncpy((char *)data, key, keyLen);
    strncat((char *)data, askOnEncrypt ? "E1" : "E0", 2);
    strncat((char *)data, askOnDecrypt ? "D1" : "D0", 2);
    result = cx_hmac_sha512(privateKey, priKeyLen, data, strlen((char *)data), data, sizeof(data));
    strncpy((char *) out, (const char *) data, outLen);
}

void nem_public_key_to_address(const uint8_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, char *outAddress, uint8_t outLen) {
    uint8_t buffer1[32];
    uint8_t buffer2[20];
    uint8_t rawAddress[32];
    sha_calculation(inAlgo, inPublicKey, 32, buffer1, sizeof(buffer1));
    ripemd(buffer1, 32, buffer2, sizeof(buffer2));
    //step1: add network prefix char
    rawAddress[0] = inNetworkId;   //152:,,,,,
    //step2: add ripemd160 hash
    memcpy(rawAddress + 1, buffer2, sizeof(buffer2));
    sha_calculation(inAlgo, rawAddress, 21, buffer1, sizeof(buffer1));
    //step3: add checksum
    memcpy(rawAddress + 21, buffer1, 4);
    base32_encode((const uint8_t *) rawAddress, 25, (char *) outAddress, outLen);
}
