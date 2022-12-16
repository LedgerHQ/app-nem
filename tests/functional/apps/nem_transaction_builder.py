# Transaction builder for Ledger Nem application
# The binary transaction format is based on:
# - https://nemproject.github.io/nem-docs/pages/Developers/serialization/
# - https://bob.nem.ninja/docs/#transaction-objects
# - https://github.com/QuantumMechanics/NEM-sdk#31---create-and-prepare-transaction-objects
#
# The input, a JSON format is a custom one.
# Examples can be found in the tests/corpus directory.
# Maybe its readability could by improved by introducing more enums for network values, payload types, etc..

from struct import pack

UINT32_MAX = 0xffffffff

TRANSACTION_TYPES = {
    'TRANSFER': 0x0101,
    'IMPORTANCE_TRANSFER': 0x0801,
    'MULTISIG_AGGREGATE_MODIFICATION': 0x1001,
    'MULTISIG_SIGNATURE': 0x1002,
    'MULTISIG': 0x1004,
    'PROVISION_NAMESPACE': 0x2001,
    'MOSAIC_DEFINITION': 0x4001,
    'MOSAIC_SUPPLY_CHANGE': 0x4002,
}


def write_uint8_t(value):
    return pack('<B', value)


def write_uint16_t(value):
    return pack('<H', value)


def write_uint32_t(value):
    return pack('<I', value)


def write_uint64_t(value):
    return pack('<Q', value)


def write_len_prefixed_string(string):
    data = write_uint32_t(len(string))
    data += string.encode('utf-8')
    return data


def write_address(address):
    data = write_uint32_t(40)
    data += address.encode('utf-8')
    return data


def write_public_key(public_key):
    data = write_uint32_t(32)
    data += bytes.fromhex(public_key)
    return data


def encode_common_txn_header(header):
    data = write_uint32_t(TRANSACTION_TYPES[header['transactionType']])
    data += write_uint8_t(header['version'])
    data += write_uint16_t(0)  # reserved
    data += write_uint8_t(header['networkType'])
    data += write_uint32_t(header['timestamp'])
    data += write_public_key(header['public_key'])
    data += write_uint64_t(header['fee'])
    data += write_uint32_t(header['deadline'])
    return data, header['transactionType'], header['version']


def encode_transfer_transaction(fields, version):
    data = write_address(fields['recipient'])
    data += write_uint64_t(fields['amount'])

    payload = fields.get('payload', None)
    if not payload:
        payload_data = b''
    else:
        payload = bytes.fromhex(payload)
        payload_data = write_uint32_t(fields['payloadType'])
        payload_data += write_uint32_t(len(payload))
        payload_data += payload
    data += write_uint32_t(len(payload_data))
    data += payload_data

    if version == 2:
        mosaic_list = fields.get('mosaicList', [])
        data += write_uint32_t(len(mosaic_list))
        for mosaic in mosaic_list:
            mosaicId_data = write_len_prefixed_string(mosaic['namespace'])
            mosaicId_data += write_len_prefixed_string(mosaic['mosaicName'])

            mosaic_data = write_uint32_t(len(mosaicId_data))
            mosaic_data += mosaicId_data
            mosaic_data += write_uint64_t(mosaic['quantity'])

            data += write_uint32_t(len(mosaic_data))
            data += mosaic_data
    return data


def encode_importance_transfer_transaction(fields):
    data = write_uint32_t(fields['iMode'])
    data += write_public_key(fields['iPublicKey'])
    return data


def encode_aggregate_modification_transaction(fields, version):
    cm_list = fields['cmList']
    data = write_uint32_t(len(cm_list))
    for cm in cm_list:
        data += write_uint32_t(cm['cmsLen'])  # not used?
        data += write_uint32_t(cm['amType'])
        data += write_public_key(cm['amPublicKey'])

    if version == 2:
        data += write_uint32_t(fields['cmLen'])  # only 0 or != 0 used
        if fields['cmLen'] > 0:
            data += write_uint32_t(fields['minCm'])
    return data


def encode_multisig_signature_transaction(fields):
    data = write_uint32_t(fields['hashObjLen'])  # not used?
    hashdata = bytes.fromhex(fields['hash'])
    data += write_uint32_t(len(hashdata))
    data += hashdata
    data += write_address(fields['msAddress'])

    data += encode_multisig_transaction(fields)
    return data


def encode_provision_namespace_transaction(fields):
    data = write_address(fields['rAddress'])
    data += write_uint64_t(fields['rentalFee'])

    data += write_len_prefixed_string(fields['namespace'])
    if 'parentNamespace' in fields:
        data += write_len_prefixed_string(fields['parentNamespace'])
    else:
        data += write_uint32_t(UINT32_MAX)
    return data


def encode_mosaic_definition_creation_transaction(fields):
    mds_data = write_public_key(fields['mdcPublicKey'])

    mosaicId_data = write_len_prefixed_string(fields['namespace'])
    mosaicId_data += write_len_prefixed_string(fields['mosaicName'])
    mds_data += write_uint32_t(len(mosaicId_data))
    mds_data += mosaicId_data

    mds_data += write_len_prefixed_string(fields['description'])

    properties = fields.get('properties', [])
    mds_data += write_uint32_t(len(properties))
    for pro in properties:
        pro_data = write_len_prefixed_string(pro['propertyName'])
        pro_data += write_len_prefixed_string(pro['propertyVal'])

        mds_data += write_uint32_t(len(pro_data))
        mds_data += pro_data

    levy = fields.get('levy', {})
    if levy:
        levy_data = write_uint32_t(levy['feeType'])
        levy_data += write_address(levy['lsAddress'])

        mosaicId_data = write_len_prefixed_string(levy['namespace'])
        mosaicId_data += write_len_prefixed_string(levy['mosaicName'])
        levy_data += write_uint32_t(len(mosaicId_data))
        levy_data += mosaicId_data

        levy_data += write_uint64_t(levy['fee'])

        mds_data += write_uint32_t(len(levy_data))
        mds_data += levy_data
    else:
        mds_data += write_uint32_t(0)

    data = write_uint32_t(len(mds_data))
    data += mds_data

    data += write_address(fields['mdAddress'])
    data += write_uint64_t(fields['mdfee'])
    return data


def encode_mosaic_supply_change_transaction(fields):
    mosaicId_data = write_len_prefixed_string(fields['namespace'])
    mosaicId_data += write_len_prefixed_string(fields['mosaicName'])
    data = write_uint32_t(len(mosaicId_data))
    data += mosaicId_data

    data += write_uint32_t(fields['supplyType'])
    data += write_uint64_t(fields['supplyDelta'])
    return data


def encode_multisig_transaction(fields):
    subdata = b''
    for transaction in fields['transactions']:
        subdata += encode_txn_context(transaction)

    data = write_uint32_t(len(subdata))
    data += subdata
    return data


def encode_txn_detail(fields, transaction_type, version):
    if transaction_type == 'TRANSFER':
        return encode_transfer_transaction(fields, version)
    elif transaction_type == 'IMPORTANCE_TRANSFER':
        return encode_importance_transfer_transaction(fields)
    elif transaction_type == 'MULTISIG_AGGREGATE_MODIFICATION':
        return encode_aggregate_modification_transaction(fields, version)
    elif transaction_type == 'MULTISIG_SIGNATURE':
        return encode_multisig_signature_transaction(fields)
    elif transaction_type == 'MULTISIG':
        return encode_multisig_transaction(fields)
    elif transaction_type == 'PROVISION_NAMESPACE':
        return encode_provision_namespace_transaction(fields)
    elif transaction_type == 'MOSAIC_DEFINITION':
        return encode_mosaic_definition_creation_transaction(fields)
    elif transaction_type == 'MOSAIC_SUPPLY_CHANGE':
        return encode_mosaic_supply_change_transaction(fields)
    assert False


def encode_txn_context(transaction):
    data, transaction_type, version = encode_common_txn_header(transaction['common_txn_header'])
    data += encode_txn_detail(transaction['fields'], transaction_type, version)
    return data
