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
#include <string.h>
#include <inttypes.h>
#include "format.h"
#include "fields.h"
#include "readers.h"
#include "nem/nem_helpers.h"
#include "common.h"
#include "base32.h"

typedef void (*field_formatter_t)(field_t* field, char* dst);

void uint8_formatter(field_t* field, char *dst) {
    uint8_t value = read_uint8(field->data);
    SNPRINTF(dst, "%d", value);
}

void uint32_formatter(field_t* field, char *dst) {
    uint32_t value = read_uint32(field->data);
    if (field->id == NEM_UINT32_MOSAIC_COUNT) {
        SNPRINTF(dst, "Found %d txs", value);
    } else if (field->id == NEM_UINT32_TRANSACTION_TYPE || field->id == NEM_UINT32_INNER_TRANSACTION_TYPE) {
        switch (value) {
            CASE_FIELDVALUE(NEM_TXN_TRANSFER, "Transfer TX")
            CASE_FIELDVALUE(NEM_TXN_IMPORTANCE_TRANSFER, "Importance Transfer TX")
            CASE_FIELDVALUE(NEM_TXN_MULTISIG_AGGREGATE_MODIFICATION, "Modify Multisig Aggregate TX")
            CASE_FIELDVALUE(NEM_TXN_MULTISIG_SIGNATURE, "Multi Sig. TX")
            CASE_FIELDVALUE(NEM_TXN_MULTISIG, "Multisig TX")
            CASE_FIELDVALUE(NEM_TXN_PROVISION_NAMESPACE, "Provision Namespace TX")
            CASE_FIELDVALUE(NEM_TXN_MOSAIC_DEFINITION, "Mosaic Definition TX")
            CASE_FIELDVALUE(NEM_TXN_MOSAIC_SUPPLY_CHANGE, "Mosaic Supply Change")
            default:
                SNPRINTF(dst, "%s", "Unknown");
        }
    } else if (field->id == NEM_UINT32_IT_MODE) {
        if (value == 1) {
            SNPRINTF(dst, "%s", "Activate");
        } else if (value == 2) {
            SNPRINTF(dst, "%s", "Deactivate");
        }
    } else if (field->id == NEM_UINT32_AM_MODICATION_TYPE) {
        if (value == 1) {
            SNPRINTF(dst, "%s", "Add cosignatory");
        } else if (value == 2) {
            SNPRINTF(dst, "%s", "Delete cosign.");
        }
    } else if (field->id == NEM_UINT32_AM_RELATIVE_CHANGE) {
        if (value == 0) {
            SNPRINTF(dst, "%s", "Not change");
        } else {
            SNPRINTF(dst, "%d", value);
        }
    } else if (field->id == NEM_UINT32_LEVY_FEE_TYPE) {
        if (value == 1) {
            SNPRINTF(dst, "%s", "Absolute");
        } else {
            SNPRINTF(dst, "%s", "Percentile");
        }
    } else {
        SNPRINTF(dst, "%d", value);
    }
}

void uint16_formatter(field_t* field, char *dst) {
    uint16_t value = read_uint16(field->data);
    SNPRINTF(dst, "%x", value);
}

void hash_formatter(field_t* field, char *dst) {
    sprintf_hex(dst, MAX_FIELD_LEN, field->data, field->length, 0);
}

void uint64_formatter(field_t* field, char *dst) {
    if (field->id == NEM_UINT64_DURATION) {
        uint64_t duration = read_uint64(field->data);
        if (duration == 0) {
            SNPRINTF(dst, "%s", "Unlimited");
        } else {
            uint16_t day = duration / 5760;
            uint8_t hour = (duration % 5760) / 240;
            uint8_t min = (duration % 240) / 4;
            SNPRINTF(dst, "%d%s%d%s%d%s", day, "d ", hour, "h ", min, "m");
        }
    } else {
        sprintf_hex(dst, MAX_FIELD_LEN, field->data, field->length, 1);
    }
}

void address_formatter(field_t* field, char *dst) {
    sprintf_ascii(dst, MAX_FIELD_LEN,field->data, field->length);
}

void mosaic_formatter(field_t* field, char *dst) {
    if (field->id == NEM_MOSAIC_SUPPLY_DELTA) {
        //data = supply type + delta
        sprintf_number(dst, MAX_FIELD_LEN, read_uint64(field->data + sizeof(uint32_t)));
    } else {
        //data = mosaic name + amount
        sprintf_mosaic(dst, MAX_FIELD_LEN, field->data, field->length);
    }
}

void nem_formatter(field_t* field, char *dst) {
    if (field->id == NEM_UINT64_LEVY_FEE) {
        sprintf_token(dst, MAX_FIELD_LEN, read_uint64(field->data), 6, "micro");
    } else {
        sprintf_token(dst, MAX_FIELD_LEN, read_uint64(field->data), 6, "xem");
    }
}

void msg_formatter(field_t* field, char *dst) {
    if (field->length == 0) {
        if (field->id == NEM_STR_ENC_MESSAGE) {
            SNPRINTF(dst, "%s", "<encrypted msg>");
        } else {
            SNPRINTF(dst, "%s", "<empty msg>");
        }
    } else {
        if (field->data[0] == 0xFE) { // hex message
            if (field->length - 1 >= MAX_FIELD_LEN) {
                sprintf_hex2ascii(dst, MAX_FIELD_LEN, &field->data[1], MAX_FIELD_LEN - 1);
            } else {
                sprintf_hex2ascii(dst, MAX_FIELD_LEN, &field->data[1], field->length - 1);
            }
        } else {
            if (field->length >= MAX_FIELD_LEN) {
                sprintf_ascii(dst, MAX_FIELD_LEN, &field->data[0], MAX_FIELD_LEN - 1);
            } else {
                sprintf_ascii(dst, MAX_FIELD_LEN, &field->data[0], field->length);
            }
        }
    }
}

void string_formatter(field_t* field, char *dst) {
    if (field->id == NEM_MOSAIC_UNKNOWN_TYPE) {
        SNPRINTF(dst, "%s", "Divisibility and levy cannot be shown");
    } else if (field->id == NEM_STR_ROOT_NAMESPACE) {
        SNPRINTF(dst, "%s", "namespace");
    } else if (field->id == NEM_STR_LEVY_MOSAIC || field->id == NEM_STR_TRANSFER_MOSAIC) {
        // Show levy mosaic: namespace:mosaic name
        // data=len namespace id, namespaceId, len mosaic name, mosaic name

        //read len of namespace id
        uint32_t nsid_len = read_uint32(field->data);
        //read namespace id
        sprintf_ascii(dst, MAX_FIELD_LEN, field->data + sizeof(uint32_t), nsid_len);
        strcat(dst,": ");
        //read len of mosaic name
        uint32_t ms_len = read_uint32(field->data + nsid_len + sizeof(uint32_t));
        //read mosaic name
        snprintf_ascii(dst, strlen(dst), MAX_FIELD_LEN, field->data + nsid_len + 2*sizeof(uint32_t) , ms_len);
    } else {
        if (field->length > MAX_FIELD_LEN) {
            sprintf_ascii(dst, MAX_FIELD_LEN, field->data, MAX_FIELD_LEN - 1);
        } else {
            sprintf_ascii(dst, MAX_FIELD_LEN, field->data, field->length);
        }
    }
}

void property_formatter(field_t* field, char *dst) {
    // field->data = len name, name, len value, value (ignore field->length)
    // Length of the property name
    uint32_t nameLen = read_uint32(field->data);
    // Length of the property name
    uint32_t valueLen = read_uint32(field->data + sizeof(uint32_t) + nameLen);
    if (valueLen > MAX_FIELD_LEN) {
        sprintf_ascii(dst, MAX_FIELD_LEN, field->data + nameLen + 2 * sizeof(uint32_t), MAX_FIELD_LEN - 1);
    } else {
        sprintf_ascii(dst, MAX_FIELD_LEN, field->data + nameLen + 2 * sizeof(uint32_t), valueLen);
    }
}

field_formatter_t get_formatter(field_t* field) {
    switch (field->dataType) {
        case STI_UINT8:
            return uint8_formatter;
        case STI_UINT16:
            return uint16_formatter;
        case STI_UINT32:
            return uint32_formatter;
        case STI_UINT64:
            return uint64_formatter;
        case STI_HASH256:
            return hash_formatter;
        case STI_ADDRESS:
            return address_formatter;
        case STI_MOSAIC_CURRENCY:
            return mosaic_formatter;
        case STI_NEM:
            return nem_formatter;
        case STI_MESSAGE:
            return msg_formatter;
        case STI_STR:
            return string_formatter;
        case STI_PROPERTY:
            return property_formatter;
        default:
            return NULL;
    }
}

void format_field(field_t* field, char* dst) {
    memset(dst, 0, MAX_FIELD_LEN);

    field_formatter_t formatter = get_formatter(field);
    if (formatter != NULL) {
        formatter(field, dst);
    } else {
        SNPRINTF(dst, "%s", "[Not implemented]");
    }

    // Replace a zero-length string with a space because of rendering issues
    if (dst[0] == 0x00) {
        dst[0] = ' ';
    }
}
