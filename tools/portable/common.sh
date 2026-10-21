#!/usr/bin/env bash

die() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

log() {
    printf '==> %s\n' "$*"
}

require_linux() {
    [[ "$(uname -s)" == "Linux" ]] || die "This experimental installer supports Linux only."
}

normalize_home() {
    local canonical_home
    [[ "${HOME:-}" == /* && -d "$HOME" ]] ||
        die "HOME must be an existing absolute directory."
    canonical_home="$(realpath -e -- "$HOME")" ||
        die "Could not normalize HOME safely."
    [[ "$canonical_home" != "/" ]] ||
        die "Refusing to use filesystem root as HOME."
    HOME="$canonical_home"
    export HOME
}

assert_no_symlink_components() {
    local path="$1" current="/" component
    local -a components=()

    [[ "$path" == /* && "$path" != "/" ]] ||
        die "Refusing an invalid path while checking for symlinks: $path"

    IFS='/' read -r -a components <<< "$path"
    for component in "${components[@]}"; do
        [[ -n "$component" ]] || continue
        if [[ "$current" == "/" ]]; then
            current="/$component"
        else
            current="$current/$component"
        fi
        [[ ! -L "$current" ]] ||
            die "Refusing to use a path containing a symlink component: $current (requested: $path)"
    done
}
assert_no_nested_mounts() {
    local path="$1" nested_mount
    [[ -d "$path" && ! -L "$path" ]] ||
        die "Refusing to inspect an invalid recursive-operation target: $path"
    command -v mountpoint >/dev/null 2>&1 ||
        die "Cannot verify mount boundaries because mountpoint is unavailable; refusing recursive operation on: $path"
    nested_mount="$(find "$path" -type d -exec mountpoint -q -- {} \; -print -quit 2>/dev/null)" ||
        die "Could not inspect mount boundaries safely; refusing recursive operation on: $path"
    [[ -z "$nested_mount" ]] ||
        die "Refusing recursive operation because the target contains a mount point: $nested_mount"
}

xdg_dir() {
    local variable="$1"
    local fallback="$2"
    local value="${!variable:-}"
    # Ignore relative and root-valued XDG settings, then canonicalize the
    # selected value including the fallback. This prevents later mkdir/copy
    # operations from traversing a symlink in the default ~/.local/share or
    # ~/.cache path.
    if [[ "$value" != /* || "$value" == "/" ]]; then
        value="$fallback"
    fi
    value="$(realpath -m -- "$value" 2>/dev/null)" ||
        die "Could not normalize $variable safely."
    [[ "$value" == /* && "$value" != "/" ]] ||
        die "Could not resolve a safe path for $variable."
    printf '%s\n' "$value"
}

prepare_paths() {
    local data_home cache_home
    normalize_home

    data_home="$(xdg_dir XDG_DATA_HOME "$HOME/.local/share")"
    cache_home="$(xdg_dir XDG_CACHE_HOME "$HOME/.cache")"

    OBK_PREFIX="$HOME/.local"
    OBK_DATA_HOME="$data_home"
    OBK_CACHE="$cache_home/openbangla-keyboard"
    OBK_BUILD="$OBK_CACHE/build"
    OBK_WORKSPACE="$(cd -- "$SCRIPT_DIR/.." && pwd)"

    # Validate all write roots before mkdir can traverse any symlinked parent.
    assert_no_symlink_components "$OBK_PREFIX"
    assert_no_symlink_components "$OBK_BUILD"
    assert_no_symlink_components "$OBK_DATA_HOME/ibus/component"
    mkdir -p "$OBK_BUILD" "$OBK_DATA_HOME/ibus/component"

    # Never clear a possibly stale staging directory. Create a new one for
    # each invocation so a missing/unset path can never expand to /*.
    OBK_STAGE="$(mktemp -d "$OBK_CACHE/stage.XXXXXXXX")" ||
        die "Could not create a fresh OpenBangla staging directory."

    export OBK_PREFIX OBK_DATA_HOME OBK_CACHE OBK_BUILD OBK_STAGE OBK_WORKSPACE
}
