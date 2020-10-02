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
#ifndef LEDGER_APP_NEM_READERS_H
#define LEDGER_APP_NEM_READERS_H

#include <stdint.h>
#include "parse/nem_parse.h"

uint16_t sprintf_hex(char *dst, uint16_t maxLen, uint8_t *src, uint16_t dataLength, uint8_t reverse);
uint16_t sprintf_ascii(char *dst, uint16_t maxLen, uint8_t *src, uint16_t dataLength);
uint16_t sprintf_number(char *dst, uint16_t maxLen, uint64_t value);
uint16_t sprintf_mosaic(char *dst, uint16_t maxLen, uint8_t *mosaic, uint16_t dataLength);
uint16_t sprintf_token(char* dst, uint16_t len, uint64_t amount, uint8_t divisibility, char* token);
uint16_t sprintf_hex2ascii(char *dst, uint16_t maxLen, uint8_t *src, uint16_t dataLength);

uint16_t snprintf_ascii(char *dst, uint16_t pos, uint16_t maxLen, uint8_t *src, uint16_t dataLength);

uint64_t read_uint64(uint8_t *src);
uint16_t read_uint16(uint8_t *src);
uint32_t read_uint32(uint8_t *src);
uint8_t read_uint8(uint8_t *src);
int8_t read_int8(uint8_t *src);

#endif //LEDGER_APP_NEM_READERS_H
