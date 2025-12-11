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

#ifndef FUZZ
#if defined(IOCUSTOMCRYPT)
#include "aes.h"
typedef struct AES_ctx AES_CTX;
#else
typedef cx_aes_key_t AES_CTX;
#endif

int get_network_type(const uint32_t bip32Path[], uint8_t *network_type) {
    int error = SWO_INCORRECT_DATA;
    switch (bip32Path[1]) {
        case 0x8000002B:
            *network_type = MAINNET;  // N
            error = SWO_SUCCESS;
            break;
        case 0x80000001:
            *network_type = TESTNET;  // T
            error = SWO_SUCCESS;
            break;
        default:
            break;
    }
    return error;
}

uint8_t get_algo(uint8_t network_type) {
    if (network_type == MAINNET || network_type == TESTNET) {
        return CX_KECCAK;
    } else {
        return CX_SHA3;
    }
}

int sha_calculation(uint8_t algorithm,
                    const uint8_t *in,
                    uint8_t inlen,
                    uint8_t *out,
                    uint8_t outlen) {
    int error = SWO_PARAMETER_ERROR_NO_INFO;
    cx_sha3_t hash;
    if (algorithm == CX_KECCAK) {
        CX_CHECK(cx_keccak_init_no_throw(&hash, 256));
    } else {  // CX_SHA3
        CX_CHECK(cx_sha3_init_no_throw(&hash, 256));
    }
    CX_CHECK(cx_hash_no_throw(&hash.header, CX_LAST, in, inlen, out, outlen));
end:
    return error;
}

int ripemd(const uint8_t *in, uint8_t inlen, uint8_t *out, uint8_t outlen) {
    cx_ripemd160_t hash;
    cx_ripemd160_init(&hash);
    return cx_hash_no_throw(&hash.header, CX_LAST, in, inlen, out, outlen);
}

int nem_public_key_and_address(cx_ecfp_public_key_t *inPublicKey,
                               uint8_t inNetworkId,
                               unsigned int inAlgo,
                               uint8_t *outPublicKey,
                               char *outAddress,
                               uint32_t outLen) {
    uint8_t buffer1[32];
    uint8_t buffer2[20];
    uint8_t rawAddress[32];
    int error = SWO_PARAMETER_ERROR_NO_INFO;

    for (uint8_t i = 0; i < 32; i++) {
        outPublicKey[i] = inPublicKey->W[64 - i];
    }
    if ((inPublicKey->W[32] & 1) != 0) {
        outPublicKey[31] |= 0x80;
    }
    CX_CHECK(sha_calculation(inAlgo, outPublicKey, 32, buffer1, sizeof(buffer1)));
    CX_CHECK(ripemd(buffer1, 32, buffer2, sizeof(buffer2)));
    // step1: add network prefix char
    rawAddress[0] = inNetworkId;  // 152:,,,,,
    // step2: add ripemd160 hash
    memcpy(rawAddress + 1, buffer2, sizeof(buffer2));
    CX_CHECK(sha_calculation(inAlgo, rawAddress, 21, buffer1, sizeof(buffer1)));
    // step3: add checksum
    memcpy(rawAddress + 21, buffer1, 4);
    if (base32_encode((const uint8_t *) rawAddress, 25, (char *) outAddress, outLen) < 0) {
        error = SWO_DATA_MAY_BE_CORRUPTED;
        goto end;
    }
    error = SWO_SUCCESS;
end:
    return error;
}

int nem_get_remote_private_key(const uint8_t *privateKey,
                               unsigned int priKeyLen,
                               const uint8_t *key,
                               unsigned int keyLen,
                               const uint8_t *value,
                               unsigned int valueLen,
                               uint8_t encrypt,
                               uint8_t askOnEncrypt,
                               uint8_t askOnDecrypt,
                               uint8_t *out,
                               unsigned int outLen) {
    uint8_t data[260];
    memset(data, 0, sizeof(data));
    strncpy((char *) data, (const char *) key, keyLen);
    strncat((char *) data, askOnEncrypt ? "E1" : "E0", 2);
    strncat((char *) data, askOnDecrypt ? "D1" : "D0", 2);
    cx_hmac_sha512(privateKey, priKeyLen, data, strlen((char *) data), data, sizeof(data));
    AES_CTX ctx;
    const uint8_t *aes = data;
    uint8_t *iv = data + 32;
    int error = SWO_PARAMETER_ERROR_NO_INFO;

#if defined(IOCUSTOMCRYPT)
    UNUSED(valueLen);
    strncpy((char *) out, (const char *) value, outLen);
    AES_init_ctx_iv(&ctx, aes, iv);
#else
    explicit_bzero(out, outLen);
    CX_CHECK(cx_aes_init_key_no_throw(aes, 32, &ctx));
#endif

    if (encrypt) {
#if defined(IOCUSTOMCRYPT)
        AES_CBC_encrypt_buffer(&ctx, out, outLen);
#else
        CX_CHECK(cx_aes_iv_no_throw(&ctx,
                                    CX_LAST | CX_ENCRYPT | CX_CHAIN_CBC | CX_PAD_NONE,
                                    iv,
                                    16,
                                    value,
                                    valueLen,
                                    out,
                                    &outLen));
#endif
    } else {
#if defined(IOCUSTOMCRYPT)
        AES_CBC_decrypt_buffer(&ctx, out, outLen);
#else
        CX_CHECK(cx_aes_iv_no_throw(&ctx,
                                    CX_LAST | CX_DECRYPT | CX_CHAIN_CBC | CX_PAD_NONE,
                                    iv,
                                    16,
                                    value,
                                    valueLen,
                                    out,
                                    &outLen));
#endif
    }
    error = SWO_SUCCESS;
#ifndef IOCUSTOMCRYPT
end:
#endif
    return error;
}

int nem_public_key_to_address(const uint8_t *inPublicKey,
                              uint8_t inNetworkId,
                              unsigned int inAlgo,
                              char *outAddress,
                              uint32_t outLen) {
    uint8_t buffer1[32];
    uint8_t buffer2[20];
    uint8_t rawAddress[32];
    int error = SWO_PARAMETER_ERROR_NO_INFO;

    CX_CHECK(sha_calculation(inAlgo, inPublicKey, 32, buffer1, sizeof(buffer1)));
    CX_CHECK(ripemd(buffer1, 32, buffer2, sizeof(buffer2)));
    // step1: add network prefix char
    rawAddress[0] = inNetworkId;  // 152:,,,,,
    // step2: add ripemd160 hash
    memcpy(rawAddress + 1, buffer2, sizeof(buffer2));
    CX_CHECK(sha_calculation(inAlgo, rawAddress, 21, buffer1, sizeof(buffer1)));
    // step3: add checksum
    memcpy(rawAddress + 21, buffer1, 4);
    if (base32_encode((const uint8_t *) rawAddress, 25, (char *) outAddress, outLen) < 0) {
        error = SWO_DATA_MAY_BE_CORRUPTED;
        goto end;
    }
    error = SWO_SUCCESS;
end:
    return error;
}
#endif
