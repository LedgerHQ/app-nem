from pathlib import Path
import re

from ragger.backend import BackendInterface

from apps.nem import NemClient


def verify_version(version: str) -> None:
    """Verify the app version, based on defines in Makefile

    Args:
        Version (str): Version to be checked
    """

    vers_dict = {}
    vers_str = ""
    lines = _read_makefile()
    version_re = re.compile(r"^APPVERSION_(?P<part>\w)\s?=\s?(?P<val>\d*)", re.I)
    for line in lines:
        info = version_re.match(line)
        if info:
            dinfo = info.groupdict()
            vers_dict[dinfo["part"]] = dinfo["val"]
    try:
        vers_str = f"{vers_dict['M']}.{vers_dict['N']}.{vers_dict['P']}"
    except KeyError:
        pass
    assert version == vers_str


def _read_makefile() -> list[str]:
    """Read lines from the parent Makefile"""

    parent = Path(__file__).parent.parent.parent.resolve()
    makefile = f"{parent}/Makefile"
    with open(makefile, "r", encoding="utf-8") as f_p:
        lines = f_p.readlines()
    return lines


# In this test we check the behavior of the device when asked to provide the app version
def test_version(backend: BackendInterface):
    # Use the app interface instead of raw interface
    client = NemClient(backend)
    # Send the GET_VERSION instruction
    (MAJOR, MINOR, PATCH) = client.send_get_version()
    verify_version(f"{MAJOR}.{MINOR}.{PATCH}")
