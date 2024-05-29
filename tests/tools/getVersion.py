#!/usr/bin/env python3

import sys

from pathlib import Path

from ragger.backend import LedgerCommBackend

NEM_LIB_DIRECTORY = (Path(__file__).parent / "../functional/apps").resolve().as_posix()
sys.path.append(NEM_LIB_DIRECTORY)
from nem import NemClient


def main():
    with LedgerCommBackend(None, interface="hid") as backend:
        zilliqa = NemClient(backend)
        version = zilliqa.send_get_version()
        print("v{}.{}.{}".format(version[0], version[1], version[2]))


if __name__ == "__main__":
    main()
