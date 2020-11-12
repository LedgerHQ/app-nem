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
#include <os_io_seproxyhal.h>
#include <string.h>
#include "readers.h"

char int_to_number_char(uint64_t value) {
    if (value > 9) {
        return '?';
    }

    return (char) ('0' + value);
}

uint16_t sprintf_number(char *dst, uint16_t len, uint64_t value) {
    char *p = dst;

    // First, compute the address of the last digit to be written.
    uint64_t shifter = value;
    do {
        p++;
        shifter /= 10;
    } while (shifter);

    if (p > dst + len - 1) {
        THROW(EXCEPTION_OVERFLOW);
    }
    uint16_t n = p - dst;

    // Now write string representation, right to left.
    *p-- = 0;
    do {
        *p-- = int_to_number_char(value % 10);
        value /= 10;
    } while (value);
    return n;
}

uint16_t sprintf_token(char* dst, uint16_t len, uint64_t amount, uint8_t divisibility, char* token) {
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
            THROW(0x6700);
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < len-1; i--, j++) {
        dst[j] = buffer[i];
    }
    // strip trailing 0s
    if (MAX_DIVISIBILITY != 0)
    {
        for (j -= 1; j > 0; j--) {
            if (dst[j] != '0') break;
        }
        j += 1;
    }
    // strip trailing .
    if (dst[j-1] == '.') j -= 1;

    if (token) {
        // qualify amount
        dst[j++] = ' ';
        strcpy(dst + j, token);
        dst[j+strlen(token)] = '\0';
        return j+strlen(token);
    } else {
        dst[j] = '\0';
        return j;
    }
}

uint16_t sprintf_hex(char *dst, uint16_t maxLen, uint8_t *src, uint16_t dataLength, uint8_t reverse) {
    if (2 * dataLength > maxLen - 1) {
        THROW(EXCEPTION_OVERFLOW);
    }
    for (uint16_t i = 0; i < dataLength; i++) {
        SPRINTF(dst + 2 * i, "%02X", reverse==1?src[dataLength-1-i]:src[i]);
    }
    dst[2*dataLength] = '\0';
    return 2*dataLength;
}

uint16_t snprintf_ascii_ex(char *dst, uint16_t pos, uint16_t maxLen, uint8_t *src, uint16_t dataLength) {
    if (dataLength + pos > maxLen - 1) {
        THROW(EXCEPTION_OVERFLOW);
    }
    char *tmpCh = (char *) src;
    uint16_t k = 0, l = 0;
    for (uint8_t j=0; j < dataLength; j++){
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
    return l + pos;
}

uint16_t sprintf_ascii(char *dst, uint16_t maxLen, uint8_t *src, uint16_t dataLength) {
    return snprintf_ascii_ex(dst, 0, maxLen, src, dataLength);
}

uint16_t snprintf_ascii(char *dst, uint16_t pos, uint16_t maxLen, uint8_t *src, uint16_t dataLength) {
    if (dataLength + pos > maxLen - 1) {
        THROW(EXCEPTION_OVERFLOW);
    }
    char *tmpCh = (char *) src;
    for (uint16_t j=0; j < dataLength; j++) {
        if (tmpCh[j] < 32 || tmpCh[j] > 126) {
            dst[pos+j] = '?';
        } else {
            dst[pos+j] = tmpCh[j];
        }
    }
    dst[dataLength + pos] = '\0';
    return dataLength + pos;
}

/** Convert 1 hex byte to 2 characters */
char hex2ascii(uint8_t input){
    return input > 9 ? (char)(input + 87) : (char)(input + 48);
}

uint16_t sprintf_hex2ascii(char *dst, uint16_t maxLen, uint8_t *src, uint16_t dataLength) {
    if (2 * dataLength > maxLen - 1) {
        THROW(EXCEPTION_OVERFLOW);
    }
    for (uint16_t j=0; j < dataLength; j++) {
        dst[2*j] = hex2ascii((src[j] & 0xf0) >> 4);
        dst[2*j+1] = hex2ascii(src[j] & 0x0f);
    }
    dst[2*dataLength] = '\0';
    return 2*dataLength;
}


uint16_t sprintf_mosaic(char *dst, uint16_t maxLen, uint8_t *mosaic, uint16_t dataLength) {
    //mosaic = mosaic name + amount (uint64)
    uint16_t mosaicNameLen = dataLength - 8;
    uint16_t len = sprintf_number(dst, maxLen, read_uint64(mosaic + mosaicNameLen));
    return len+mosaicNameLen;
}

uint64_t read_uint64(uint8_t *src) {
    uint64_t value ;
    value = src[7] ;
    value = (value << 8 ) + src[6] ;
    value = (value << 8 ) + src[5] ;
    value = (value << 8 ) + src[4] ;
    value = (value << 8 ) + src[3] ;
    value = (value << 8 ) + src[2] ;
    value = (value << 8 ) + src[1] ;
    value = (value << 8 ) + src[0] ;
    return value ;
}

uint8_t read_uint8(uint8_t *src) {
    return (uint8_t) *((uint8_t *)src);
}

uint16_t read_uint16(uint8_t *src) {
    return (uint16_t) *((uint16_t *)src);
}

uint32_t read_uint32(uint8_t *src) {
    return (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
}
