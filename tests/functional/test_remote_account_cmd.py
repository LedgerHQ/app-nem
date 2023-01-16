from ragger.backend import SpeculosBackend
from ragger.backend.interface import RaisePolicy
from ragger.navigator import NavInsID, NavIns

from apps.nem import NemClient, ErrorType
from utils import ROOT_SCREENSHOT_PATH

# Proposed NEM derivation paths for tests ###
NEM_PATH = "m/44'/43'/0'/0'/0'"

SPECULOS_EXPECTED_DELEGATED_HARVESTING_KEY = "328a20f8900fd5e2fc0f9f02d949feb6"\
                                             "6a8a448f8822c914b22aba9bcddaa46f"


def check_get_remote_account_resp(backend, delegated_harvesting_key):
    if isinstance(backend, SpeculosBackend):
        # Check against nominal Speculos seed expected results
        assert delegated_harvesting_key.hex() == SPECULOS_EXPECTED_DELEGATED_HARVESTING_KEY


def test_get_remote_account_non_confirm(backend):
    client = NemClient(backend)
    response = client.send_get_remote_account_non_confirm(NEM_PATH).data
    delegated_harvesting_key = client.parse_get_remote_account_response(response)
    check_get_remote_account_resp(backend, delegated_harvesting_key)


def test_get_remote_account_confirm_accepted(backend, navigator, test_name):
    client = NemClient(backend)
    with client.send_async_get_remote_account_confirm(NEM_PATH):
        navigator.navigate_until_text_and_compare(NavIns(NavInsID.RIGHT_CLICK),
                                                  [NavIns(NavInsID.BOTH_CLICK)],
                                                  "Approve",
                                                  ROOT_SCREENSHOT_PATH,
                                                  test_name)
    response = client.get_async_response().data
    delegated_harvesting_key = client.parse_get_remote_account_response(response)
    check_get_remote_account_resp(backend, delegated_harvesting_key)


def test_get_remote_account_confirm_refused(backend, navigator, test_name):
    client = NemClient(backend)
    with client.send_async_get_remote_account_confirm(NEM_PATH):
        backend.raise_policy = RaisePolicy.RAISE_NOTHING
        navigator.navigate_until_text_and_compare(NavIns(NavInsID.RIGHT_CLICK),
                                                  [NavIns(NavInsID.BOTH_CLICK)],
                                                  "Reject",
                                                  ROOT_SCREENSHOT_PATH,
                                                  test_name)
    rapdu = client.get_async_response()
    assert rapdu.status == ErrorType.SW_USER_REJECTED
    assert len(rapdu.data) == 0
