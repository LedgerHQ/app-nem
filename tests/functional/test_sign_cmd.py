from json import load
import pytest

from ragger.backend.interface import RaisePolicy
from ragger.navigator import NavInsID, NavIns
from ragger.error import ExceptionRAPDU

from apps.nem import NemClient, ErrorType
from apps.nem_transaction_builder import encode_txn_context
from utils import ROOT_SCREENSHOT_PATH, CORPUS_DIR, CORPUS_FILES

# Proposed NEM derivation paths for tests ###
NEM_PATH = "m/44'/43'/0'/0'/0'"


def load_transaction_from_file(transaction_filename):
    with open(CORPUS_DIR / transaction_filename, encoding="utf-8") as f:
        transaction = load(f)
    return encode_txn_context(transaction)


def check_transaction(test_name, firmware, backend, navigator, transaction_filename, scenario_navigator):
    transaction = load_transaction_from_file(transaction_filename)
    nem = NemClient(backend)
    with nem.send_async_sign_message(NEM_PATH, transaction):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK,
                                                       NavIns(NavInsID.WAIT, ([0]))],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            scenario_navigator.review_approve(ROOT_SCREENSHOT_PATH, test_name)
    # Missing signature verification


@pytest.mark.parametrize("transaction_filename", CORPUS_FILES)
def test_sign_tx_accepted(test_name, firmware, backend, navigator, transaction_filename, scenario_navigator):
    folder_name = test_name + "/" + transaction_filename.replace(".json", "")
    check_transaction(folder_name, firmware, backend, navigator, transaction_filename, scenario_navigator)


def test_sign_tx_refused(test_name, firmware, backend, navigator, scenario_navigator):
    transaction = load_transaction_from_file("transfer_tx.json")
    client = NemClient(backend)

    if firmware.device.startswith("nano"):
        with client.send_async_sign_message(NEM_PATH, transaction):
            backend.raise_policy = RaisePolicy.RAISE_NOTHING
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Reject",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        rapdu = client.get_async_response()
        assert rapdu.status == ErrorType.SW_USER_REJECTED
        assert len(rapdu.data) == 0
    else:

        try:
            with client.send_async_sign_message(NEM_PATH, transaction):
                scenario_navigator.review_reject(ROOT_SCREENSHOT_PATH)
        except ExceptionRAPDU as e:
            assert e.status == ErrorType.SW_USER_REJECTED
