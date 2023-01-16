from json import load

from apps.nem_transaction_parser import decode_txn_context
from apps.nem_transaction_builder import encode_txn_context
from utils import CORPUS_DIR, CORPUS_FILES


def test_nem_builder_parser():
    for file in CORPUS_FILES:
        print("Testing encoding / decoding on:", file)
        with open(CORPUS_DIR / file, "r") as f:
            transaction = load(f)
        transaction_bytes = encode_txn_context(transaction)
        transaction_decoded = decode_txn_context(transaction_bytes)
        assert transaction == transaction_decoded
