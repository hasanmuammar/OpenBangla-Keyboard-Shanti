#!/usr/bin/env bash
# EXPERIMENTAL ONLY: host IBus -> Flatpak engine launcher.
# Not installed by the production installer and not a production integration.
set -euo pipefail

fail() {
    printf 'shanti-ibus-flatpak-wrapper: %s\n' "$*" >&2
    exit "${2:-64}"
}

# Require an explicit app ID so this prototype cannot silently launch an
# unrelated or guessed deployment.
app_id="${SHANTI_FLATPAK_APP_ID:-}"
[[ -n "$app_id" ]] || fail 'set SHANTI_FLATPAK_APP_ID to the installed Shanti Flatpak app ID'
[[ "$app_id" =~ ^[A-Za-z0-9_-]+(\.[A-Za-z0-9_-]+)+$ ]] || fail 'SHANTI_FLATPAK_APP_ID is not a valid reverse-DNS-style app ID'
command -v flatpak >/dev/null 2>&1 || fail 'flatpak command not found' 69
command -v ibus >/dev/null 2>&1 || fail 'ibus command not found; cannot discover the host IBus address' 69

# ibus_bus_new() did not discover the host IBus socket reliably in the
# Flatpak environment. Pass the address reported by the host ibus CLI.
ibus_address="$(ibus address)" || fail 'failed to obtain the host IBus address' 69
[[ -n "$ibus_address" ]] || fail 'host IBus returned an empty address' 69

# Flatpak cannot see the host IBus Unix socket unless its path is granted.
# Grant read-only access to the specific socket, not the whole IBus cache.
[[ "$ibus_address" == unix:path=* ]] || fail 'host IBus address is not a supported unix:path address' 69
ibus_socket_path="${ibus_address#unix:path=}"
ibus_socket_path="${ibus_socket_path%%,*}"
[[ -n "$ibus_socket_path" && "$ibus_socket_path" == /* ]] || fail 'host IBus address contains an invalid socket path' 69

# Run Flatpak in its own process group so the EXIT/signal cleanup can stop
# the sandbox and its descendants without using app-wide "flatpak kill".
# Enabling job control gives this asynchronous job a distinct process group.
flatpak_pid=""
cleanup() {
    local status=$?
    trap - EXIT INT TERM HUP

    if [[ -n "$flatpak_pid" ]]; then
        if kill -0 -- "-$flatpak_pid" 2>/dev/null; then
            kill -TERM -- "-$flatpak_pid" 2>/dev/null || true
            for _ in {1..20}; do
                kill -0 -- "-$flatpak_pid" 2>/dev/null || break
                sleep 0.1
            done
            kill -KILL -- "-$flatpak_pid" 2>/dev/null || true
        fi
        wait "$flatpak_pid" 2>/dev/null || true
    fi

    exit "$status"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
set -m

# The engine is installed under /app/libexec, which is not on Flatpak's
# default PATH (/app/bin:/usr/bin). Keep the D-Bus grants narrowly scoped.
flatpak run \
    "--filesystem=$ibus_socket_path:ro" \
    "--env=IBUS_ADDRESS=$ibus_address" \
    --talk-name=org.freedesktop.IBus \
    --own-name=org.freedesktop.IBus.OpenBangla \
    --command=/app/libexec/ibus-engine-openbangla \
    "$app_id" --ibus "$@" &
flatpak_pid=$!

set +e
wait "$flatpak_pid"
status=$?
set -e
exit "$status"
