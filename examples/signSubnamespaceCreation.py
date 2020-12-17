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

TEST_TX =  "0120000001000098C46BAE0A200000009F96DF7E7A639B4034B8BEE5B88AB1D640DB66EB5A47AFE018E320CB130C183DF049020000000000D479AE0A2800000054414D4553504143455748344D4B464D42435646455244504F4F5034464B374D54444A455950333580969800000000000C0000007375626E616D65737061636508000000746573745F6E656D"

print("-= NEM Ledger =-")
print("Sign a testnet sub-namespace creation transaction")
print("Please confirm on your Ledger Nano S")

send_sign_package(TESTNET, TEST_TX)
