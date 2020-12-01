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

FIRST_TEST_TX =  "E0048080FF058000002C8000002B8000009880000000800000000140000001000098D1EFA108200000003E6E6CBAC488B8A44BDF5ABF27B9E1CC2A6F20D09D550A66B9B36F525CA222EEF049020000000000E1FDA108D3000000200000003E6E6CBAC488B8A44BDF5ABF27B9E1CC2A6F20D09D550A66B9B36F525CA222EE21000000150000006C6F6E676C65653030312E6C6F6E676C656530303304000000746573740E000000746869732069732061207465737404000000150000000C00000064697669736962696C69747901000000331A0000000D000000696E697469616C557570706C79050000003132333435190000000D000000737570706C794D757461626C"
MORE_TEST_TX =  "E00401805D650400000074727565180000000C0000007472616E7366657261626C650400000074727565000000002800000054424D4F534149434F443446353445453543444D523233434342474F414D3258534A4252354F4C438096980000000000"

result1 = send_hex(FIRST_TEST_TX)
result2 = send_hex(MORE_TEST_TX)
