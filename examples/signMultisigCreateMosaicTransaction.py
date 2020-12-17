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
from base import send_hex

parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP32 path to retrieve.")
parser.add_argument('--ed25519', help="Derive on ed25519 curve", action='store_true')
parser.add_argument("--apdu", help="Display APDU log", action='store_true')
args = parser.parse_args()

FIRST_TEST_TX =  "E0048080FF058000002C8000002B8000009880000000800000000410000001000098100F690A200000009F96DF7E7A639B4034B8BEE5B88AB1D640DB66EB5A47AFE018E320CB130C183DF049020000000000201D690A420100000140000001000098100F690A20000000180158D9FEED1711FBFC7718ED144275311DCFD10A4480035D1856CDAC7242ABF049020000000000201D690ACE00000020000000180158D9FEED1711FBFC7718ED144275311DCFD10A4480035D1856CDAC7242AB1B00000008000000746573745F6E656D0B0000006D6F736169635F6E616D65120000006D6F73616963206465736372697074696F6E04000000150000000C0000006469766973"
MORE_TEST_TX = "E0040180986962696C6974790100000032170000000D000000696E697469616C537570706C79020000003132190000000D000000737570706C794D757461626C650400000074727565180000000C0000007472616E7366657261626C650400000074727565000000002800000054424D4F534149434F443446353445453543444D523233434342474F414D3258534A4252354F4C438096980000000000"

print("-= NEM Ledger =-")
print("Sign a multisig create mosaic transaction")
print("Please confirm on your Ledger Nano S")

result1 = send_hex(FIRST_TEST_TX)
result2 = send_hex(MORE_TEST_TX)
