#!/usr/bin/env bash
# Unit tests for the experimental wrapper. No Flatpak app or host IBus is used.
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"
wrapper="$repo_root/tools/flatpak/ibus-flatpak-host-wrapper.sh"
tmp="$(mktemp -d)"
trap 'rm -rf -- "$tmp"' EXIT

fail() {
    printf 'FAIL: %s\n' "$*" >&2
    exit 1
}

[[ -f "$wrapper" ]] || fail "wrapper is missing: $wrapper"
bash -n "$wrapper" || fail 'wrapper has a shell syntax error'

mkdir -p "$tmp/bin"
cat > "$tmp/bin/flatpak" <<'MOCK'
#!/usr/bin/env bash
printf '%s\n' "$@" > "$FLATPAK_ARGS_FILE"
MOCK
chmod +x "$tmp/bin/flatpak"

if env -u SHANTI_FLATPAK_APP_ID PATH="$tmp/bin:/usr/bin:/bin" bash "$wrapper" >"$tmp/out" 2>&1; then
    fail 'wrapper accepted an unset app ID'
fi
grep -q 'set SHANTI_FLATPAK_APP_ID' "$tmp/out" || fail 'unset app ID error was not clear'

if SHANTI_FLATPAK_APP_ID='not valid' PATH="$tmp/bin:/usr/bin:/bin" bash "$wrapper" >"$tmp/out" 2>&1; then
    fail 'wrapper accepted a malformed app ID'
fi
grep -q 'not a valid reverse-DNS-style app ID' "$tmp/out" || fail 'malformed app ID error was not clear'

export FLATPAK_ARGS_FILE="$tmp/args"
SHANTI_FLATPAK_APP_ID='org.example.Shanti' PATH="$tmp/bin:/usr/bin:/bin" bash "$wrapper" --probe-arg
expected="$tmp/expected"
cat > "$expected" <<'EXPECTED'
run
--talk-name=org.freedesktop.IBus
--own-name=org.freedesktop.IBus.OpenBangla
--command=ibus-engine-openbangla
org.example.Shanti
--ibus
--probe-arg
EXPECTED
cmp -s "$expected" "$FLATPAK_ARGS_FILE" || {
    printf 'Expected Flatpak arguments:\n' >&2
    cat "$expected" >&2
    printf 'Actual Flatpak arguments:\n' >&2
    cat "$FLATPAK_ARGS_FILE" >&2
    fail 'wrapper passed unexpected arguments to Flatpak'
}

printf 'PASS: wrapper syntax, input validation, and Flatpak argument forwarding\n'
printf 'NOTE: this test does not validate Flatpak permissions, D-Bus connectivity, IBus discovery, or text entry.\n'
