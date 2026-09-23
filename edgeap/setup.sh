#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"

if [ "$EUID" -ne 0 ]; then
		echo "Run as root (sudo)."
		exit 1
fi

for script in "$DIR"/scripts/[0-9][0-9]-*.sh; do
		name="$(basename "$script")"
		echo ""
		echo "=== Running $name ==="
		bash "$script"
done

echo ""
echo "=== Setup complete ==="
