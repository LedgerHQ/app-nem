from contextlib import contextmanager
from enum import IntEnum
from typing import Generator
from struct import pack
from base64 import b32encode

from bip_utils.utils.crypto.ripemd import Ripemd160
from bip_utils.utils.crypto.sha3 import Kekkak256

from ragger.backend.interface import BackendInterface, RAPDU
from ragger.utils import split_message
from ragger.bip import pack_derivation_path


TESTNET = 152
MAINNET = 104
MIJIN_MAINNET = 96
MIJIN_TESTNET = 144


class INS(IntEnum):
    INS_GET_PUBLIC_KEY = 0x02
    INS_SIGN = 0x04
    INS_GET_REMOTE_ACCOUNT = 0x05
    INS_GET_APP_CONFIGURATION = 0x06


CLA = 0xE0

P1_CONFIRM = 0x01
P1_NON_CONFIRM = 0x00
P2_NO_CHAINCODE = 0x00
P2_CHAINCODE = 0x01
P1_MASK_ORDER = 0x01
P1_MASK_MORE = 0x80
P2_SECP256K1 = 0x40
P2_ED25519 = 0x80

STATUS_OK = 0x9000

MAX_CHUNK_SIZE = 255


class ErrorType:
    SW_WRONG_LENGTH = 0x6700
    SW_USER_REJECTED = 0x6985
    SW_INVALID_DATA = 0x6A80
    SW_INVALID_P1P2 = 0x6B00
    SW_INS_NOT_SUPPORTED = 0x6D00
    SW_CLA_NOT_SUPPORTED = 0x6E00


class NemClient:
    def __init__(self, backend: BackendInterface):
        self._backend = backend

    def send_get_version(self) -> (int, int, int):
        rapdu: RAPDU = self._backend.exchange(CLA, INS.INS_GET_APP_CONFIGURATION, 0, 0, b"")
        response = rapdu.data
        # response = 0x00 (1) ||
        #            LEDGER_MAJOR_VERSION (1) ||
        #            LEDGER_MINOR_VERSION (1) ||
        #            LEDGER_PATCH_VERSION (1)
        assert len(response) == 4
        assert int(response[0]) == 0
        major = int(response[1])
        minor = int(response[2])
        patch = int(response[3])
        return (major, minor, patch)

    def compute_adress_from_public_key(self, public_key: bytes, network_type: int = MAINNET) -> str:
        assert network_type in [TESTNET, MAINNET]
        buffer1 = Kekkak256.QuickDigest(public_key)
        rawAddress = pack("<B", network_type) + Ripemd160.QuickDigest(buffer1)[:20]
        rawAddress += Kekkak256.QuickDigest(rawAddress)[:4]

        buff = b32encode(rawAddress).decode("utf-8")
        return buff

    def parse_get_public_key_response(self, response: bytes, network_type: int = MAINNET) -> (bytes, str, bytes):
        # response = address_len (1) ||
        #            address (40) ||
        #            public_key_len (1) ||
        #            public_key (32)
        assert len(response) == 1 + 40 + 1 + 32
        assert response[0] == 40
        address: str = response[1: 1 + 40].decode("utf-8")
        assert response[41] == 32
        public_key: bytes = response[42:]

        assert self.compute_adress_from_public_key(public_key, network_type) == address
        return public_key, address

    def send_get_public_key_non_confirm(self, derivation_path: str,
                                        network_type: int = MAINNET) -> RAPDU:
        p1 = P1_NON_CONFIRM
        p2 = 0  # Unused
        payload = pack_derivation_path(derivation_path) + pack("<B", network_type)
        return self._backend.exchange(CLA, INS.INS_GET_PUBLIC_KEY,
                                      p1, p2, payload)

    @contextmanager
    def send_async_get_public_key_confirm(self, derivation_path: str,
                                          network_type: int = MAINNET) -> RAPDU:
        p1 = P1_CONFIRM
        p2 = 0  # Unused
        payload = pack_derivation_path(derivation_path) + pack("<B", network_type)
        with self._backend.exchange_async(CLA, INS.INS_GET_PUBLIC_KEY,
                                          p1, p2, payload):
            yield

    def parse_get_remote_account_response(self, response: bytes, network_type: int = MAINNET) -> (bytes, str, bytes):
        # response = delegated_harvesting_key_len (1) ||
        #            delegated_harvesting_key (32)
        assert len(response) == 1 + 32
        assert response[0] == 32
        delegated_harvesting_key: bytes = response[1:]
        return delegated_harvesting_key

    def send_get_remote_account_non_confirm(self, derivation_path: str) -> RAPDU:
        p1 = P1_NON_CONFIRM
        p2 = 0  # Unused
        payload = pack_derivation_path(derivation_path)
        return self._backend.exchange(CLA, INS.INS_GET_REMOTE_ACCOUNT,
                                      p1, p2, payload)

    @contextmanager
    def send_async_get_remote_account_confirm(self, derivation_path: str) -> RAPDU:
        p1 = P1_CONFIRM
        p2 = 0  # Unused
        payload = pack_derivation_path(derivation_path)
        with self._backend.exchange_async(CLA, INS.INS_GET_REMOTE_ACCOUNT,
                                          p1, p2, payload):
            yield

    def _send_sign_message(self, message: bytes, first: bool, last: bool) -> RAPDU:
        p1 = 0
        if not first:
            p1 |= P1_MASK_ORDER
        if not last:
            p1 |= P1_MASK_MORE
        return self._backend.exchange(CLA, INS.INS_SIGN, p1, 0, message)

    @contextmanager
    def _send_async_sign_message(self, message: bytes,
                                 first: bool, last: bool) -> Generator[None, None, None]:
        p1 = 0
        if not first:
            p1 |= P1_MASK_ORDER
        if not last:
            p1 |= P1_MASK_MORE
        with self._backend.exchange_async(CLA, INS.INS_SIGN, p1, 0, message):
            yield

    def send_async_sign_message(self,
                                derivation_path: str,
                                message: bytes) -> Generator[None, None, None]:
        messages = split_message(pack_derivation_path(derivation_path) + message, MAX_CHUNK_SIZE)
        first = True

        if len(messages) > 1:
            self._send_sign_message(messages[0], True, False)
            for m in messages[1:-1]:
                self._send_sign_message(m, False, False)
            first = False

        return self._send_async_sign_message(messages[-1], first, True)

    def get_async_response(self) -> RAPDU:
        return self._backend.last_async_response
