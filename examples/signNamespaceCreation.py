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

TEST_TX =  "01200000010000987162d007200000003e6e6cbac488b8a44bdf5abf27b9e1cc2a6f20d09d550a66b9b36f525ca222eef0490200000000008170d0072800000054414d4553504143455748344d4b464d42435646455244504f4f5034464b374d54444a455950333500e1f505000000000b0000007465737430303030303031ffffffff"

print("-= NEM Ledger =-")
print("Sign a testnet namespace creation transaction")
print("Please confirm on your Ledger Nano S")

send_sign_package(TESTNET, TEST_TX)
