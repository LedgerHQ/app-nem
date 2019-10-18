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

# transfer
# TEST_TX =  "0190544140420F0000000000860919C71900000090A5744E3BF0C584C6B2691B8E86803DE4BFA09EA2C98A6222050002007465737469BBF1532237D3044086A40800000000D787D9329996A177809FD50000000000".decode('hex')  
# create namespace
# TEST_TX =  "01904E4180841E0000000000AB3C94DB1900000000E803000000000000458B3708FB131EF40A74686973697361796573".decode('hex')  
# Create mosaic
TEST_TX =  "17FA4747F5014B50413CCF968749604D728D7065DC504291EEE556899A534CBB0190414120A107000000000063EDBD0A1A000000770000003E0000005764E905964D430176E98ADC870DFC6130C831223162BD3228B8E744C751C25D01904D41F574FF6CF9712D0AC621217305000000000000000000390000005764E905964D430176E98ADC870DFC6130C831223162BD3228B8E744C751C25D01904D42F9712D0AC62121730187D6120000000000".decode('hex')  
signatureForTestTx = "c05bc625da78bb71b0860891860211329870108f2299ccfa334fe67e03080ef4f9c1a994fbdc9c013d041f43ac13f53f3900f26e4b45be95a202a2a29318560d"

donglePath = parse_bip32_path(args.path)
print("-= NEM Ledger =-")
print("Sign a testnet transaction")
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
if sig[0:128] == signatureForTestTx:
  print "Ki dung"
else :
  print "Ki sai"