#!/usr/bin/env python3
# *******************************************************************************
# *   NEM Wallet
# *   (c) 2020 FDS
# *
# *  Licensed under the Apache License, Version 2.0 (the "License");
# *  you may not use this file except in compliance with the License.
# *  You may obtain a copy of the License at
# *
# *      http://www.apache.org/licenses/LICENSE-2.0
# *
# *  Unless required by applicable law or agreed to in writing, software
# *  distributed under the License is distributed on an "AS IS" BASIS,
# *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# *  See the License for the specific language governing permissions and
# *  limitations under the License.
# ********************************************************************************
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException

TESTNET=152
MAINNET=104
MIJIN_MAINNET=96
MIJIN_TESTNET=144

BIPP32_LENGTH = 20
bipp32_path = (
      "8000002C"
    + "8000002B"
    + "%s"
    + "80000000"
    + "80000000")

APDU_GET_ACCOUNT = "E0020180"
APDU_SIGN_TX = "E0040080"
APDU_GET_APP_CONFIGURATION = "E0060000ff"

dongle = getDongle(True)

def to_hex(i):
    str = '%x' % i
    if len(str) % 2 == 1:
        str = "0" + str
    return str

def get_network_bipp32(network_type):
    return hex(0x80000000 | network_type).lstrip("0x")

def get_bipp32_path(network_type):
    return bipp32_path % (get_network_bipp32(network_type))

def send_hex(params):
    return dongle.exchange(bytes(bytearray.fromhex(params)))

def send_package(apdu_hex, network_type, data_hex=None):
    params = apdu_hex
    if apdu_hex != APDU_GET_APP_CONFIGURATION:
        tx_len = to_hex(BIPP32_LENGTH+1)
        if data_hex:
            tx_len = to_hex(BIPP32_LENGTH + 1 + int(len(data_hex)/2))
        else:
            data_hex = ""
        bipp32 = get_bipp32_path(network_type)
        params += tx_len + to_hex(int(BIPP32_LENGTH/4)) + bipp32 + data_hex
    result = dongle.exchange(bytes(bytearray.fromhex(params)))
    print("Result len: " + str(len(result)))
    if apdu_hex == APDU_GET_ACCOUNT:
        print("Address [" + str(result[0]) + "] " + result[1:41].decode())
        print("PublicKey [" + str(result[41]) + "] " + result[42:74].hex().upper())
    elif apdu_hex == APDU_GET_APP_CONFIGURATION:
        print('App-Nem Version: {:d}.{:d}.{:d}'.format(result[1],result[2],result[3]))
    else:
        print("Signature: " + result.hex().upper())
    return result

def get_version():
    return send_package(APDU_GET_APP_CONFIGURATION, TESTNET)

def send_sign_package(network_type, data_hex=None):
    return send_package(APDU_SIGN_TX, network_type, data_hex)

def get_publickey(network_type):
    return send_package(APDU_GET_ACCOUNT, network_type)
