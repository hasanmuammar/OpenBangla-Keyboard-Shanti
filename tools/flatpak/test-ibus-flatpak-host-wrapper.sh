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
if [[ "${MOCK_FLATPAK_HOLD:-0}" == 1 ]]; then
    sleep 60 &
    child_pid=$!
    printf '%s %s\n' "$" "$child_pid" > "$FLATPAK_PIDS_FILE"
    wait "$child_pid"
fi
MOCK
cat > "$tmp/bin/ibus" <<'MOCK'
#!/usr/bin/env bash
[[ "${1:-}" == address ]] || exit 2
printf '%s\n' 'unix:path=/tmp/ibus-test,guid=0123456789abcdef'
MOCK
chmod +x "$tmp/bin/flatpak" "$tmp/bin/ibus"

if env -u SHANTI_FLATPAK_APP_ID PATH="$tmp/bin:/usr/bin:/bin" bash "$wrapper" >"$tmp/out" 2>&1; then
    fail 'wrapper accepted an unset app ID'
fi
grep -q 'set SHANTI_FLATPAK_APP_ID' "$tmp/out" || fail 'unset app ID error was not clear'

if SHANTI_FLATPAK_APP_ID='not valid' PATH="$tmp/bin:/usr/bin:/bin" bash "$wrapper" >"$tmp/out" 2>&1; then
    fail 'wrapper accepted a malformed app ID'
fi
grep -q 'not a valid reverse-DNS-style app ID' "$tmp/out" || fail 'malformed app ID error was not clear'

# Invoke Bash by its absolute path while exposing an empty PATH to the wrapper,
# so this check remains deterministic even on hosts where Flatpak is installed.
bash_path="$(command -v bash)"
mkdir -p "$tmp/empty-path"
set +e
SHANTI_FLATPAK_APP_ID='org.example.Shanti' PATH="$tmp/empty-path" "$bash_path" "$wrapper" >"$tmp/out" 2>&1
missing_flatpak_status=$?
set -e
[[ "$missing_flatpak_status" -eq 69 ]] || fail "missing flatpak returned exit code $missing_flatpak_status, expected 69"
grep -q 'flatpak command not found' "$tmp/out" || fail 'missing flatpak error was not clear'

# Verify explicit address discovery, narrowly scoped settings access, and argument forwarding.
mkdir -p "$tmp/host-config/OpenBangla"
export XDG_CONFIG_HOME="$tmp/host-config"
export FLATPAK_ARGS_FILE="$tmp/args"
SHANTI_FLATPAK_APP_ID='org.example.Shanti' PATH="$tmp/bin:/usr/bin:/bin" bash "$wrapper" --probe-arg
expected="$tmp/expected"
cat > "$expected" <<EXPECTED
run
--filesystem=$tmp/host-config/OpenBangla:ro
--env=XDG_CONFIG_HOME=$tmp/host-config
--filesystem=/tmp/ibus-test:ro
--env=IBUS_ADDRESS=unix:path=/tmp/ibus-test,guid=0123456789abcdef
--talk-name=org.freedesktop.IBus
--own-name=org.freedesktop.IBus.OpenBangla
--command=/app/libexec/ibus-engine-openbangla
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

# Verify terminating the wrapper also terminates its isolated Flatpak process
# group. This uses a sleeping mock, not a real Flatpak deployment.
export FLATPAK_PIDS_FILE="$tmp/pids"
MOCK_FLATPAK_HOLD=1 SHANTI_FLATPAK_APP_ID='org.example.Shanti' \
    PATH="$tmp/bin:/usr/bin:/bin" bash "$wrapper" >"$tmp/lifecycle.out" 2>&1 &
wrapper_pid=$!
for _ in {1..50}; do
    [[ -s "$FLATPAK_PIDS_FILE" ]] && break
    sleep 0.1
done
[[ -s "$FLATPAK_PIDS_FILE" ]] || fail 'mock Flatpak did not start for lifecycle test'
read -r mock_pid mock_child_pid < "$FLATPAK_PIDS_FILE"
kill -TERM "$wrapper_pid" 2>/dev/null || true
set +e
wait "$wrapper_pid"
wrapper_status=$?
set -e
[[ "$wrapper_status" -eq 143 ]] || fail "TERM returned wrapper status $wrapper_status, expected 143"
for _ in {1..20}; do
    kill -0 "$mock_pid" 2>/dev/null || break
    sleep 0.1
done
if kill -0 "$mock_pid" 2>/dev/null; then
    fail "mock Flatpak process $mock_pid survived wrapper termination"
fi

printf 'PASS: wrapper validation, address discovery, argument forwarding, and signal cleanup\n'
printf 'NOTE: tests do not validate real Flatpak permissions, D-Bus connectivity, IBus discovery, or text entry.\n'
