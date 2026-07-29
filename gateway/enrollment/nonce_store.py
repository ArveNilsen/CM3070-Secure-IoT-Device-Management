import secrets
import time
from dataclasses import dataclass, field
from threading import Lock

NONCE_TTL_SECONDS = 60

@dataclass
class PendingEnrollment:
    public_key_id: str
    nonce: bytes
    issued_at: float = field(default_factory=time.monotonic)

    def is_expired(self) -> bool:
        return (time.monotonic() - self.issued_at > NONCE_TTL_SECONDS)


class NonceStore:
    """
    Tracks pending enrollment nonces. One nonce per public_key_id at a time,
    a second request invalidates the first.
    """
    def __init__(self):
        self._pending: dict[str, PendingEnrollment] = {}
        self._lock = Lock()

    def issue(self, public_key_id: str) -> bytes:
        nonce = secrets.token_bytes(32)
        with self._lock:
            self._pending[public_key_id] = PendingEnrollment(
                public_key_id=public_key_id,
                nonce=nonce)
        return nonce

    def consume(self,
                public_key_id: str,
                presented_nonce: bytes
                ) -> bool:
        """
        Validate and consume a nonce. Returns False if not found, expired or
        mismatched. Consuming removes it, nonces are single-use.
        """
        with self._lock:
            pending = self._pending.get(public_key_id)
            if pending is None:
                return False
            if pending.is_expired():
                del self._pending[public_key_id]
                return False
            if not secrets.compare_digest(pending.nonce, presented_nonce):
                return False
            del self._pending[public_key_id]
            return True

