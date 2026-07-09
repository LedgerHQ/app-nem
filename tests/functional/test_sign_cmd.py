from json import load

import pytest
from apps.nem import MAINNET, ErrorType, NemClient
from apps.nem_transaction_builder import encode_txn_context
from Crypto.Hash import keccak as _keccak
from ragger.error import ExceptionRAPDU
from ragger.navigator.navigation_scenario import NavigateWithScenario
from utils import CORPUS_DIR, CORPUS_FILES, ROOT_SCREENSHOT_PATH

# Proposed NEM derivation paths for tests ###
NEM_PATH = "m/44'/43'/0'/0'/0'"

# Ed25519 curve constants
_P = 2**255 - 19
_Q = 2**252 + 27742317777372353535851937790883648493
_D = -121665 * pow(121666, _P - 2, _P) % _P
_I = pow(2, (_P - 1) // 4, _P)


def _keccak512(*args: bytes) -> bytes:
    h = _keccak.new(digest_bits=512)
    for a in args:
        h.update(a)
    return h.digest()


def _recover_x(y: int, sign: int):
    x2 = (y * y - 1) * pow(_D * y * y + 1, _P - 2, _P) % _P
    if x2 == 0:
        return 0 if not sign else None
    x = pow(x2, (_P + 3) // 8, _P)
    if (x * x - x2) % _P != 0:
        x = x * _I % _P
    if (x * x - x2) % _P != 0:
        return None
    if x & 1 != sign:
        x = _P - x
    return x


def _point_add(P: tuple, Q: tuple) -> tuple:
    A = (P[1] - P[0]) * (Q[1] - Q[0]) % _P
    B = (P[1] + P[0]) * (Q[1] + Q[0]) % _P
    C = 2 * P[3] * Q[3] * _D % _P
    D = 2 * P[2] * Q[2] % _P
    E, F, G, H = B - A, D - C, D + C, B + A
    return (E * F % _P, G * H % _P, F * G % _P, E * H % _P)


def _point_mul(s: int, P: tuple) -> tuple:
    Q = (0, 1, 1, 0)
    while s > 0:
        if s & 1:
            Q = _point_add(Q, P)
        P = _point_add(P, P)
        s >>= 1
    return Q


def _point_compress(P: tuple) -> bytes:
    zi = pow(P[2], _P - 2, _P)
    x = P[0] * zi % _P
    y = P[1] * zi % _P
    return (y | ((x & 1) << 255)).to_bytes(32, "little")


def _point_decompress(s: bytes) -> tuple:
    v = int.from_bytes(s, "little")
    y, sign = v & ((1 << 255) - 1), v >> 255
    x = _recover_x(y, sign)
    if x is None:
        raise ValueError("Point decompression failed")
    return (x, y, 1, x * y % _P)


# Base point of Ed25519
_G = _point_decompress(bytes.fromhex("5866666666666666666666666666666666666666666666666666666666666666"))


def verify_nem_ed25519_keccak(public_key_bytes: bytes, message: bytes, signature: bytes) -> bool:
    """Verify an Ed25519-Keccak512 signature as used by NEM mainnet/testnet."""
    if len(signature) != 64:
        return False
    R_bytes = signature[:32]
    s = int.from_bytes(signature[32:], "little")
    if s >= _Q:
        return False
    A = _point_decompress(public_key_bytes)
    R = _point_decompress(R_bytes)
    k = int.from_bytes(_keccak512(R_bytes, public_key_bytes, message), "little") % _Q
    return _point_compress(_point_mul(s, _G)) == _point_compress(_point_add(R, _point_mul(k, A)))


def load_transaction_from_file(transaction_filename: str) -> bytes:
    with open(CORPUS_DIR / transaction_filename, encoding="utf-8") as f:
        transaction = load(f)
    return encode_txn_context(transaction)


@pytest.mark.parametrize("transaction_filename", CORPUS_FILES)
def test_sign_tx_accepted(transaction_filename: str, scenario_navigator: NavigateWithScenario):
    transaction = load_transaction_from_file(transaction_filename)
    client = NemClient(scenario_navigator.backend)
    test_name = scenario_navigator.test_name + "/" + transaction_filename.replace(".json", "")
    with client.send_async_sign_message(NEM_PATH, transaction):
        scenario_navigator.review_approve(ROOT_SCREENSHOT_PATH, test_name)

    # Verify signature
    response = client.get_async_response()
    assert response is not None
    assert len(response.data) == 64  # Ed25519 signature is 64 bytes

    pub_key_response = client.send_get_public_key_non_confirm(NEM_PATH, MAINNET).data
    public_key_bytes, _ = client.parse_get_public_key_response(pub_key_response, MAINNET)
    assert verify_nem_ed25519_keccak(public_key_bytes, transaction, response.data), "Invalid signature returned by device"


def test_sign_tx_refused(scenario_navigator: NavigateWithScenario):
    transaction = load_transaction_from_file("transfer_tx.json")
    client = NemClient(scenario_navigator.backend)

    try:
        with client.send_async_sign_message(NEM_PATH, transaction):
            scenario_navigator.review_reject(ROOT_SCREENSHOT_PATH)
    except ExceptionRAPDU as e:
        assert e.status == ErrorType.SW_USER_REJECTED
