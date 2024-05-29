#!/usr/bin/env python3

import sys
import json

from pathlib import Path
from subprocess import run


NEM_LIB_DIRECTORY = (Path(__file__).parent / "../functional/apps").resolve().as_posix()
sys.path.append(NEM_LIB_DIRECTORY)
from nem_transaction_builder import encode_txn_context

CORPUS_DIR = Path(__file__).resolve().parent.parent / "corpus"
PARSER_BINARY = (Path(__file__).parent / "build/test_transaction_parser").resolve().as_posix()
TEMP_TXN_FILE = (Path(__file__).parent / "temp_txn.raw").resolve().as_posix()


TESTS_CASES = {
    "transfer_tx.json": [
        ("Transaction Type", "Transfer TX"),
        ("Recipient", "TBE56Z7MLQZ4S755JZL46VRYM7OD37SLPGFZPO5O"),
        ("Amount", "5 XEM"),
        ("Message", "ttest"),
        ("Fee", "0.1 XEM")
    ],
    "transfer_hex_message_tx.json": [
        ("Transaction Type", "Transfer TX"),
        ("Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"),
        ("Amount", "10 XEM"),
        ("Message", "0123456789abcdef"),
        ("Fee", "0.1 XEM")
    ],
    "transfer_encrypted_message_tx.json": [
        ("Transaction Type", "Transfer TX"),
        ("Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"),
        ("Amount", "0 XEM"),
        ("Message", "<encrypted msg>"),
        ("Fee", "0.2 XEM")
    ],
    "multiple_mosaic_tx.json": [
        ("Transaction Type", "Transfer TX"),
        ("Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"),
        ("Message", "Test message"),
        ("Fee", "0.15 XEM"),
        ("Mosaics", "Found 2"),
        ("Amount", "1 XEM"),
        ("Unknown Mosaic", "Divisibility and levy cannot be shown"),
        ("Namespace", "testnet: token"),
        ("Micro Units", "1")
    ],
    "multiple_mosaic_2_tx.json": [
        ("Transaction Type", "Transfer TX"),
        ("Recipient", "TA545ICAVNEUDFUBIHO3CEJBSVIZ7YYHFFX5LQPT"),
        ("Message", "Mosaics transaction"),
        ("Fee", "0.15 XEM"),
        ("Mosaics", "Found 2"),
        ("Amount", "10 XEM"),
        ("Unknown Mosaic", "Divisibility and levy cannot be shown"),
        ("Namespace", "xarleecm.zodiac: gemini"),
        ("Micro Units", "10")
    ],
    "create_namespace_tx.json": [
        ("Transaction Type", "Provision Namespace TX"),
        ("Namespace", "test_namespace_name"),
        ("Parent Name", "test_nem"),
        ("Sink Address", "TAMESPACEWH4MKFMBCVFERDPOOP4FK7MTDJEYP35"),
        ("Rental Fee", "10 XEM"),
        ("Fee", "0.15 XEM")
    ],
    "create_mosaic_tx.json": [
        ("Transaction Type", "Mosaic Definition TX"),
        ("Parent Name", "test_nem"),
        ("Mosaic Name", "mosaic_name"),
        ("Description", "mosaic description"),
        ("divisibility", "2"),
        ("initialSupply", "12"),
        ("supplyMutable", "true"),
        ("transferable", "true"),
        ("Sink Address", "TBMOSAICOD4F54EE5CDMR23CCBGOAM2XSJBR5OLC"),
        ("Rental Fee", "10 XEM"),
        ("Fee", "0.15 XEM")
    ],
    "create_mosaic_levy_tx.json": [
        ("Transaction Type", "Mosaic Definition TX"),
        ("Parent Name", "test_nem"),
        ("Mosaic Name", "mosaic_name"),
        ("Description", "mosaic description"),
        ("divisibility", "2"),
        ("initialSupply", "12"),
        ("supplyMutable", "true"),
        ("transferable", "true"),
        ("Levy Mosaic", "nem: xem"),
        ("Levy Address", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"),
        ("Levy Fee Type", "Absolute"),
        ("Levy Fee", "0.000005 micro"),
        ("Sink Address", "TBMOSAICOD4F54EE5CDMR23CCBGOAM2XSJBR5OLC"),
        ("Rental Fee", "10 XEM"),
        ("Fee", "0.15 XEM")
    ],
    "multisig_transfer_transaction_tx.json": [
        ("Transaction Type", "Multisig TX"),
        ("Multisig Fee", "0.15 XEM"),
        ("Inner TX Type", "Transfer TX"),
        ("Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"),
        ("Amount", "10 XEM"),
        ("Message", "Send a transfer transaction from a multisig account using Ledger"),
        ("Fee", "0.2 XEM")
    ],
    "multisig_create_namespace_tx.json": [
        ("Transaction Type", "Multisig TX"),
        ("Multisig Fee", "0.15 XEM"),
        ("Inner TX Type", "Provision Namespace TX"),
        ("Namespace", "test_namespace"),
        ("Create new root", "namespace"),
        ("Sink Address", "TAMESPACEWH4MKFMBCVFERDPOOP4FK7MTDJEYP35"),
        ("Rental Fee", "100 XEM"),
        ("Fee", "0.15 XEM")
    ],
    "multisig_create_mosaic_tx.json": [
        ("Transaction Type", "Multisig TX"),
        ("Multisig Fee", "0.15 XEM"),
        ("Inner TX Type", "Mosaic Definition TX"),
        ("Parent Name", "test_nem"),
        ("Mosaic Name", "mosaic_name"),
        ("Description", "mosaic description"),
        ("divisibility", "2"),
        ("initialSupply", "12"),
        ("supplyMutable", "true"),
        ("transferable", "true"),
        ("Sink Address", "TBMOSAICOD4F54EE5CDMR23CCBGOAM2XSJBR5OLC"),
        ("Rental Fee", "10 XEM"),
        ("Fee", "0.15 XEM"),
    ],
    "multisig_create_mosaic_levy_tx.json": [
        ("Transaction Type", "Multisig TX"),
        ("Multisig Fee", "0.15 XEM"),
        ("Inner TX Type", "Mosaic Definition TX"),
        ("Parent Name", "test_nem"),
        ("Mosaic Name", "mosaic_create_from_ledger"),
        ("Description", "This mosaic is created by a ledger wallet from a multisig account"),
        ("divisibility", "3"),
        ("initialSupply", "1000"),
        ("supplyMutable", "true"),
        ("transferable", "true"),
        ("Levy Mosaic", "nem: xem"),
        ("Levy Address", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"),
        ("Levy Fee Type", "Absolute"),
        ("Levy Fee", "0.000005 micro"),
        ("Sink Address", "TBMOSAICOD4F54EE5CDMR23CCBGOAM2XSJBR5OLC"),
        ("Rental Fee", "10 XEM"),
        ("Fee", "0.15 XEM")
    ],
    "multisig_signature_transfer_transaction.json": [
        ("Transaction Type", "Multi Sig. TX"),
        ("SHA3 Tx Hash", "9298E6A7255F269D88BA5D096341B3C56C3E65A511B72A947097C0CFAB95D4B1"),
        ("Multisig Address", "TA6DD3TAAW7DIOFJKWHNJJZQLTSRWAQ67YKWYQBG"),
        ("Multisig Fee", "0.15 XEM"),
        ("Detail TX Type", "Transfer TX"),
        ("Recipient", "TB7IB6DSJKWBVQEK7PD7TWO66ECW5LY6SISM2CJJ"),
        ("Amount", "0.4 XEM"),
        ("Message", "test message"),
        ("Fee", "0.1 XEM")
    ],
    "multisig_signature_provision_namespace_transaction.json": [
        ("Transaction Type", "Multi Sig. TX"),
        ("SHA3 Tx Hash", "39D64C1602FA48D49C59A464EF109235472134EA854D1B70284EA32AEC6DD17A"),
        ("Multisig Address", "TA6DD3TAAW7DIOFJKWHNJJZQLTSRWAQ67YKWYQBG"),
        ("Multisig Fee", "0.15 XEM"),
        ("Detail TX Type", "Provision Namespace TX"),
        ("Namespace", "ledger_ns"),
        ("Create new root", "namespace"),
        ("Sink Address", "TAMESPACEWH4MKFMBCVFERDPOOP4FK7MTDJEYP35"),
        ("Rental Fee", "100 XEM"),
        ("Fee", "0.15 XEM"),
    ]
}


def assert_equal(a, b, text):
    if a != b:
        print(f"[  ERROR   ] Mismatch in {text}: <{a}> vs <{b}>")
        return False
    return True


def test_parsing(filename, expected):
    print("[ RUN      ] ", filename)
    with open(CORPUS_DIR / filename) as f:
        transaction = json.load(f)

    tx_data = encode_txn_context(transaction)

    with open(TEMP_TXN_FILE, 'wb') as f:
        f.write(tx_data)

    cmd = [PARSER_BINARY, TEMP_TXN_FILE]
    res = run(cmd, capture_output=True)
    status = res.returncode

    if status != 0:
        print("[  ERROR   ] ", res.stderr)
    else:
        parsed = res.stdout.decode().strip()
        received = [pair.split("::") for pair in parsed.split("\n")]
        if not assert_equal(len(received), len(expected), "number of fields"):
            status = 1
        else:
            for i in range(len(received)):
                if not assert_equal(received[i][0], expected[i][0], "name of field"):
                    status = 1
                    break
                if not assert_equal(received[i][1], expected[i][1], "value of field"):
                    status = 1
                    break

    if status != 0:
        print("[  FAILED  ] ", filename)
    else:
        print("[       OK ] ", filename)
    return status


status = 0
for filename, expected in TESTS_CASES.items():
    res = test_parsing(filename, expected)
    if res != 0:
        status = res

exit(status)
