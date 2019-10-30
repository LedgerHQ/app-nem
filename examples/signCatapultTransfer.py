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
  args.path = "44'/43'/144'/0'/0'"

TEST_TX =  "17FA4747F5014B50413CCF968749604D728D7065DC504291EEE556899A534CBB0190544140420F0000000000C65AFE4C1A00000090FC443D62754C19452DC196C3C8CDC86782F36565BEC9A41D0F0001005468697320697320612074657374C47BCD9047148F30C0D4540700000000".decode('hex')  

donglePath = parse_bip32_path(args.path)
print("-= NEM Ledger =-")
print("Sign a catapult transfer transaction")
print "Please confirm on your Ledger Nano S"
apdu = "e0" + "04" + "90" + "80"
apdu = apdu.decode('hex') + chr(len(donglePath) + 1 + len(TEST_TX)) + chr(len(donglePath) / 4) + donglePath + TEST_TX
dongle = getDongle(args.apdu)
result = dongle.exchange(bytes(apdu))
sig = str(result).encode('hex')
print "signDatas:\t", str(TEST_TX).encode('hex')
print "signature:\t", sig[0:128]
print "publicKey:\t", sig[130:130+64]
print "bip32Path:\t", args.path