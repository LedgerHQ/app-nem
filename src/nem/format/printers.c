/*******************************************************************************
*   NEM Wallet
*   (c) 2017 Ledger
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
#include <stdio.h>
#include <stdint.h>
#include "printers.h"
#include "parse/nem_parse.h"

int snprintf_number(char *dst, uint32_t len, uint64_t value) {
    char *p = dst;
    // First, compute the address of the last digit to be written.
    uint64_t shifter = value;
    do {
        p++;
        shifter /= 10;
    } while (shifter);

    if (p > dst + len - 1) {
        return E_NOT_ENOUGH_DATA;
    }
    int n = p - dst;

    // Now write string representation, right to left.
    *p-- = 0;
    do {
        *p-- = '0' + (value % 10);
        value /= 10;
    } while (value);
    return n;
}

int snprintf_token(char* dst, uint32_t len, uint64_t amount, uint8_t divisibility, char* token) {
    char buffer[MAX_FIELD_LEN];
    uint64_t dVal = amount;
    int i, j;
    uint8_t MAX_DIVISIBILITY = (divisibility == 0) ? 0 : 6;

    memset(buffer, 0, MAX_FIELD_LEN);
    for (i = 0; dVal > 0 || i < MAX_DIVISIBILITY + 1; i++) {
        if (dVal > 0) {
            buffer[i] = (dVal % 10) + '0';
            dVal /= 10;
        } else {
            buffer[i] = '0';
        }
        if (i == divisibility - 1) { // divisibility
            i += 1;
            buffer[i] = '.';
            if (dVal == 0) {
                i += 1;
                buffer[i] = '0';
            }
        }
        if (i >= MAX_FIELD_LEN) {
            return E_NOT_ENOUGH_DATA;
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < (int)len-1; i--, j++) {
        dst[j] = buffer[i];
    }
    // strip trailing 0s
    if (MAX_DIVISIBILITY != 0) {
        for (j -= 1; j > 0; j--) {
            if (dst[j] != '0') break;
        }
        j += 1;
    }
    // strip trailing .
    if (dst[j-1] == '.') j -= 1;

    if (token) {
        // qualify amount
        if (j + strlen(token) + 1 < len) {
            dst[j++] = ' ';
            strcpy(dst + j, token);
            dst[j+strlen(token)] = '\0';
            return j+strlen(token);
        } else {
            dst[j] = '\0';
            return j;
        }
    } else {
        dst[j] = '\0';
        return j;
    }
}

int snprintf_hex(char *dst, uint32_t maxLen, const uint8_t *src, uint32_t dataLength, uint8_t reverse) {
    if (2 * dataLength > maxLen - 1 || maxLen < 1 || dataLength < 1) {
        return E_NOT_ENOUGH_DATA;
    }
    for (uint32_t i = 0; i < dataLength; i++) {
        snprintf(dst + 2 * i, maxLen - 2 * i, "%02X", reverse==1?src[dataLength-1-i]:src[i]);
    }
    dst[2*dataLength] = '\0';
    return 2*dataLength;
}

int snprintf_ascii(char *dst, uint32_t pos, uint32_t maxLen, const uint8_t *src, uint32_t dataLength) {
    if (dataLength + pos > maxLen - 1 || maxLen < 1 || dataLength < 1) {
        return E_NOT_ENOUGH_DATA;
    }
    char *tmpCh = (char *) src;
    uint32_t k = 0, l = 0;
    for (uint32_t j=0; j < dataLength; j++){
        if (tmpCh[j] < 32 || tmpCh[j] > 126) {
            k++;
            if (k==1) {
                dst[pos + l] = '?';
                l++;
            } else if (k==2) {
                k = 0;
            }
        } else {
            k = 0;
            dst[pos + l] = tmpCh[j];
            l++;
        }
    }
    dst[pos + l] = '\0';
    return l;
}

/** Convert 1 hex byte to 2 characters */
char hex2ascii(uint8_t input){
    return input > 9 ? (char)(input + 87) : (char)(input + 48);
}

int snprintf_hex2ascii(char *dst, uint32_t maxLen, const uint8_t *src, uint32_t dataLength) {
    if (2 * dataLength > maxLen - 1 || maxLen < 1 || dataLength < 1) {
        return E_NOT_ENOUGH_DATA;
    }
    for (uint32_t j=0; j < dataLength; j++) {
        dst[2*j] = hex2ascii((src[j] & 0xf0) >> 4);
        dst[2*j+1] = hex2ascii(src[j] & 0x0f);
    }
    dst[2*dataLength] = '\0';
    return 2*dataLength;
}
