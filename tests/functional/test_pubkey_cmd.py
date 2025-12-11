from ragger.backend import BackendInterface, SpeculosBackend
from ragger.navigator.navigation_scenario import NavigateWithScenario
from ragger.error import ExceptionRAPDU

from apps.nem import NemClient, ErrorType
from utils import ROOT_SCREENSHOT_PATH

# Proposed NEM derivation paths for tests ###
NEM_PATH = "m/44'/43'/0'/0'/0'"

SPECULOS_EXPECTED_PUBLIC_KEY = "8e494cb179a5acef773c1bbf83ad2cd7"\
                               "97673149e3ccdb7df739a1f5deb2fad0"


def check_get_public_key_resp(backend: BackendInterface, public_key: bytes):
    if isinstance(backend, SpeculosBackend):
        # Check against nominal Speculos seed expected results
        assert public_key.hex() == SPECULOS_EXPECTED_PUBLIC_KEY


def test_get_public_key_non_confirm(backend: BackendInterface):
    client = NemClient(backend)
    response = client.send_get_public_key_non_confirm(NEM_PATH).data
    public_key, _ = client.parse_get_public_key_response(response)
    check_get_public_key_resp(backend, public_key)


def test_get_public_key_confirm_accepted(scenario_navigator: NavigateWithScenario):
    client = NemClient(scenario_navigator.backend)
    with client.send_async_get_public_key_confirm(NEM_PATH):
        scenario_navigator.address_review_approve(ROOT_SCREENSHOT_PATH)
    response = client.get_async_response()
    assert response is not None
    public_key, _ = client.parse_get_public_key_response(response.data)
    check_get_public_key_resp(scenario_navigator.backend, public_key)


# In this test we check that the GET_PUBLIC_KEY in confirmation mode replies an error if the user refuses
def test_get_public_key_confirm_refused(scenario_navigator: NavigateWithScenario):
    client = NemClient(scenario_navigator.backend)

    try:
        with client.send_async_get_public_key_confirm(NEM_PATH):
            scenario_navigator.address_review_reject(ROOT_SCREENSHOT_PATH)
    except ExceptionRAPDU as e:
        assert e.status == ErrorType.SW_USER_REJECTED
