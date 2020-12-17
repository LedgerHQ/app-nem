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

TEST_TX =  "010100000200009888AF640A200000009F96DF7E7A639B4034B8BEE5B88AB1D640DB66EB5A47AFE018E320CB130C183DF04902000000000098BD640A2800000054423749423644534A4B57425651454B3750443754574F3636454357354C59365349534D32434A4A40420F000000000014000000010000000C00000054657374206D657373616765020000001A0000000E000000030000006E656D0300000078656D40420F0000000000200000001400000007000000746573746E657405000000746F6B656E0100000000000000"
TEST_TX_2 =  "01010000020000988161d007200000003e6e6cbac488b8a44bdf5abf27b9e1cc2a6f20d09d550a66b9b36f525ca222eef049020000000000916fd007280000005441353435494341564e45554446554249484f3343454a425356495a37595948464658354c51505440420f00000000001b00000001000000130000004d6f7361696373207472616e73616374696f6e020000001a0000000e000000030000006e656d0300000078656d8096980000000000290000001d0000000f0000007861726c6565636d2e7a6f646961630600000067656d696e690a00000000000000"

TEST_TX_LIST = [TEST_TX, TEST_TX_2]

for transaction in TEST_TX_LIST:
  print("-= NEM Ledger =-")
  print("Sign a testnet transaction with multiple mosaics")
  print("Please confirm on your Ledger Nano S")

  send_sign_package(TESTNET, transaction)
