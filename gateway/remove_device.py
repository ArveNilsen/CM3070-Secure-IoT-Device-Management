import sqlite3
import sys
from pathlib import Path

REGISTRY_PATH = Path(__file__).parent / "registry.db"


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <public_key_id>")
        sys.exit(1)

    public_key_id = sys.argv[1]
    conn = sqlite3.connect(str(REGISTRY_PATH))

    row = conn.execute(
        "SELECT public_key_id, state, enrolled_at FROM devices "
        "WHERE public_key_id = ?", (public_key_id,)
    ).fetchone()

    if row is None:
        print(f"No registry entry found for '{public_key_id}'.")
        conn.close()
        return

    print(f"Found: public_key_id={row[0]}, state={row[1]}, enrolled_at={row[2]}")
    confirm = input(f"Remove '{public_key_id}' from the registry? "
                    "This cannot be undone. [y/N] ")
    if confirm.lower() != 'y':
        print("Aborted.")
        conn.close()
        return

    conn.execute("DELETE FROM devices WHERE public_key_id = ?", (public_key_id,))
    conn.commit()
    conn.close()
    print(f"Removed '{public_key_id}' from the registry.")


if __name__ == "__main__":
    main()
