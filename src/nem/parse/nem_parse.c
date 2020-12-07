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
#include "nem/format/printers.h"
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

#define BAIL_IF(x) {int err = x; if (err) return err;}
#define BAIL_IF_ERR(x, err) {if (x) return err;}

// Security check
static bool has_data(parse_context_t *context, uint32_t numBytes) {
    if (context->offset + numBytes < context->offset) {
        return false;
    }
    return context->offset + numBytes - 1 < context->length;
}

static field_t *get_field(parse_context_t *context, int idx) {
    return &context->result.fields[idx];
}

static int _set_field_data(field_t* field, uint8_t id, uint8_t data_type, uint32_t length, const uint8_t* data) {
    field->id = id;
    field->dataType = data_type;
    field->length = length;
    field->data = data;
    return E_SUCCESS;
}

static int set_field_data(parse_context_t *context, int idx, uint8_t id, uint8_t data_type, uint32_t length, const uint8_t* data) {
    BAIL_IF_ERR(idx >= MAX_FIELD_COUNT, E_TOO_MANY_FIELDS);
    BAIL_IF_ERR(data == NULL, E_NOT_ENOUGH_DATA);
    return _set_field_data(get_field(context, idx), id, data_type, length, data);
}

static int add_new_field(parse_context_t *context, uint8_t id, uint8_t data_type, uint32_t length, const uint8_t* data) {
    return set_field_data(context, context->result.numFields++, id, data_type, length, data);
}

// Read data and security check
static const uint8_t* read_data(parse_context_t *context, uint32_t numBytes) {
    BAIL_IF_ERR(!has_data(context, numBytes), NULL);
    uint32_t offset = context->offset;
    context->offset += numBytes;
#ifdef HAVE_PRINTF
    PRINTF("******* Read: %d bytes - Move offset: %d->%d/%d\n", numBytes, offset, context->offset, context->length);
#endif
    return context->data + offset;
}

// Read uint32 and security check
static int _read_uint32(parse_context_t *context, uint32_t *result) {
    const uint8_t *p = read_data(context, sizeof(uint32_t));
    BAIL_IF_ERR(p == NULL, E_NOT_ENOUGH_DATA);
    *result = read_uint32(p);
    return E_SUCCESS;
}

// Read uint32 and security check
static int _read_uint32_ptr(parse_context_t *context, uint32_t *result, uint8_t **presult) {
    const uint8_t *p = read_data(context, sizeof(uint32_t));
    BAIL_IF_ERR(p == NULL, E_NOT_ENOUGH_DATA);
    *result = read_uint32(p);
    *presult = (uint8_t *) p;
    return E_SUCCESS;
}

// Move position and security check
static const uint8_t* move_pos(parse_context_t *context, uint32_t numBytes) {
    return read_data(context, numBytes); // Read data and security check
}

static int parse_transfer_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    char str[32];
    const uint8_t *ptr;
    const uint8_t* startPtr;
    transfer_txn_header_t *txn = (transfer_txn_header_t *) read_data(context, sizeof(transfer_txn_header_t)); // Read data and security check
    BAIL_IF_ERR(txn == NULL, E_NOT_ENOUGH_DATA);
    BAIL_IF_ERR(txn->recipient.length > NEM_ADDRESS_LENGTH, E_INVALID_DATA);
    // Show Recipient address
    BAIL_IF(add_new_field(context, NEM_STR_RECIPIENT_ADDRESS, STI_ADDRESS, NEM_ADDRESS_LENGTH, (const uint8_t *) &txn->recipient.address));
    if (common_header->version == 1) { // NEM tranfer tx version 1
        // Show xem amount
        BAIL_IF(add_new_field(context, NEM_MOSAIC_AMOUNT, STI_NEM, sizeof(uint64_t), (const uint8_t *) &txn->amount));
    }
    if (txn->msgLen == 0) {
        // empty msg
        BAIL_IF(add_new_field(context, NEM_STR_TXN_MESSAGE, STI_MESSAGE, txn->msgLen, (const uint8_t *) &txn->msgLen));
    } else {
        uint32_t payloadType, payloadLength;
        BAIL_IF(_read_uint32_ptr(context, &payloadType, (uint8_t **) &ptr));
        BAIL_IF(_read_uint32_ptr(context, &payloadLength, (uint8_t **) &ptr));
        if (payloadType == 1) {
            // Show Message
            BAIL_IF(add_new_field(context, NEM_STR_TXN_MESSAGE, STI_MESSAGE, payloadLength, read_data(context, payloadLength))); // Read data and security check
        } else { //show <encrypted msg>
            BAIL_IF(add_new_field(context, NEM_STR_ENC_MESSAGE, STI_MESSAGE, 0, (const uint8_t *) ptr));
        }
    }
    // Show fee
    BAIL_IF(add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &common_header->fee));
    if (common_header->version == 2) { //NEM tranfer tx version 2
        // num of mosaic pointer
        const uint8_t *pnumMosaic;
        uint32_t numMosaic;
        BAIL_IF(_read_uint32_ptr(context, &numMosaic, (uint8_t **) &pnumMosaic));
        if (numMosaic == 0) {
            // Show xem amount
            BAIL_IF(add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &txn->amount));
        } else {
            // Show sent other mosaic num
            BAIL_IF(add_new_field(context, NEM_UINT32_MOSAIC_COUNT, STI_UINT32, sizeof(uint32_t), (const uint8_t *) pnumMosaic));
            for (uint32_t i = 0; i < numMosaic; i++) {
                // mosaic structure length pointer
                uint32_t mosaicLen;
                BAIL_IF(_read_uint32(context, &mosaicLen));
                BAIL_IF_ERR(!has_data(context, mosaicLen), E_INVALID_DATA);
                // mosaicId structure length pointer
                uint32_t mosaicIdLen;
                BAIL_IF(_read_uint32(context, &mosaicIdLen));
                BAIL_IF_ERR(!has_data(context, mosaicIdLen), E_INVALID_DATA);
                BAIL_IF_ERR(mosaicLen - sizeof(uint32_t) - mosaicIdLen - sizeof(uint64_t) != 0, E_INVALID_DATA);
                // namespaceID length pointer
                uint32_t nsIdLen;
                BAIL_IF(_read_uint32_ptr(context, &nsIdLen, (uint8_t **) &startPtr));
                // namespaceID pointer
                ptr = read_data(context, nsIdLen); // Read data and security check
                BAIL_IF_ERR(ptr == NULL, E_NOT_ENOUGH_DATA);
                snprintf_ascii(str, 0, 32, ptr, nsIdLen);
                uint8_t is_nem = 0; //namespace is nem
                if (strcmp(str, STR_NEM) == 0) {
                    is_nem = 1;
                }
                // mosaic name length
                uint32_t mosaicNameLen;
                BAIL_IF(_read_uint32(context, &mosaicNameLen))
                BAIL_IF_ERR(mosaicIdLen - sizeof(uint32_t) - nsIdLen - sizeof(uint32_t) - mosaicNameLen != 0, E_INVALID_DATA);
                // mosaic name
                ptr = read_data(context, mosaicNameLen); // Read data and security check
                BAIL_IF_ERR(ptr == NULL, E_NOT_ENOUGH_DATA);
                snprintf_ascii(str, 0, 32, ptr, mosaicNameLen);
                if (is_nem == 1 && strcmp(str, STR_XEM) == 0) {
                    // xem quantity
                    BAIL_IF(add_new_field(context, NEM_MOSAIC_AMOUNT, STI_NEM, sizeof(uint64_t), read_data(context, sizeof(uint64_t)))); // Read data and security check
                } else {
                    // Unknow mosaic notification
                    BAIL_IF(add_new_field(context, NEM_MOSAIC_UNKNOWN_TYPE, STI_STR, 0, (const uint8_t *) &is_nem));
                    // Show mosaic information: namespace: mosaic name, data=len namespaceId, namespaceId, len mosaic name, mosaic name
                    BAIL_IF(add_new_field(context, NEM_STR_TRANSFER_MOSAIC, STI_STR, mosaicIdLen, (const uint8_t *) startPtr));
                    // Mosaic quantity
                    BAIL_IF(add_new_field(context, NEM_MOSAIC_UNITS, STI_MOSAIC_CURRENCY, sizeof(uint64_t), read_data(context, sizeof(uint64_t)))); // Read data and security check
                }
            }
        }
    }
    return E_SUCCESS;
}

static int parse_importance_transfer_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    importance_txn_header_t *txn = (importance_txn_header_t*) read_data(context, sizeof(importance_txn_header_t)); // Read data and security check
    BAIL_IF_ERR(txn == NULL, E_NOT_ENOUGH_DATA);
    BAIL_IF_ERR(txn->iPublicKey.length > NEM_PUBLIC_KEY_LENGTH, E_INVALID_DATA);
    //  Show importance transfer mode
    BAIL_IF(add_new_field(context, NEM_UINT32_IT_MODE, STI_UINT32, sizeof(uint8_t), (const uint8_t *) &txn->iMode));
    // Show public key of remote account
    BAIL_IF(add_new_field(context, NEM_PUBLICKEY_IT_REMOTE, STI_HASH256, NEM_PUBLIC_KEY_LENGTH, (const uint8_t *) &txn->iPublicKey.publicKey));
    // Show fee
    BAIL_IF(add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &common_header->fee));
    return E_SUCCESS;
}

static int parse_aggregate_modification_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    const uint8_t *pcmNum;
    uint32_t cmNum;
    BAIL_IF(_read_uint32_ptr(context, &cmNum, (uint8_t **) &pcmNum));
    // Show number of cosignatory modification
    BAIL_IF(add_new_field(context, NEM_UINT32_AM_COSIGNATORY_NUM, STI_UINT32, sizeof(uint32_t), (const uint8_t *) pcmNum));
    for (uint32_t i = 0; i < cmNum; i++) {
        aggregate_modication_header_t *txn = (aggregate_modication_header_t*) read_data(context, sizeof(aggregate_modication_header_t)); // Read data and security check
        BAIL_IF_ERR(txn == NULL, E_NOT_ENOUGH_DATA);
        BAIL_IF_ERR(txn->amPublicKey.length > NEM_PUBLIC_KEY_LENGTH, E_INVALID_DATA);
        //  Show modification type
        BAIL_IF(add_new_field(context, NEM_UINT32_AM_MODICATION_TYPE, STI_UINT32, sizeof(uint32_t), (const uint8_t *) &txn->amType));
        // Show public key of cosignatory
        BAIL_IF(add_new_field(context, NEM_PUBLICKEY_AM_COSIGNATORY, STI_HASH256, NEM_PUBLIC_KEY_LENGTH, (const uint8_t *) &txn->amPublicKey.publicKey));
    }
    if (common_header->version == 2) {
        const uint8_t *pcmLen;
        uint32_t cmLen;
        BAIL_IF(_read_uint32_ptr(context, &cmLen, (uint8_t **) &pcmLen));
        if (cmLen > 0) {
            // Show relative change in minimum cosignatories modification structure
            const uint8_t *pminCm = read_data(context, sizeof(uint32_t));
            BAIL_IF_ERR(pminCm==NULL, E_NOT_ENOUGH_DATA);
            BAIL_IF(add_new_field(context, NEM_UINT32_AM_RELATIVE_CHANGE, STI_UINT32, sizeof(uint32_t), (const uint8_t *) pminCm)); // Read data and security check
        } else {
            // Show no minimum cosignatories modification
            BAIL_IF(add_new_field(context, NEM_UINT32_AM_RELATIVE_CHANGE, STI_UINT32, sizeof(uint32_t), (const uint8_t *) pcmLen));
        }
    }
    // Show fee
    BAIL_IF(add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &common_header->fee));
    return E_SUCCESS;
}

static int parse_multisig_signature_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    multsig_signature_header_t *txn = (multsig_signature_header_t*) read_data(context, sizeof(multsig_signature_header_t)); // Read data and security check
    BAIL_IF_ERR(txn == NULL, E_NOT_ENOUGH_DATA);
    BAIL_IF_ERR(txn->msAddress.length > NEM_ADDRESS_LENGTH, E_INVALID_DATA);
    BAIL_IF_ERR(txn->hashLen > NEM_TRANSACTION_HASH_LENGTH, E_INVALID_DATA);
    // Show sha3 hash
    BAIL_IF(add_new_field(context, NEM_HASH256, STI_HASH256, txn->hashLen, (const uint8_t *) &txn->hash));
    // Show multisig address
    BAIL_IF(add_new_field(context, NEM_STR_MULTISIG_ADDRESS, STI_ADDRESS, NEM_ADDRESS_LENGTH, (const uint8_t *) &txn->msAddress.address));
    // Show fee
    BAIL_IF(add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &common_header->fee));
    return E_SUCCESS;
}

static int parse_provision_namespace_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    rental_header_t *txn = (rental_header_t*) read_data(context, sizeof(rental_header_t)); // Read data and security check
    BAIL_IF_ERR(txn == NULL, E_NOT_ENOUGH_DATA);
    BAIL_IF_ERR(txn->rAddress.length > NEM_ADDRESS_LENGTH, E_INVALID_DATA);
    uint32_t len;
    BAIL_IF(_read_uint32(context, &len));
    // New part string
    BAIL_IF(add_new_field(context, NEM_STR_NAMESPACE, STI_STR, len, read_data(context, len))); // Read data and security check
    BAIL_IF(_read_uint32(context, &len));
    if (len == UINT32_MAX) {
        // Show create new root namespace
        BAIL_IF(add_new_field(context, NEM_STR_ROOT_NAMESPACE, STI_STR, sizeof(uint32_t), (const uint8_t *) &len));
    } else {
        // Show parent namespace string
        BAIL_IF(add_new_field(context, NEM_STR_PARENT_NAMESPACE, STI_STR, len, read_data(context, len))); // Read data and security check
    }
    // Show sink address
    BAIL_IF(add_new_field(context, NEM_STR_SINK_ADDRESS, STI_ADDRESS, txn->rAddress.length, (const uint8_t *) &txn->rAddress.address));
    // Show rental fee
    BAIL_IF(add_new_field(context, NEM_UINT64_RENTAL_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &txn->rentalFee));
    // Show fee./
    BAIL_IF(add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &common_header->fee));
    return E_SUCCESS;
}

static int parse_mosaic_definition_creation_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    const uint8_t* ptr;
    uint32_t mdsLen;
    BAIL_IF(_read_uint32(context, &mdsLen));
    BAIL_IF_ERR(!has_data(context, mdsLen), E_INVALID_DATA);
    publickey_t* mdcPublicKey = (publickey_t*) read_data(context, sizeof(publickey_t));
    BAIL_IF_ERR(mdcPublicKey == NULL, E_NOT_ENOUGH_DATA);
    BAIL_IF_ERR(mdcPublicKey->length > NEM_PUBLIC_KEY_LENGTH, E_INVALID_DATA);
    //Length of mosaic id structure
    uint32_t midsLen;
    BAIL_IF(_read_uint32(context, &midsLen));
    BAIL_IF_ERR(midsLen >= mdsLen, E_INVALID_DATA);
    //Length of namespace id string
    uint32_t nsIdLen;
    BAIL_IF(_read_uint32(context, &nsIdLen));
    BAIL_IF_ERR(nsIdLen >= midsLen, E_INVALID_DATA);
    // Show namespace id string
    BAIL_IF(add_new_field(context, NEM_STR_PARENT_NAMESPACE, STI_STR, nsIdLen, read_data(context, nsIdLen))); // Read data and security check
    // Length of mosaic name string
    uint32_t mosaicNameLen;
    BAIL_IF(_read_uint32(context, &mosaicNameLen));
    BAIL_IF_ERR(mosaicNameLen >= midsLen, E_INVALID_DATA);
    // Show mosaic name string
    BAIL_IF(add_new_field(context, NEM_STR_MOSAIC, STI_STR, mosaicNameLen, read_data(context, mosaicNameLen))); // Read data and security check
    //Length of description string
    uint32_t desLen;
    BAIL_IF(_read_uint32(context, &desLen));
    BAIL_IF_ERR(desLen >= mdsLen, E_INVALID_DATA);
    // Show description string
    BAIL_IF(add_new_field(context, NEM_STR_DESCRIPTION, STI_STR, desLen, read_data(context, desLen))); // Read data and security check
    uint32_t propertyNum;
    BAIL_IF(_read_uint32(context, &propertyNum));
    for (uint32_t i = 0; i < propertyNum; i++) {
        // Length of the property structure
        uint32_t proStructLen;
        BAIL_IF(_read_uint32(context, &proStructLen));
        BAIL_IF_ERR(!has_data(context, proStructLen), E_INVALID_DATA);
        BAIL_IF_ERR(proStructLen >= mdsLen, E_INVALID_DATA);
        // Length of the property name
        uint32_t proNameLen;
        BAIL_IF(_read_uint32_ptr(context, &proNameLen, (uint8_t **) &ptr));
        BAIL_IF_ERR(proNameLen >= proStructLen, E_INVALID_DATA);
        // Show property name string
        BAIL_IF_ERR(move_pos(context, proNameLen) == NULL, E_NOT_ENOUGH_DATA);
        uint32_t proValLen;
        BAIL_IF(_read_uint32(context, &proValLen));
        BAIL_IF_ERR(proValLen >= proStructLen, E_INVALID_DATA);
        // Show property value string
        BAIL_IF_ERR(move_pos(context, proValLen) == NULL, E_NOT_ENOUGH_DATA);
        // data = len name, name, len value, value (ignore length)
        BAIL_IF(add_new_field(context, NEM_STR_PROPERTY, STI_PROPERTY, proStructLen, ptr));
    }
    // Levy structure length
    uint32_t levyLen;
    BAIL_IF(_read_uint32(context, &levyLen));
    if(levyLen > 0) {
        BAIL_IF_ERR(!has_data(context, levyLen), E_NOT_ENOUGH_DATA);
        levy_structure_t *levy = (levy_structure_t*) read_data(context, sizeof(levy_structure_t)); // Read data and security check
        BAIL_IF_ERR(levy == NULL, E_NOT_ENOUGH_DATA);
        BAIL_IF_ERR(levy->feeType != 1 && levy->feeType != 2, E_INVALID_DATA);
        BAIL_IF_ERR(levy->lsAddress.length > NEM_ADDRESS_LENGTH, E_INVALID_DATA);
        BAIL_IF_ERR(levy->msIdLen > mdsLen, E_INVALID_DATA);
        ptr = read_data(context, sizeof(uint32_t)); // Read data and security check
        BAIL_IF_ERR(ptr == NULL, E_NOT_ENOUGH_DATA);
        //Length of namespace id string
        uint32_t nsidLen = read_uint32(ptr);
        BAIL_IF_ERR(nsidLen > levy->msIdLen, E_INVALID_DATA);
        //namespaceid
        BAIL_IF_ERR(move_pos(context, nsidLen) == NULL, E_NOT_ENOUGH_DATA);
        uint32_t mnLen;
        BAIL_IF(_read_uint32(context, &mnLen));
        BAIL_IF_ERR(mnLen > levy->msIdLen, E_INVALID_DATA);
        //mosaic name
        BAIL_IF_ERR(move_pos(context, mnLen) == NULL, E_NOT_ENOUGH_DATA);
        // Show levy mosaic: namespace: mosaic name, data = len namespaceid, namespaceid, len mosaic name, mosaic name
        BAIL_IF(add_new_field(context, NEM_STR_LEVY_MOSAIC, STI_STR, levy->msIdLen, (const uint8_t *) ptr));
        // Show levy address
        BAIL_IF(add_new_field(context, NEM_STR_LEVY_ADDRESS, STI_ADDRESS, NEM_ADDRESS_LENGTH, (const uint8_t *) &levy->lsAddress.address));
        // Show levy fee type
        BAIL_IF(add_new_field(context, NEM_UINT32_LEVY_FEE_TYPE, STI_UINT32, sizeof(uint32_t), (const uint8_t *) &levy->feeType));
        // Show levy fee
        BAIL_IF(add_new_field(context, NEM_UINT64_LEVY_FEE, STI_NEM, sizeof(uint64_t), read_data(context, sizeof(uint64_t)))); // Read data and security check
    }
    mosaic_definition_sink_t *sink = (mosaic_definition_sink_t*) read_data(context, sizeof(mosaic_definition_sink_t)); // Read data and security check
    BAIL_IF_ERR(sink == NULL, E_NOT_ENOUGH_DATA);
    BAIL_IF_ERR(sink->mdAddress.length > NEM_ADDRESS_LENGTH, E_INVALID_DATA);
    // Show sink address
    BAIL_IF(add_new_field(context, NEM_STR_SINK_ADDRESS, STI_ADDRESS, NEM_ADDRESS_LENGTH, (const uint8_t *) &sink->mdAddress.address));
    // Show rentail fee
    BAIL_IF(add_new_field(context, NEM_UINT64_RENTAL_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &sink->fee));
    // Show tx fee
    BAIL_IF(add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &common_header->fee));
    return E_SUCCESS;
}

static int parse_mosaic_supply_change_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    //Length of mosaic id structure
    uint32_t msidLen;
    BAIL_IF(_read_uint32(context, &msidLen));
    //Length of namespace id string: 4
    uint32_t nsidLen;
    BAIL_IF(_read_uint32(context, &nsidLen));
    BAIL_IF_ERR(nsidLen >= msidLen, E_INVALID_DATA);
    // Show namespace id string
    BAIL_IF(add_new_field(context, NEM_STR_NAMESPACE, STI_STR, nsidLen, read_data(context, nsidLen))); // Read data and security check
    //Length of mosaic name string
    uint32_t msnLen;
    BAIL_IF(_read_uint32(context, &msnLen));
    BAIL_IF_ERR(msnLen >= msidLen, E_INVALID_DATA);
    // Show mosaic name string
    BAIL_IF(add_new_field(context, NEM_STR_MOSAIC, STI_STR, msnLen, read_data(context, msnLen))); // Read data and security check
    // supply type and delta change
    uint32_t supplyType;
    BAIL_IF(_read_uint32(context, &supplyType));
    if (supplyType == 1) {
        BAIL_IF(add_new_field(context, NEM_MOSAIC_CREATE_SUPPLY_DELTA, STI_MOSAIC_CURRENCY, sizeof(uint64_t), read_data(context, sizeof(uint64_t)))); // Read data and security check
    } else if (supplyType == 2) {
        BAIL_IF(add_new_field(context, NEM_MOSAIC_DELETE_SUPPLY_DELTA, STI_MOSAIC_CURRENCY, sizeof(uint64_t), read_data(context, sizeof(uint64_t)))); // Read data and security check
    } else {
        return E_INVALID_DATA;
    }
    // Show fee
    BAIL_IF(add_new_field(context, NEM_UINT64_TXN_FEE, STI_NEM, sizeof(uint64_t), (const uint8_t *) &common_header->fee));
    return E_SUCCESS;
}

static int parse_multisig_transaction(parse_context_t *context, common_txn_header_t *common_header) {
    // Length of inner transaction object.
    // This can be a transfer, an importance transfer or an aggregate modification transaction
    uint32_t innerTxnLength;
    BAIL_IF(_read_uint32(context, &innerTxnLength)); // Read uint32 and security check
    BAIL_IF_ERR(!has_data(context, innerTxnLength), E_NOT_ENOUGH_DATA);
    BAIL_IF(add_new_field(context, NEM_UINT64_MULTISIG_FEE, STI_NEM, sizeof(uint32_t), (const uint8_t *) &common_header->fee));
    uint32_t innerOffset = 0;
    while (innerOffset < innerTxnLength) {
        uint32_t previousOffset = context->offset;
        // get header first
        common_txn_header_t *inner_header = (common_txn_header_t*) read_data(context, sizeof(common_txn_header_t)); // Read data and security check
        BAIL_IF_ERR(inner_header == NULL, E_NOT_ENOUGH_DATA);
        // Show inner transaction type
        BAIL_IF(add_new_field(context, NEM_UINT32_INNER_TRANSACTION_TYPE, STI_UINT32, sizeof(uint32_t), (const uint8_t *) &inner_header->transactionType));
        switch (inner_header->transactionType) {
            case NEM_TXN_TRANSFER:
                BAIL_IF(parse_transfer_transaction(context, inner_header));
                break;
            case NEM_TXN_IMPORTANCE_TRANSFER:
                BAIL_IF(parse_importance_transfer_transaction(context, inner_header));
                break;
            case NEM_TXN_MULTISIG_AGGREGATE_MODIFICATION:
                BAIL_IF(parse_aggregate_modification_transaction(context, inner_header));
                break;
            case NEM_TXN_MULTISIG_SIGNATURE:
                BAIL_IF(parse_multisig_signature_transaction(context, inner_header));
                break;
            case NEM_TXN_PROVISION_NAMESPACE:
                BAIL_IF(parse_provision_namespace_transaction(context, inner_header));
                break;
            case NEM_TXN_MOSAIC_DEFINITION:
                BAIL_IF(parse_mosaic_definition_creation_transaction(context, inner_header));
                break;
            case NEM_TXN_MOSAIC_SUPPLY_CHANGE:
                BAIL_IF(parse_mosaic_supply_change_transaction(context, inner_header));
                break;
            default:
                return E_INVALID_DATA;
        }
        innerOffset = innerOffset + context->offset - previousOffset;
    }
    return E_SUCCESS;
}

static int parse_txn_detail(parse_context_t *context, common_txn_header_t *common_header) {
    int err;
    context->result.numFields = 0;
    // Show Transaction type
    BAIL_IF(add_new_field(context, NEM_UINT32_TRANSACTION_TYPE, STI_UINT32, sizeof(uint32_t), (const uint8_t *) &common_header->transactionType));
    switch (common_header->transactionType) {
        case NEM_TXN_TRANSFER:
            err = parse_transfer_transaction(context, common_header);
            break;
        case NEM_TXN_IMPORTANCE_TRANSFER:
            err = parse_importance_transfer_transaction(context, common_header);
            break;
        case NEM_TXN_MULTISIG_AGGREGATE_MODIFICATION:
            err = parse_aggregate_modification_transaction(context, common_header);
            break;
        case NEM_TXN_MULTISIG_SIGNATURE:
            err = parse_multisig_signature_transaction(context, common_header);
            break;
        case NEM_TXN_MULTISIG:
            err = parse_multisig_transaction(context, common_header);
            break;
        case NEM_TXN_PROVISION_NAMESPACE:
            err = parse_provision_namespace_transaction(context, common_header);
            break;
        case NEM_TXN_MOSAIC_DEFINITION:
            err = parse_mosaic_definition_creation_transaction(context, common_header);
            break;
        case NEM_TXN_MOSAIC_SUPPLY_CHANGE:
            err = parse_mosaic_supply_change_transaction(context, common_header);
            break;
        default:
            err = E_INVALID_DATA;
            break;
    }
    return err;
}

static common_txn_header_t *parse_common_header(parse_context_t *context) {
    // get gen_hash and transaction_type
    common_txn_header_t *common_header = (common_txn_header_t *) read_data(context, sizeof(common_txn_header_t)); // Read data and security check
    BAIL_IF_ERR(common_header == NULL, NULL);
    context->version = common_header->version;
    return common_header;
}

int parse_txn_context(parse_context_t *context) {
    common_txn_header_t* txn = parse_common_header(context);
    BAIL_IF_ERR(txn == NULL, E_NOT_ENOUGH_DATA);
    return parse_txn_detail(context, txn);
}
