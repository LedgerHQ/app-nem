from ragger.backend import SpeculosBackend
from ragger.backend.interface import RaisePolicy
from ragger.navigator import NavInsID, NavIns

from apps.nem import NemClient, ErrorType
from utils import ROOT_SCREENSHOT_PATH

# Proposed NEM derivation paths for tests ###
NEM_PATH = "m/44'/43'/0'/0'/0'"

SPECULOS_EXPECTED_PUBLIC_KEY = "8e494cb179a5acef773c1bbf83ad2cd7"\
                               "97673149e3ccdb7df739a1f5deb2fad0"


def check_get_public_key_resp(backend, public_key):
    if isinstance(backend, SpeculosBackend):
        # Check against nominal Speculos seed expected results
        assert public_key.hex() == SPECULOS_EXPECTED_PUBLIC_KEY


def test_get_public_key_non_confirm(backend):
    client = NemClient(backend)
    response = client.send_get_public_key_non_confirm(NEM_PATH).data
    public_key, address = client.parse_get_public_key_response(response)
    check_get_public_key_resp(backend, public_key)


def test_get_public_key_confirm_accepted(firmware, backend, navigator, test_name):
    client = NemClient(backend)
    with client.send_async_get_public_key_confirm(NEM_PATH):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavIns(NavInsID.RIGHT_CLICK),
                                                      [NavIns(NavInsID.BOTH_CLICK)],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            instructions = [
                NavIns(NavInsID.USE_CASE_REVIEW_TAP),
                NavIns(NavInsID.TOUCH, (200, 335)),
                NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR),
                NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM),
                NavIns(NavInsID.USE_CASE_STATUS_WAIT)
            ]
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name,
                                           instructions)
    response = client.get_async_response().data
    public_key, address = client.parse_get_public_key_response(response)
    check_get_public_key_resp(backend, public_key)


# In this test we check that the GET_PUBLIC_KEY in confirmation mode replies an error if the user refuses
def test_get_public_key_confirm_refused(firmware, backend, navigator, test_name):
    client = NemClient(backend)

    if firmware.device.startswith("nano"):
        with client.send_async_get_public_key_confirm(NEM_PATH):
            backend.raise_policy = RaisePolicy.RAISE_NOTHING
            navigator.navigate_until_text_and_compare(NavIns(NavInsID.RIGHT_CLICK),
                                                      [NavIns(NavInsID.BOTH_CLICK)],
                                                      "Reject",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        rapdu = client.get_async_response()
        assert rapdu.status == ErrorType.SW_USER_REJECTED
        assert len(rapdu.data) == 0
    else:
        instructions_set = [
            [
                NavIns(NavInsID.USE_CASE_REVIEW_REJECT),
                NavIns(NavInsID.USE_CASE_STATUS_WAIT)
            ],
            [
                NavIns(NavInsID.USE_CASE_REVIEW_TAP),
                NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CANCEL),
                NavIns(NavInsID.USE_CASE_STATUS_WAIT)
            ]
        ]
        for i, instructions in enumerate(instructions_set):
            with client.send_async_get_public_key_confirm(NEM_PATH):
                backend.raise_policy = RaisePolicy.RAISE_NOTHING
                navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                               test_name + f"/part{i}",
                                               instructions)
            rapdu = client.get_async_response()
            assert rapdu.status == ErrorType.SW_USER_REJECTED
            assert len(rapdu.data) == 0
