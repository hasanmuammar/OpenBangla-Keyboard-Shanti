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

# The engine is installed under /app/libexec, which is not on Flatpak's
# default PATH (/app/bin:/usr/bin). Keep the D-Bus grants limited to the
# service name the engine talks to and the name it owns.
exec flatpak run \
    "--env=IBUS_ADDRESS=$ibus_address" \
    --talk-name=org.freedesktop.IBus \
    --own-name=org.freedesktop.IBus.OpenBangla \
    --command=/app/libexec/ibus-engine-openbangla \
    "$app_id" --ibus "$@"
