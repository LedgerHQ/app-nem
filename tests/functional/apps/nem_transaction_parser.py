from struct import unpack

UINT32_MAX = 0xffffffff

TRANSACTION_TYPES = {
    0x0101: 'TRANSFER',
    0x0801: 'IMPORTANCE_TRANSFER',
    0x1001: 'MULTISIG_AGGREGATE_MODIFICATION',
    0x1002: 'MULTISIG_SIGNATURE',
    0x1004: 'MULTISIG',
    0x2001: 'PROVISION_NAMESPACE',
    0x4001: 'MOSAIC_DEFINITION',
    0x4002: 'MOSAIC_SUPPLY_CHANGE',
}


def read_uint8_t(buffer):
    return buffer[1:], unpack('<B', buffer[:1])[0]


def read_uint16_t(buffer):
    return buffer[2:], unpack('<H', buffer[:2])[0]


def read_uint32_t(buffer):
    return buffer[4:], unpack('<I', buffer[:4])[0]


def read_uint64_t(buffer):
    return buffer[8:], unpack('<Q', buffer[:8])[0]


def read_len_prefixed_data(buffer):
    buffer, length = read_uint32_t(buffer)
    return buffer[length:], buffer[:length]


def read_len_prefixed_string(buffer):
    buffer, data = read_len_prefixed_data(buffer)
    return buffer, data.decode('utf-8')


def read_address(buffer):
    buffer, string = read_len_prefixed_string(buffer)
    assert len(string) == 40
    return buffer, string


def read_public_key(buffer):
    buffer, public_key = read_len_prefixed_data(buffer)
    assert len(public_key) == 32
    return buffer, public_key.hex()


def decode_common_txn_header(buffer):
    buffer, value = read_uint32_t(buffer)
    transactionType = TRANSACTION_TYPES[value]
    buffer, version = read_uint8_t(buffer)
    buffer, reserved = read_uint16_t(buffer)
    buffer, networkType = read_uint8_t(buffer)
    buffer, timestamp = read_uint32_t(buffer)
    buffer, public_key = read_public_key(buffer)
    buffer, fee = read_uint64_t(buffer)
    buffer, deadline = read_uint32_t(buffer)

    data = {
        "transactionType": transactionType,
        "version": version,
        "networkType": networkType,
        "timestamp": timestamp,
        "public_key": public_key,
        "fee": fee,
        "deadline": deadline
    }
    return buffer, data


def decode_transfer_transaction(buffer, version):
    buffer, recipient = read_address(buffer)
    buffer, amount = read_uint64_t(buffer)

    data = {
        "recipient": recipient,
        "amount": amount
    }

    buffer, payload_len = read_uint32_t(buffer)
    if payload_len:
        payload_data = buffer[:payload_len]
        buffer = buffer[payload_len:]
        payload_data, payloadType = read_uint32_t(payload_data)
        payload_data, payload = read_len_prefixed_data(payload_data)

        data["payloadType"] = payloadType
        data["payload"] = payload.hex()

    if version == 2:
        buffer, mosaicListLen = read_uint32_t(buffer)
        mosaicList = []
        for _ in range(mosaicListLen):
            buffer, _ = read_uint32_t(buffer)  # length not check for now
            buffer, _ = read_uint32_t(buffer)  # length not check for now
            buffer, namespace = read_len_prefixed_string(buffer)
            buffer, mosaicName = read_len_prefixed_string(buffer)
            buffer, quantity = read_uint64_t(buffer)
            mosaicList.append({
                "namespace": namespace,
                "mosaicName": mosaicName,
                "quantity": quantity
            })

        data["mosaicList"] = mosaicList

    return buffer, data


def decode_importance_transfer_transaction(buffer):
    buffer, iMode = read_uint32_t(buffer)
    buffer, iPublicKey = read_public_key(buffer)
    data = {
        "iMode": iMode,
        "iPublicKey": iPublicKey
    }
    return buffer, data


def decode_aggregate_modification_transaction(buffer, version):
    buffer, cmListLen = read_uint32_t(buffer)
    cmList = []
    for _ in range(cmListLen):
        buffer, cmsLen = read_uint32_t(buffer)  # not used?
        buffer, amType = read_uint32_t(buffer)
        buffer, amPublicKey = read_public_key(buffer)
        cmList.append({
            "cmsLen": cmsLen,
            "amType": amType,
            "amPublicKey": amPublicKey
        })
    data = {"cmList": cmList}

    if version == 2:
        buffer, cmLen = read_uint32_t(buffer)  # only 0 or != 0 used
        data["cmLen"] = cmLen
        if cmLen > 0:
            buffer, minCm = read_uint32_t(buffer)
            data["minCm"] = minCm
    return buffer, data


def decode_multisig_signature_transaction(buffer):
    buffer, hashObjLen = read_uint32_t(buffer)  # not used?
    buffer, hashdata = read_len_prefixed_data(buffer)
    buffer, msAddress = read_address(buffer)

    buffer, transactions = decode_multisig_transaction(buffer)
    data = {
        'hashObjLen': hashObjLen,
        'hash': hashdata.hex(),
        'msAddress': msAddress
    }
    data.update(transactions)
    return buffer, data


def decode_provision_namespace_transaction(buffer):
    buffer, rAddress = read_address(buffer)
    buffer, rentalFee = read_uint64_t(buffer)
    buffer, namespace = read_len_prefixed_string(buffer)

    data = {
        'rAddress': rAddress,
        'rentalFee': rentalFee,
        'namespace': namespace
    }

    _, length = read_uint32_t(buffer)
    if length == UINT32_MAX:
        # drop the 4read bytes
        buffer, _ = read_uint32_t(buffer)
    else:
        buffer, parentNamespace = read_len_prefixed_string(buffer)
        data['parentNamespace'] = parentNamespace

    return buffer, data


def decode_mosaic_definition_creation_transaction(buffer):
    buffer, mds_data_len = read_uint32_t(buffer)
    mds_data = buffer[:mds_data_len]
    buffer = buffer[mds_data_len:]

    mds_data, mdcPublicKey = read_public_key(mds_data)
    mds_data, _ = read_uint32_t(mds_data)  # length not check for now
    mds_data, namespace = read_len_prefixed_string(mds_data)
    mds_data, mosaicName = read_len_prefixed_string(mds_data)
    mds_data, description = read_len_prefixed_string(mds_data)

    data = {
        'mdcPublicKey': mdcPublicKey,
        'namespace': namespace,
        'mosaicName': mosaicName,
        'description': description
    }

    properties = []
    mds_data, propertiesLen = read_uint32_t(mds_data)
    for _ in range(propertiesLen):
        mds_data, _ = read_uint32_t(mds_data)  # length not check for now
        mds_data, propertyName = read_len_prefixed_string(mds_data)
        mds_data, propertyVal = read_len_prefixed_string(mds_data)

        properties.append({
            'propertyName': propertyName,
            'propertyVal': propertyVal
        })

    if properties:
        data['properties'] = properties

    mds_data, levyLen = read_uint32_t(mds_data)
    if levyLen:
        mds_data, feeType = read_uint32_t(mds_data)
        mds_data, lsAddress = read_address(mds_data)

        mds_data, _ = read_uint32_t(mds_data)  # length not check for now
        mds_data, namespace = read_len_prefixed_string(mds_data)
        mds_data, mosaicName = read_len_prefixed_string(mds_data)

        mds_data, fee = read_uint64_t(mds_data)
        data['levy'] = {
            'feeType': feeType,
            'lsAddress': lsAddress,
            'namespace': namespace,
            'mosaicName': mosaicName,
            'fee': fee
        }

    assert not mds_data

    buffer, mdAddress = read_address(buffer)
    data['mdAddress'] = mdAddress
    buffer, mdfee = read_uint64_t(buffer)
    data['mdfee'] = mdfee

    return buffer, data


def decode_mosaic_supply_change_transaction(buffer):
    buffer, _ = read_uint32_t(buffer)  # length not check for now
    buffer, namespace = read_len_prefixed_string(buffer)
    buffer, mosaicName = read_len_prefixed_string(buffer)
    buffer, supplyType = read_uint32_t(buffer)
    buffer, supplyDelta = read_uint64_t(buffer)

    data = {
        'namespace': namespace,
        'mosaicName': mosaicName,
        'supplyType': supplyType,
        'supplyDelta': supplyDelta
    }

    return buffer, data


def decode_multisig_transaction(buffer):
    buffer, subdataLen = read_uint32_t(buffer)
    subbuffer = buffer[:subdataLen]
    buffer = buffer[subdataLen:]

    subdataList = []

    while subbuffer:
        subbuffer, subdata = _decode_txn_context(subbuffer)
        subdataList.append(subdata)

    return buffer, {'transactions': subdataList}


def decode_txn_detail(buffer, transaction_type, version):
    if transaction_type == 'TRANSFER':
        return decode_transfer_transaction(buffer, version)
    elif transaction_type == 'IMPORTANCE_TRANSFER':
        return decode_importance_transfer_transaction(buffer)
    elif transaction_type == 'MULTISIG_AGGREGATE_MODIFICATION':
        return decode_aggregate_modification_transaction(buffer, version)
    elif transaction_type == 'MULTISIG_SIGNATURE':
        return decode_multisig_signature_transaction(buffer)
    elif transaction_type == 'MULTISIG':
        return decode_multisig_transaction(buffer)
    elif transaction_type == 'PROVISION_NAMESPACE':
        return decode_provision_namespace_transaction(buffer)
    elif transaction_type == 'MOSAIC_DEFINITION':
        return decode_mosaic_definition_creation_transaction(buffer)
    elif transaction_type == 'MOSAIC_SUPPLY_CHANGE':
        return decode_mosaic_supply_change_transaction(buffer)
    assert False


def _decode_txn_context(buffer):
    buffer, header = decode_common_txn_header(buffer)
    buffer, fields = decode_txn_detail(buffer, header["transactionType"], header["version"])
    return buffer, {'common_txn_header': header, 'fields': fields}


def decode_txn_context(buffer):
    buffer, data = _decode_txn_context(buffer)
    assert len(buffer) == 0
    return data
