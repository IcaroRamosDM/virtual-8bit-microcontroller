"""Cache checksum-verified build inputs without retaining partial downloads."""

import hashlib
from pathlib import Path
import tempfile
import urllib.request


def verify_checksum(path, expected):
    with path.open("rb") as stream:
        actual = hashlib.file_digest(stream, "sha256").hexdigest()
    if actual != expected:
        raise RuntimeError(f"Checksum mismatch: {path}. Nothing was extracted or executed.")


def download_verified(url, path, digest):
    if path.exists():
        verify_checksum(path, digest)
        return

    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(
            prefix=f".{path.name}.", dir=path.parent, delete=False
        ) as stream:
            temporary = Path(stream.name)
        print(f"Downloading {path.name} from the official upstream", flush=True)
        urllib.request.urlretrieve(url, temporary)
        verify_checksum(temporary, digest)
        temporary.replace(path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
