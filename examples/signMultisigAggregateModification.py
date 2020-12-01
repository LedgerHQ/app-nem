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

import argparse
from base import send_sign_package

TESTNET=152
parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP32 path to retrieve.")
parser.add_argument('--ed25519', help="Derive on ed25519 curve", action='store_true')
parser.add_argument("--apdu", help="Display APDU log", action='store_true')
args = parser.parse_args()

TEST_TX =  "04100000010000988B0B690A200000009F96DF7E7A639B4034B8BEE5B88AB1D640DB66EB5A47AFE018E320CB130C183DF0490200000000009B19690A6C00000001100000010000988B0B690A20000000180158D9FEED1711FBFC7718ED144275311DCFD10A4480035D1856CDAC7242AB20A10700000000009B19690A01000000280000000100000020000000A581459B8F16974E59B110FCA581EE1FFD140A8C42B9ECDFFC316D722F0CDA35"
TEST_TX_2 =  "04100000010000985612690A200000009F96DF7E7A639B4034B8BEE5B88AB1D640DB66EB5A47AFE018E320CB130C183DF0490200000000006620690A7400000001100000020000985612690A20000000180158D9FEED1711FBFC7718ED144275311DCFD10A4480035D1856CDAC7242AB20A10700000000006620690A01000000280000000100000020000000A581459B8F16974E59B110FCA581EE1FFD140A8C42B9ECDFFC316D722F0CDA350400000001000000"
TEST_TX_3 =  "04100000010000989412690A200000009F96DF7E7A639B4034B8BEE5B88AB1D640DB66EB5A47AFE018E320CB130C183DF049020000000000A420690A7400000001100000020000989412690A20000000180158D9FEED1711FBFC7718ED144275311DCFD10A4480035D1856CDAC7242AB20A1070000000000A420690A01000000280000000100000020000000A581459B8F16974E59B110FCA581EE1FFD140A8C42B9ECDFFC316D722F0CDA3504000000FFFFFFFF"

TEST_TX_LIST = [TEST_TX, TEST_TX_2, TEST_TX_3]

for transaction in TEST_TX_LIST:
  print("-= NEM Ledger =-")
  print("Sign a testnet mosaic creation transaction with multiple mosaics")
  print("Please confirm on your Ledger Nano S")

  send_sign_package(TESTNET, transaction)