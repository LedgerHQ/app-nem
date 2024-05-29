#!/usr/bin/env python3

import sys
import json
import argparse

from pathlib import Path

from ragger.backend import LedgerCommBackend


NEM_LIB_DIRECTORY = (Path(__file__).parent / "../functional/apps").resolve().as_posix()
sys.path.append(NEM_LIB_DIRECTORY)
from nem import NemClient
from nem_transaction_builder import encode_txn_context

parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP 32 path to use")
parser.add_argument('--file', help="Transaction in JSON format")
args = parser.parse_args()

if args.path is None:
    # Use testnet coin type
    args.path = "m/44'/1'/0'/0'/0'"

if args.file is None:
    args.file = '../corpus/transfer_tx.json'

with open(args.file) as f:
    obj = json.load(f)
message = encode_txn_context(obj)

with LedgerCommBackend(None, interface="hid") as backend:
    nem = NemClient(backend)

    with nem.send_async_sign_message(args.path, message):
        print("Please accept the request on the device")

    rapdu = nem.get_async_response()
    print("Status:", hex(rapdu.status))
    print("Data:", rapdu.data.hex())

    assert rapdu.status == 0x9000
