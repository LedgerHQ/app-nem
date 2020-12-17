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

parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP32 path to retrieve.")
parser.add_argument('--ed25519', help="Derive on ed25519 curve", action='store_true')
parser.add_argument("--apdu", help="Display APDU log", action='store_true')
args = parser.parse_args()

TESTNET = 152
TEST_TX =  "04100000010000985560d007200000003e6e6cbac488b8a44bdf5abf27b9e1cc2a6f20d09d550a66b9b36f525ca222eef049020000000000656ed0077400000001010000010000985560d0072000000093ce7f61acd7250f98d9ceeab18281b26fcabbc8845a6749814851626bacbf5150c3000000000000656ed007280000005441353435494341564e45554446554249484f3343454a425356495a37595948464658354c51505440420f000000000000000000"

print("-= NEM Ledger =-")
print("Sign a multisig transfer transaction")
print("Please confirm on your Ledger Nano S")

send_sign_package(TESTNET, TEST_TX)
