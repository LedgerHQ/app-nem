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
#ifndef LEDGER_APP_NEM_FIELDS_H
#define LEDGER_APP_NEM_FIELDS_H

#include <stdint.h>
#include <string.h>

// Normal field types
#define STI_INT8 0x01
#define STI_UINT8 0x02
#define STI_UINT16 0x03
#define STI_UINT32 0x04
#define STI_UINT64 0x05
#define STI_HASH256 0x06
#define STI_PUBLICKEY 0x07
#define STI_STR 0x17
// Custom field types
#define STI_NEM 0xA0
#define STI_MOSAIC_COUNT 0xA1
#define STI_MOSAIC_CURRENCY 0xA2
#define STI_MESSAGE 0xA3
#define STI_ADDRESS 0xA4
#define STI_PROPERTY 0xA5

// Small collection of used field IDs
// INT8 defines

// UINT8 defines

// UINT32 defines
#define NEM_UINT32_TRANSACTION_TYPE 0x30
#define NEM_UINT32_INNER_TRANSACTION_TYPE 0x31
#define NEM_UINT32_AM_MODICATION_TYPE 0x32
#define NEM_UINT32_AM_RELATIVE_CHANGE 0x33
#define NEM_UINT32_AM_COSIGNATORY_NUM 0x34
#define NEM_UINT32_IT_MODE 0x35
#define NEM_UINT32_MOSAIC_COUNT 0x36
#define NEM_UINT32_LEVY_FEE_TYPE 0x37
#define NEM_UINT32_MSC_TYPE 0x38

// UINT64 defines
#define NEM_UINT64_TXN_FEE 0x70
#define NEM_UINT64_MULTISIG_FEE 0x71
#define NEM_UINT64_DURATION 0x72
#define NEM_UINT64_RENTAL_FEE 0x73
#define NEM_UINT64_LEVY_FEE 0x74

// PUBLICKEY defines
#define NEM_PUBLICKEY_IT_REMOTE 0x80
#define NEM_PUBLICKEY_AM_COSIGNATORY 0x81

// String defines
#define NEM_STR_RECIPIENT_ADDRESS 0x90
#define NEM_STR_TXN_MESSAGE 0x91
#define NEM_STR_ENC_MESSAGE 0x92
#define NEM_STR_MULTISIG_ADDRESS 0x93
#define NEM_STR_NAMESPACE 0x94
#define NEM_STR_PARENT_NAMESPACE 0x95
#define NEM_STR_ROOT_NAMESPACE 0x96
#define NEM_STR_SINK_ADDRESS 0x97
#define NEM_STR_MOSAIC 0x98
#define NEM_STR_DESCRIPTION 0x99
#define NEM_STR_PROPERTY 0x9A
#define NEM_STR_LEVY_MOSAIC 0x9B
#define NEM_STR_LEVY_ADDRESS 0x9C
#define NEM_STR_TRANSFER_MOSAIC 0x9D

// Hash defines
#define NEM_HASH256 0xB0

// Mosaic defines
#define NEM_MOSAIC_AMOUNT 0xD0
#define NEM_MOSAIC_UNITS 0xD1
#define NEM_MOSAIC_CREATE_SUPPLY_DELTA 0xD2
#define NEM_MOSAIC_DELETE_SUPPLY_DELTA 0xD3
#define NEM_MOSAIC_UNKNOWN_TYPE 0xD4

typedef struct {
    uint8_t id;
    uint8_t dataType;
    uint16_t length;
    const uint8_t *data;
} field_t;

// Simple macro for building more readable switch statements
#define CASE_FIELDNAME(v,src) case v: snprintf(dst, MAX_FIELDNAME_LEN, "%s", src); return;

void resolve_fieldname(const field_t *field, char* dst);

#endif //LEDGER_APP_NEM_FIELDS_H
