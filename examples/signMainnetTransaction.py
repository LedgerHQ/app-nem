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

MAINNET=104

parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP32 path to retrieve.")
parser.add_argument('--ed25519', help="Derive on ed25519 curve", action='store_true')
parser.add_argument("--apdu", help="Display APDU log", action='store_true')
args = parser.parse_args()

TEST_TX = "0101000001000068435dd00720000000e65806bd8a6461f9d108892bef32a6ae6ddc0c712177e3a2ce55c00f34a8cf25a086010000000000c3aed107280000004e43534e434f55374b4e53494947505a504c594a5a504f57354543464547355343504f515156525240420f00000000002000000001000000180000005369676e206d61696e6e6574207472616e73616374696f6e"

print("-= NEM Ledger =-")
print("Sign a mainnet transaction")
print("Please confirm on your Ledger Nano S")

send_sign_package(MAINNET, TEST_TX)
