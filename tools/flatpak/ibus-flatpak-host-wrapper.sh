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

# The engine's --ibus startup path requests ownership of
# org.freedesktop.IBus.OpenBangla and talks to org.freedesktop.IBus.
# Grant only those two names for this invocation. Do not replace this with
# --socket=session-bus or a broad session-bus policy without security review.
# The actual D-Bus behaviour still needs an end-to-end test on a disposable
# desktop session.
exec flatpak run \
    --talk-name=org.freedesktop.IBus \
    --own-name=org.freedesktop.IBus.OpenBangla \
    --command=ibus-engine-openbangla \
    "$app_id" --ibus "$@"
