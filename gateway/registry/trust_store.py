"""
Pre-enrollment trust store: records whick public keys the gateway is willing to
accept attestation attempts from.

Populated by register_device.py
"""
from __future__ import annotations

import sqlite3
from pathlib import Path


class TrustStore:
    def __init__(self, db_path: str = "trust_store.db"):
        self._path = Path(db_path)
        self._conn = sqlite3.connect(str(self._path), check_same_thread=False)
        self._conn.row_factory = sqlite3.Row
        self._init_schema()


    def _init_schema(self) -> None:
        self._conn.executescript("""
            CREATE TABLE IF NOT EXISTS registered_devices (
                public_key_id       TEXT PRIMARY KEY,
                device_class        TEXT NOT NULL,
                public_key_hex      TEXT NOT NULL,
                firmware_hash_hex   TEXT NOT NULL,
                registered_at       TEXT NOT NULL
            );
        """)
        self._conn.commit()


    def is_registered(self, public_key_id: str) -> bool:
        row = self._conn.execute(
            "SELECT 1 FROM registered_devices WHERE public_key_id = ?",
            (public_key_id,)
        ).fetchone()
        return row is not None


    def get(self, public_key_id: str) -> sqlite3.Row | None:
        return self._conn.execute(
            "SELECT * FROM registered_devices WHERE public_key_id = ?",
            (public_key_id,)
        ).fetchone()


    def register(self, public_key_id: str, device_class: str,
                 public_key_hex: str, firmware_hash_hex: str) -> None:
        self._conn.execute("""
            INSERT INTO registered_devices
                (public_key_id, device_class, public_key_hex,
                firmware_hash_hex, registered_at)
            VALUES (?, ?, ?, ?, datetime('now'))
        """, (public_key_id, device_class, public_key_hex, firmware_hash_hex))
        self._conn.commit()


    def close(self) -> None:
        self._conn.close()

