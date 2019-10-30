#!/usr/bin/env python

from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException
import argparse
from base import parse_bip32_path

parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP32 path to retrieve.")
parser.add_argument('--ed25519', help="Derive on ed25519 curve", action='store_true')
parser.add_argument("--apdu", help="Display APDU log", action='store_true')
args = parser.parse_args()

if args.path == None:
  args.path = "44'/43'/152'/0'/0'"

FIRST_TEST_TX =  "E0040080FF058000002C8000002B8000009880000000800000000140000001000098D1EFA108200000003E6E6CBAC488B8A44BDF5ABF27B9E1CC2A6F20D09D550A66B9B36F525CA222EEF049020000000000E1FDA108D3000000200000003E6E6CBAC488B8A44BDF5ABF27B9E1CC2A6F20D09D550A66B9B36F525CA222EE21000000150000006C6F6E676C65653030312E6C6F6E676C656530303304000000746573740E000000746869732069732061207465737404000000150000000C00000064697669736962696C69747901000000331A0000000D000000696E697469616C557570706C79050000003132333435190000000D000000737570706C794D757461626C".decode('hex')  
MORE_TEST_TX =  "E00480805D650400000074727565180000000C0000007472616E7366657261626C650400000074727565000000002800000054424D4F534149434F443446353445453543444D523233434342474F414D3258534A4252354F4C438096980000000000".decode('hex')  

donglePath = parse_bip32_path(args.path)
dongle = getDongle(True)
print("-= NEM Ledger =-")
print("Sign a mosaic creation")
print "Please confirm on your Ledger Nano S"
result1 = dongle.exchange(FIRST_TEST_TX)
result2 = dongle.exchange(MORE_TEST_TX)
sig = str(result2).encode('hex')
print "signDatas:\t", str(FIRST_TEST_TX + MORE_TEST_TX).encode('hex')
print "signature:\t", sig[0:128]
print "publicKey:\t", sig[130:130+64]
print "bip32Path:\t", args.path