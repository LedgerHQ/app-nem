/*******************************************************************************
*    NEM Wallet
*    (c) 2020 FDS
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

#include "nem_parse.h"
#include "apdu/global.h"
#include "nem/format/readers.h"

#pragma pack(push, 1)

typedef struct address_t {
    //Length of address (always 40)
    uint32_t length;
    //Address: 40 bytes (using UTF8 encoding).
    uint8_t address[NEM_ADDRESS_LENGTH];
} address_t;

typedef struct publickey_t {
    //Length of public key byte array (always 32)
    uint32_t length;
    //Public key bytes: 32 bytes.
    uint8_t publicKey[NEM_PUBLIC_KEY_LENGTH];
} publickey_t;

typedef struct transfer_txn_header_t {
    address_t recipient;
    //Amount (micro nem)
    uint64_t amount;
    //Length of message
    uint32_t msgLen;
} transfer_txn_header_t;

typedef struct importance_txn_header_t {
    //Importance transfer mode. The following modes are supported: 0x01 (Activate), 0x02 (Deactivate)
    uint32_t iMode;
    publickey_t iPublicKey;
} importance_txn_header_t;

typedef struct aggregate_modication_header_t {
    //Length of cosignatory modification structure
    uint32_t cmsLen;
    //Modification type
    uint32_t amType;
    publickey_t amPublicKey;
} aggregate_modication_header_t;

typedef struct multsig_signature_header_t {
    //Length of hash object (hash of the corresponding multisig transaction
    uint32_t hashObjLen;
    //Length of hash
    uint32_t hashLen;
    //SHA3 hash bytes: 32 bytes
    uint8_t hash[NEM_TRANSACTION_HASH_LENGTH];
    //Multisig account address (using UTF8 encoding)
    address_t msAddress;
} multsig_signature_header_t;

typedef struct rental_header_t {
    //Address bytes of rental fee sink
    address_t rAddress;
    //Rental fee (Root always: 100000000, Sub always: 10000000) for namespace
    uint64_t rentalFee;
} rental_header_t;

typedef struct mosaic_definition_creation_t {
    //Length of mosaic definition structure
    uint32_t defStructLen;
    //Public keyof creator
    publickey_t mdcPublicKey;
    //Length of mosaic id structure
    uint32_t idStructLen;
    //Length of namespace id string
    uint32_t nsIdLen;
} mosaic_definition_creation_t;

typedef struct levy_structure_t {
    //Fee type: The following fee types are supported. 0x01 (absolute fee), 0x02 (percentile fee)
    uint32_t feeType;
    address_t lsAddress;
    //Length of mosaic id structure
    uint32_t msIdLen;
} levy_structure_t;

typedef struct mosaic_definition_sink_t {
    address_t mdAddress;
    uint64_t fee;
} mosaic_definition_sink_t;

typedef struct common_txn_header_t {
    //transaction type
    uint32_t transactionType;
    //nem version: 1 or 2
    uint8_t version;
    uint16_t reserve;
    //network type: mainnet, testnet
    uint8_t networkType;
    uint32_t timestamp;
    publickey_t publicKey;
    uint64_t fee;
    uint32_t deadline;
} common_txn_header_t;

#pragma pack(pop)

// Security check
bool has_data(parse_context_t *context, uint32_t numBytes) {
    return context->offset + numBytes - 1 < context->length;
}

field_t *get_field(parse_context_t *context, int idx) {
    return &context->result.fields[idx];
}

field_t* _set_field_data(field_t* field, uint8_t id, uint8_t data_type, uint16_t length, uint8_t* data) {
    field->id = id;
    field->dataType = data_type;
    field->length = length;
    field->data = data;
    return field;
}

field_t* set_field_data(parse_context_t *context, int idx, uint8_t id, uint8_t data_type, uint16_t length, uint8_t* data) {
    return _set_field_data(get_field(context, idx), id, data_type, length, data);
}

field_t *add_new_field(parse_context_t *context, uint8_t id, uint8_t data_type, uint16_t length, uint8_t* data) {
    return set_field_data(context, context->result.numFields++, id, data_type, length, data);
}

// Read data and security check
uint8_t* read_data(parse_context_t *context, uint32_t numBytes) {
    if (has_data(context, numBytes)) { // Security check
        uint32_t offset = context->offset;
        context->offset += numBytes;
        // PRINTF("******* Read: %d bytes - Move offset: %d->%d/%d\n", numBytes, offset, context->offset, context->length);
        return context->data + offset;
    } else {
        THROW(EXCEPTION_OVERFLOW);
    }
}

// Read uint32 and security check
uint32_t _read_uint32(parse_context_t *context) {
    return read_uint32(read_data(context, sizeof(uint32_t))); // Read data and security check
}

// Move position and security check
uint8_t* move_pos(parse_context_t *context, uint32_t numBytes) {
    return read_data(context, numBytes); // Read data and security check
}

void parse_transfer_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    transfer_txn_header_t *txn = (transfer_txn_header_t *) read_data(context, sizeof(transfer_txn_header_t)); // Read data and security check
    char str[32];
    uint8_t *ptr;
    // Show Recipient address
    add_new_field(context, NEM_STR_RECIPIENT_ADDRESS, STI_ADDRESS, NEM_ADDRESS_LENGTH, (uint8_t*) &txn->recipient.address);
    if (common_header->version == 1) { // NEM tranfer tx version 1
        // Show xem amount
        add_new_field(context, NEM_MOSAIC_AMOUNT, STI_NEM, sizeof(uint64_t), (uint8_t*) &txn->amount);
    }
    if (txn->msgLen == 0) {
        // empty msg
        add_new_field(context, NEM_STR_TXN_MESSAGE, STI_MESSAGE, 0, NULL);
    } else {
        uint32_t payloadType = _read_uint32(context); // Read uint32 and security check
        uint32_t payloadLength = _read_uint32(context); // Read uint32 and security check
        if (payloadType == 1) {
            // Show Message
            add_new_field(context, NEM_STR_TXN_MESSAGE, STI_MESSAGE, payloadLength, read_data(context, payloadLength)); // Read data and security check
        } else { //show <encrypted msg>
            add_new_field(context, NEM_STR_ENC_MESSAGE, STI_MESSAGE, 0, NULL);
        }
    }
    // Show fee
    add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &common_header->fee);
    if (common_header->version == 2) { //NEM tranfer tx version 2
        // num of mosaic pointer
        ptr = read_data(context, sizeof(uint32_t)); // Read data and security check
        uint32_t numMosaic = read_uint32(ptr);
        if (numMosaic == 0) {
            // Show xem amount
            add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &txn->amount);
        } else {
            // Show sent other mosaic num
            add_new_field(context, NEM_UINT32_MOSAIC_COUNT, STI_UINT32, sizeof(uint32_t), ptr);
            for (uint32_t i = 0; i < numMosaic; i++) {
                // mosaic structure length pointer
                move_pos(context, sizeof(uint32_t)); // Move position and security check
                // mosaicId structure length pointer
                move_pos(context, sizeof(uint32_t)); // Move position and security check
                // namespaceID length pointer
                ptr = read_data(context, sizeof(uint32_t)); // Read data and security check
                uint32_t nsIdLen = read_uint32(ptr);
                // namespaceID pointer
                ptr = read_data(context, nsIdLen); // Read data and security check
                sprintf_ascii(str, 32, ptr, nsIdLen);
                uint8_t is_nem = 0; //namespace is nem
                if (strcmp(str, "nem") == 0) {
                    is_nem = 1;
                }
                // mosaic name length pointer
                ptr = read_data(context, sizeof(uint32_t)); // Read data and security check
                uint32_t mosaicNameLen = read_uint32(ptr);
                // mosaic name and quantity
                ptr = read_data(context, mosaicNameLen + sizeof(uint64_t)); // Read data and security check
                sprintf_ascii(str, 32, ptr, mosaicNameLen);
                if (is_nem == 1 && strcmp(str, "xem") == 0) {
                    // xem quantity
                    add_new_field(context, NEM_MOSAIC_AMOUNT, STI_NEM, sizeof(uint64_t), (uint8_t *)(ptr + mosaicNameLen));
                } else {
                    // mosaic name and quantity
                    add_new_field(context, NEM_MOSAIC_UNITS, STI_MOSAIC_CURRENCY, mosaicNameLen + sizeof(uint64_t), ptr);
                }
            }
        }
    }
}

void parse_importance_transfer_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    importance_txn_header_t *txn = (importance_txn_header_t*) read_data(context, sizeof(importance_txn_header_t)); // Read data and security check
    //  Show importance transfer mode
    add_new_field(context, NEM_UINT32_IT_MODE, STI_UINT32, sizeof(uint8_t), (uint8_t*) &txn->iMode);
    // Show public key of remote account
    add_new_field(context, NEM_PUBLICKEY_IT_REMOTE, STI_HASH256, NEM_PUBLIC_KEY_LENGTH, (uint8_t*) &txn->iPublicKey.publicKey);
    // Show fee
    add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &common_header->fee);
}

void parse_aggregate_modification_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    uint8_t *ptr = read_data(context, sizeof(uint32_t)); // Read data and security check
    uint32_t cmNum = read_uint32(ptr);
    // Show number of cosignatory modification
    add_new_field(context, NEM_UINT32_AM_COSIGNATORY_NUM, STI_UINT32, sizeof(uint32_t), ptr);
    for (uint32_t i = 0; i < cmNum; i++) {
        aggregate_modication_header_t *txn = (aggregate_modication_header_t*) read_data(context, sizeof(aggregate_modication_header_t)); // Read data and security check
        //  Show modification type
        add_new_field(context, NEM_UINT32_AM_MODICATION_TYPE, STI_UINT32, sizeof(uint32_t), (uint8_t*) &txn->amType);
        // Show public key of cosignatory
        add_new_field(context, NEM_PUBLICKEY_AM_COSIGNATORY, STI_HASH256, NEM_PUBLIC_KEY_LENGTH, (uint8_t*) &txn->amPublicKey.publicKey);
    }
    if (common_header->version == 2) {
        ptr = read_data(context, sizeof(uint32_t)); // Read data and security check
        uint32_t cmLen = read_uint32(ptr);
        if (cmLen > 0) {
            // Show relative change in minimum cosignatories modification structure
            add_new_field(context, NEM_UINT32_AM_RELATIVE_CHANGE, STI_UINT32, sizeof(uint32_t), read_data(context, sizeof(uint32_t))); // Read data and security check
        } else {
            // Show no minimum cosignatories modification
            add_new_field(context, NEM_UINT32_AM_RELATIVE_CHANGE, STI_UINT32, sizeof(uint32_t), ptr);
        }
    }
    // Show fee
    add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &common_header->fee);
}

void parse_multisig_signature_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    multsig_signature_header_t *txn = (multsig_signature_header_t*) read_data(context, sizeof(multsig_signature_header_t)); // Read data and security check
    // Show sha3 hash
    add_new_field(context, NEM_HASH256, STI_HASH256, txn->hashLen, (uint8_t*) &txn->hash);
    // Show multisig address
    add_new_field(context, NEM_STR_MULTISIG_ADDRESS, STI_ADDRESS, NEM_ADDRESS_LENGTH, (uint8_t*) &txn->msAddress.address);
    // Show fee
    add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &common_header->fee);
}

void parse_provision_namespace_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    rental_header_t *txn = (rental_header_t*) read_data(context, sizeof(rental_header_t)); // Read data and security check
    uint32_t len = _read_uint32(context); // Read uint32 and security check
    // New part string
    add_new_field(context, NEM_STR_NAMESPACE, STI_STR, len, read_data(context, len)); // Read data and security check
    len = _read_uint32(context);  // Read uint32 and security check
    if (len == UINT32_MAX) {
        // Show create new root namespace
        add_new_field(context, NEM_STR_ROOT_NAMESPACE, STI_STR, 0, NULL);
    } else {
        // Show parent namespace string
        add_new_field(context, NEM_STR_PARENT_NAMESPACE, STI_STR, len, read_data(context, len)); // Read data and security check
    }
    // Show sink address
    add_new_field(context, NEM_STR_SINK_ADDRESS, STI_ADDRESS, txn->rAddress.length, (uint8_t*) &txn->rAddress.address);
    // Show rental fee
    add_new_field(context, NEM_UINT64_RENTAL_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &txn->rentalFee);
    // Show fee./
    add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &common_header->fee);
}

void parse_mosaic_definition_creation_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    uint8_t* ptr;
    mosaic_definition_creation_t *txn = (mosaic_definition_creation_t*) read_data(context, sizeof(mosaic_definition_creation_t)); // Read data and security check
    // Show namespace id string
    add_new_field(context, NEM_STR_PARENT_NAMESPACE, STI_STR, txn->nsIdLen, read_data(context, txn->nsIdLen)); // Read data and security check
    uint32_t len = read_uint32(read_data(context, sizeof(uint32_t))); // Read data and security check
    // Show mosaic name string
    add_new_field(context, NEM_STR_MOSAIC, STI_STR, len, read_data(context, len)); // Read data and security check
    len = read_uint32(read_data(context, sizeof(uint32_t))); // Read data and security check
    // Show description string
    add_new_field(context, NEM_STR_DESCRIPTION, STI_STR, len, read_data(context, len)); // Read data and security check
    uint32_t propertyNum = _read_uint32(context); // Read uint32 and security check
    for (uint32_t i = 0; i < propertyNum; i++) {
        // Length of the property structure
        move_pos(context, sizeof(uint32_t)); // Move position and security check
        // Length of the property name
        ptr = read_data(context, sizeof(uint32_t)); // Read data and security check
        len = read_uint32(ptr);
        // Show property name string
        move_pos(context, len); // Move position and security check
        len = _read_uint32(context); // Read uint32 and security check
        // Show property value string
        move_pos(context, len); // Move position and security check
        // data = len name, name, len value, value (ignore length)
        add_new_field(context, NEM_STR_PROPERTY, STI_PROPERTY, sizeof(uint32_t), ptr);
    }
    // Levy structure length
    len = _read_uint32(context);  // Read uint32 and security check
    if(len > 0) {
        if (has_data(context, len)) { // Security check
            levy_structure_t *levy = (levy_structure_t*) read_data(context, sizeof(levy_structure_t)); // Read data and security check
            //Length of namespace id string
            ptr = read_data(context, sizeof(uint32_t)); // Read data and security check
            uint32_t nsid_len = read_uint32(ptr);
            //namespaceid
            move_pos(context, nsid_len); // Move position and security check
            uint32_t mn_len = _read_uint32(context); // Read uint32 and security check
            //mosaic name
            move_pos(context, mn_len); // Move position and security check
            // Show levy mosaic: namespace:mosaic name, data=len nemspaceid, nemspaceid, len mosaic name, mosiac name
            add_new_field(context, NEM_STR_LEVY_MOSAIC, STI_STR, nsid_len + mn_len + 2 * sizeof(uint32_t), (uint8_t *) ptr);
            // Show levy address
            add_new_field(context, NEM_STR_LEVY_ADDRESS, STI_ADDRESS, NEM_ADDRESS_LENGTH, (uint8_t*) &levy->lsAddress.address);
            // Show levy fee type
            add_new_field(context, NEM_UINT32_LEVY_FEE_TYPE, STI_UINT32, sizeof(uint32_t), (uint8_t*) &levy->feeType);
            // Show levy fee
            add_new_field(context, NEM_UINT64_LEVY_FEE, STI_NEM, sizeof(uint64_t), read_data(context, sizeof(uint64_t))); // Read data and security check
        }  else {
            THROW(EXCEPTION_OVERFLOW);
        }
    }
    mosaic_definition_sink_t *sink = (mosaic_definition_sink_t*) read_data(context, sizeof(mosaic_definition_sink_t)); // Read data and security check
    // Show sink address
    add_new_field(context, NEM_STR_SINK_ADDRESS, STI_ADDRESS, NEM_ADDRESS_LENGTH, (uint8_t*) &sink->mdAddress.address);
    // Show rentail fee
    add_new_field(context, NEM_UINT64_RENTAL_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &sink->fee);
    // Show tx fee
    add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &common_header->fee);
}

void parse_mosaic_supply_change_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    //Length of mosaic id structure
    uint32_t len = _read_uint32(context); // Read uint32 and security check
    //Length of namespace id string: 4
    len = _read_uint32(context);  // Read uint32 and security check
    // Show namespace id string
    add_new_field(context, NEM_STR_NAMESPACE, STI_STR, len, read_data(context, len)); // Read data and security check
    //Length of mosaic name string
    len = _read_uint32(context); // Read uint32 and security check
    // Show mosaic name string
    add_new_field(context, NEM_STR_MOSAIC, STI_STR, len, read_data(context, len)); // Read data and security check
    len = sizeof(uint32_t) + sizeof(uint64_t);
    // supply type and delta change
    add_new_field(context, NEM_MOSAIC_SUPPLY_DELTA, STI_MOSAIC_CURRENCY, len, read_data(context, len)); // Read data and security check
    // Show fee
    add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (uint8_t*) &common_header->fee);
}

void parse_multisig_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    // Length of inner transaction object.
    // This can be a transfer, an importance transfer or an aggregate modification transaction
    uint32_t innerTxnLength = _read_uint32(context); // Read uint32 and security check
    if (has_data(context, innerTxnLength)) { // Security check
        add_new_field(context, NEM_UINT64_MULTISIG_FEE, STI_NEM, sizeof(uint32_t), (uint8_t*) &common_header->fee);
        uint32_t innerOffset = 0;
        while (innerOffset < innerTxnLength) {
            uint32_t previousOffset = context->offset;
            // get header first
            common_txn_header_t *inner_header = (common_txn_header_t*) read_data(context, sizeof(common_txn_header_t)); // Read data and security check
            // Show inner transaction type
            add_new_field(context, NEM_UINT32_INNER_TRANSACTION_TYPE, STI_UINT32, sizeof(uint32_t), (uint8_t*) &inner_header->transactionType);
            switch (inner_header->transactionType) {
                case NEM_TXN_TRANSFER:
                    parse_transfer_transaction(context, inner_header);
                    break;
                case NEM_TXN_IMPORTANCE_TRANSFER:
                    parse_importance_transfer_transaction(context, inner_header);
                    break;
                case NEM_TXN_MULTISIG_AGGREGATE_MODIFICATION:
                    parse_aggregate_modification_transaction(context, inner_header);
                    break;
                case NEM_TXN_MULTISIG_SIGNATURE:
                    parse_multisig_signature_transaction(context, inner_header);
                    break;
                case NEM_TXN_MULTISIG:
                    parse_multisig_transaction(context, inner_header);
                    break;
                case NEM_TXN_PROVISION_NAMESPACE:
                    parse_provision_namespace_transaction(context, inner_header);
                    break;
                case NEM_TXN_MOSAIC_DEFINITION:
                    parse_mosaic_definition_creation_transaction(context, inner_header);
                    break;
                case NEM_TXN_MOSAIC_SUPPLY_CHANGE:
                    parse_mosaic_supply_change_transaction(context, inner_header);
                    break;
                default:
                    // Mask real cause behind generic error (INCORRECT_DATA)
                    THROW(0x6A80);
                    break;
            }
            innerOffset = innerOffset + context->offset - previousOffset;
        }
    } else {
        THROW(EXCEPTION_OVERFLOW);
    }
}

void parse_txn_detail(parse_context_t *context, common_txn_header_t *common_header) {
    context->result.numFields = 0;
    // Show Transaction type
    add_new_field(context, NEM_UINT32_TRANSACTION_TYPE, STI_UINT32, sizeof(uint32_t), (uint8_t*) &common_header->transactionType);
    switch (common_header->transactionType) {
        case NEM_TXN_TRANSFER:
            parse_transfer_transaction(context, common_header);
            break;
        case NEM_TXN_IMPORTANCE_TRANSFER:
            parse_importance_transfer_transaction(context, common_header);
            break;
        case NEM_TXN_MULTISIG_AGGREGATE_MODIFICATION:
            parse_aggregate_modification_transaction(context, common_header);
            break;
        case NEM_TXN_MULTISIG_SIGNATURE:
            parse_multisig_signature_transaction(context, common_header);
            break;
        case NEM_TXN_MULTISIG:
            parse_multisig_transaction(context, common_header);
            break;
        case NEM_TXN_PROVISION_NAMESPACE:
            parse_provision_namespace_transaction(context, common_header);
            break;
        case NEM_TXN_MOSAIC_DEFINITION:
            parse_mosaic_definition_creation_transaction(context, common_header);
            break;
        case NEM_TXN_MOSAIC_SUPPLY_CHANGE:
            parse_mosaic_supply_change_transaction(context, common_header);
            break;
        default:
            // Mask real cause behind generic error (INCORRECT_DATA)
            THROW(0x6A80);
            break;
    }
}

common_txn_header_t *parse_common_header(parse_context_t *context) {
    // get gen_hash and transaction_type
    common_txn_header_t *common_header = (common_txn_header_t *) read_data(context, sizeof(common_txn_header_t)); // Read data and security check
    context->version = common_header->version;
    return common_header;
}

void parse_txn_internal(parse_context_t *context) {
    common_txn_header_t* txn = parse_common_header(context);
    parse_txn_detail(context, txn);
}

void parse_txn_context(parse_context_t *context) {
    BEGIN_TRY {
        TRY {
            parse_txn_internal(context);
        }
        CATCH_OTHER(e) {
            switch (e & 0xF000u) {
                case 0x6000:
                    // Proper error, forward it further
                    THROW(e);
                default:
                    // Mask real cause behind generic error (INCORRECT_DATA)
                    THROW(0x6A80);
            }
        }
        FINALLY {
        }
    }
    END_TRY
}
