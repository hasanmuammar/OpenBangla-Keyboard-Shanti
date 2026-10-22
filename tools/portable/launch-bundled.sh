#!/usr/bin/env bash
set -Eeuo pipefail

HERE="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
NAME="${0##*/}"
REAL="$HERE/${NAME}.bin"

export LD_LIBRARY_PATH="$HERE/../lib/shanti${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

exec "$REAL" "$@"
