#!/usr/bin/env python3

import sys
import argparse

from pathlib import Path

from ragger.backend import LedgerCommBackend

NEM_LIB_DIRECTORY = (Path(__file__).parent / "../functional/apps").resolve().as_posix()
sys.path.append(NEM_LIB_DIRECTORY)
from nem import NemClient


parser = argparse.ArgumentParser()
parser.add_argument('--path', help="BIP 32 path to use")
parser.add_argument('--confirm', help="Request confirmation", action="store_true")
args = parser.parse_args()

if args.path is None:
    # Use testnet coin type
    args.path = "m/44'/1'/0'/0'/0'"


with LedgerCommBackend(None, interface="hid") as backend:
    nem = NemClient(backend)

    if args.confirm:
        with nem.send_async_get_remote_account_confirm(args.path):
            print("Please accept the request on the device")
        rapdu = nem.get_async_response()
    else:
        rapdu = nem.send_get_remote_account_non_confirm(args.path)

    delegated_harvesting_key = nem.parse_get_remote_account_response(rapdu.data)
    print("Delegated harvesting:", delegated_harvesting_key.hex())
    print("length: ", len(delegated_harvesting_key.hex()))
