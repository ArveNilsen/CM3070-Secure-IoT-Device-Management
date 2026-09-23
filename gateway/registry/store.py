from __future__ import annotations

import sqlite3
import time
from dataclasses import dataclass
from pathlib import Path

# ---
# Data model
# ---

@dataclass(frozen=True)
class DeviceRecord:
    """
    Represents one enrolled device. Maps onto the enrolled Device in
    Alloy SystemState. One record per device, immutable after creation.
    """
    public_key_id:      str
    device_class:       str
    capabilities:       int # bitmask
    active_caps:        int
    manifest_version:   int
    firmware_hash:      str
    enrolled_at:        float
    state:              str # enrolled | restricted | quarantied | revoked
    mac_address:        str | None = None

    def is_active(self) -> bool:
        return self.state in ("enrolled", "restricted")


# ---
# Registry
# ---

class DeviceRegistry:
    """
    SQLite-backed device registry.

    Invariants from Alloy model:
    - One record per public_key_id
    - active_caps is a subset of capabilities
    - No unenrolled devices holds capabilities
    """

    def __init__(self, db_path: str = "registry.db"):
        self._path = Path(db_path)
        self._conn = sqlite3.connect(str(self._path), check_same_thread=False)
        self._conn.row_factory = sqlite3.Row
        self._init_schema()


# ---
# Schema
# ---

    def _init_schema(self) -> None:
        self._conn.executescript("""
            CREATE TABLE IF NOT EXISTS devices (
                public_key_id   TEXT PRIMARY KEY,
                device_class    TEXT NOT NULL,
                capabilities    INTEGER NOT NULL,
                active_caps     INTEGER NOT NULL,
                manifest_version INTEGER NOT NULL,
                firmware_hash   TEXT NOT NULL,
                enrolled_at     REAL NOT NULL,
                state           TEXT NOT NULL
                    DEFAULT 'enrolled'
                    CHECK(state IN (
                        'enrolled',
                        'restricted',
                        'quarantined',
                        'revoked')),
                mac_address     TEXT
            );

            CREATE TABLE IF NOT EXISTS audit_log (
                id              INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp       REAL NOT NULL,
                public_key_id   TEXT NOT NULL,
                event           TEXT NOT NULL,
                detail          TEXT
            );
        """)
        self._conn.commit()


# ---
# Enrollment
# ---

    def is_enrolled(self, public_key_id: str) -> bool:
        """
        Returns True if a non-revoked record exists for this device.
        Alloy: d in s.enrolled
        """
        row = self._conn.execute(
            """SELECT state FROM devices
               WHERE public_key_id = ?""",
            (public_key_id,)
        ).fetchone()

        if row is None:
            return False
        # Revoked devices are not enrolled
        return row["state"] != "revoked"


    def enroll(self,
               public_key_id:    str,
               device_class:     str,
               capabilities:     int,
               manifest_version: int,
               firmware_hash:    str,
               mac_address: str | None = None) -> None:
        """
        Record a successful enrollment.

        Enforces UniqueManifestPerDevice:
        raises if device already enrolled. Sets active_caps = capabilities.
        """
        if self.is_enrolled(public_key_id):
            raise ValueError(f"Device {public_key_id} already enrolled")

        self._conn.execute(
            """INSERT INTO devices (
                    public_key_id,
                    device_class,
                    capabilities,
                    active_caps,
                    manifest_version,
                    firmware_hash,
                    enrolled_at,
                    state,
                    mac_address)
                VALUES (?, ?, ?, ?, ?, ?, ?, 'enrolled', ?)
            """,
            (public_key_id,
             device_class,
             capabilities,
             capabilities,
             manifest_version,
             firmware_hash,
             time.time(),
             mac_address))

        self._audit(public_key_id, "enrolled",
                    f"class={device_class} "
                    f"{capabilities:#010x}")
        self._conn.commit()


# ---
# Lifecycle transistions
# ---

    def restrict(self,
                 public_key_id:  str,
                 permitted_mask: int) -> None:
        """
        Reduce active capabilities. Enforces ActiveSubset: permitted_mask is
        intersected with ceiling before storing. Corresponds to the Restrict
        transition in the TLA+ model.
        """
        device = self._get_active(public_key_id)
        bounded = permitted_mask & device["capabilities"]
        self._conn.execute(
            """UPDATE devices
               SET active_caps = ?,
                   state = 'restricted'
               WHERE public_key_id = ?""",
               (bounded, public_key_id))
        self._audit(public_key_id, "restricted",
            f"active_caps={bounded:#010x}")
        self._conn.commit()


    def quarantine(self, public_key_id: str) -> None:
        """
        Full isolation: active capabilities set to empty set.
        """
        self._get_active(public_key_id)
        self._conn.execute(
            """UPDATE devices
               SET active_caps = 0,
                   state = 'quarantined'
               WHERE public_key_id = ?""",
               (public_key_id,))
        self._audit(public_key_id, "quarantined")
        self._conn.commit()


    def restore(self, public_key_id: str) -> None:
        """
        Restore active capabilities to ceiling. Only valid from restricted
        state, quarantined devices must re-enroll.
        """
        device = self._get(public_key_id)
        if device["state"] == "quarantined":
            raise ValueError("Quarantined device must re-enroll to restore "
                             "capabilities")
        self._conn.execute(
            """UPDATE devices
               SET active_caps = capabilities,
                   state = 'enrolled'
               WHERE public_key_id = ?""",
               (public_key_id,))
        self._audit(public_key_id, "restored",
            f"caps={device['capabilities']:#010x}")
        self._conn.commit()


    def revoke(self, public_key_id: str) -> None:
        """
        Permanently invalidate enrollment. Device must re-enroll from scratch.
        """
        self._conn.execute(
            """UPDATE devices
               SET active_caps = 0,
                   state = 'revoked'
               WHERE public_key_id = ?""",
            (public_key_id,))
        self._audit(public_key_id, "revoked")
        self._conn.commit()


# ---
# Queries
# ---

    def get(self,
            public_key_id: str) -> DeviceRecord | None:
        row = self._get(public_key_id)
        if row is None:
            return None
        return self._to_record(row)


    def all_devices(self) -> list[DeviceRecord]:
        rows = self._conn.execute(
            "SELECT * FROM devices"
        ).fetchall()
        return [self._to_record(r) for r in rows]


    def active_capabilities(self, public_key_id: str) -> int:
        """
        Returns the current active capability bitmask for a device.
        Returns 0 for unenrolled/quarantined/revoked.
        """
        row = self._get(public_key_id)
        if row is None:
            return 0
        return row["active_caps"]


    def audit_log(self,
                  public_key_id: str | None,
                  limit: int = 100) -> list[dict]:
        if public_key_id:
            rows = self._conn.execute(
                """SELECT * FROM audit_log
                   WHERE public_key_id = ?
                   ORDER BY timestamp DESC
                   LIMIT ?""",
                   (public_key_id, limit)
            ).fetchall()
        else:
            rows = self._conn.execute(
                """SELECT * FROM audit_log
                   ORDER BY timestamp DESC
                   LIMIT ?""",
                (limit,)
            ).fetchall()
        return [dict(r) for r in rows]


# ---
# Internal helpers
# ---
    def _get(self, public_key_id: str) -> sqlite3.Row | None:
        return self._conn.execute(
            "SELECT * FROM devices "
            "WHERE public_key_id = ?",
            (public_key_id,)
        ).fetchone()


    def _get_active(self, public_key_id: str) -> sqlite3.Row:
        """
        Fetch device and assert it is active. Raises ValueError if not found
        or revoked.
        """
        row = self._get(public_key_id)
        if row is None:
            raise ValueError(f"Device {public_key_id} not found")
        if row["state"] == "revoked":
            raise ValueError(f"Device {public_key_id} is revoked")
        return row


    def _audit(self,
               public_key_id: str,
               event: str,
               detail: str = "") -> None:
        self._conn.execute(
            """INSERT INTO audit_log
               (timestamp, public_key_id, event, detail)
               VALUES (?, ?, ?, ?)""",
            (time.time(), public_key_id, event, detail))


    @staticmethod
    def _to_record(row: sqlite3.Row) -> DeviceRecord:
        return DeviceRecord(
            public_key_id=      row["public_key_id"],
            device_class=       row["device_class"],
            capabilities=       row["capabilities"],
            active_caps=        row["active_caps"],
            manifest_version=   row["manifest_version"],
            firmware_hash=      row["firmware_hash"],
            enrolled_at=        row["enrolled_at"],
            state=              row["state"],
            mac_address=        row["mac_address"])


    def close(self) -> None:
        self._conn.close()

