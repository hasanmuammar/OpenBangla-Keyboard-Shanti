#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

if [[ -f "$SCRIPT_DIR/portable/common.sh" ]]; then
    # The helper library ships beside this script in the checkout and inside
    # every release archive. Never re-copy shared helpers into this file.
    source "$SCRIPT_DIR/portable/common.sh"
else
    printf 'Error: tools/portable/common.sh helper library not found; run this script from the checkout or the release archive.\n' >&2
    exit 1
fi

normalize_home
PREFIX="$(realpath -m -- "$HOME/.local")" ||
    die "Could not normalize the install prefix safely."

DATA_HOME="$(xdg_dir XDG_DATA_HOME "$HOME/.local/share")"
CONFIG_HOME="$(xdg_dir XDG_CONFIG_HOME "$HOME/.config")"
CACHE_HOME="$(xdg_dir XDG_CACHE_HOME "$HOME/.cache")"

PURGE_CACHE=0
PURGE_DATA=0
FCITX_FILES=()

usage() {
    cat <<'EOF'
Usage: tools/uninstall.sh [--purge-cache] [--purge-data]

Remove the current user's Shanti Keyboard installation.

Options:
  --purge-cache   Also remove the build/staging cache at
                  ~/.cache/shanti-keyboard or the legacy
                  ~/.cache/openbangla-keyboard (or XDG_CACHE_HOME).
  --purge-data    Also remove Shanti user data, including custom layouts
                  and autocorrect data. This is not removed by default.
  -h, --help      Show this help.
EOF
}

is_allowed_shanti_target() {
    local path="$1" relative directory

    case "$path" in
        "$PREFIX/bin/openbangla-gui"|"$PREFIX/bin/openbangla-gui.bin"|"$PREFIX/libexec/ibus-engine-openbangla"|"$PREFIX/libexec/ibus-engine-openbangla.bin"|"$PREFIX/lib/openbangla"|"$DATA_HOME/openbangla-keyboard"|"$DATA_HOME/applications/openbangla-keyboard.desktop"|"$DATA_HOME/ibus/component/openbangla.xml"|"$DATA_HOME/fcitx5/addon/openbangla.conf"|"$DATA_HOME/fcitx5/inputmethod/openbangla.conf"|"$DATA_HOME/metainfo/io.github.openbangla.keyboard.metainfo.xml"|"$DATA_HOME/pixmaps/openbangla-keyboard.png"|"$DATA_HOME/.openbangla-keyboard-xdg-migration-v1-complete"|"$CONFIG_HOME/environment.d/90-openbangla-ibus.conf"|"$CONFIG_HOME/fcitx5/profile"|"$CACHE_HOME/openbangla-keyboard"|\
        "$PREFIX/bin/shanti-gui"|"$PREFIX/bin/shanti-gui.bin"|"$PREFIX/libexec/ibus-engine-shanti"|"$PREFIX/libexec/ibus-engine-shanti.bin"|"$PREFIX/lib/shanti"|"$DATA_HOME/shanti-keyboard"|"$DATA_HOME/applications/shanti-keyboard.desktop"|"$DATA_HOME/ibus/component/shanti.xml"|"$DATA_HOME/fcitx5/addon/shanti.conf"|"$DATA_HOME/fcitx5/inputmethod/shanti.conf"|"$DATA_HOME/metainfo/org.shanti.ShantiKeyboard.metainfo.xml"|"$DATA_HOME/pixmaps/shanti-keyboard.png"|"$DATA_HOME/.shanti-keyboard-xdg-migration-v1-complete"|"$DATA_HOME/ibus/component/shanti-flatpak.xml"|"$CONFIG_HOME/environment.d/90-shanti-ibus.conf"|"$CONFIG_HOME/fcitx5/profile"|"$CACHE_HOME/shanti-keyboard")
            return 0
            ;;
        "$DATA_HOME/icons/hicolor/16x16/apps/openbangla-keyboard.png"|"$DATA_HOME/icons/hicolor/32x32/apps/openbangla-keyboard.png"|"$DATA_HOME/icons/hicolor/48x48/apps/openbangla-keyboard.png"|"$DATA_HOME/icons/hicolor/128x128/apps/openbangla-keyboard.png"|"$DATA_HOME/icons/hicolor/512x512/apps/openbangla-keyboard.png"|"$DATA_HOME/icons/hicolor/1024x1024/apps/openbangla-keyboard.png"|"$PREFIX/lib/fcitx5/openbangla.so"|"$PREFIX/lib64/fcitx5/openbangla.so"|"$PREFIX/lib/fcitx5/addon/openbangla.conf"|"$PREFIX/lib/fcitx5/inputmethod/openbangla.conf"|"$PREFIX/share/fcitx5/addon/openbangla.conf"|"$PREFIX/share/fcitx5/inputmethod/openbangla.conf"|\
        "$DATA_HOME/icons/hicolor/16x16/apps/shanti-keyboard.png"|"$DATA_HOME/icons/hicolor/32x32/apps/shanti-keyboard.png"|"$DATA_HOME/icons/hicolor/48x48/apps/shanti-keyboard.png"|"$DATA_HOME/icons/hicolor/128x128/apps/shanti-keyboard.png"|"$DATA_HOME/icons/hicolor/512x512/apps/shanti-keyboard.png"|"$DATA_HOME/icons/hicolor/1024x1024/apps/shanti-keyboard.png"|"$PREFIX/lib/fcitx5/shanti.so"|"$PREFIX/lib64/fcitx5/shanti.so"|"$PREFIX/lib/fcitx5/addon/shanti.conf"|"$PREFIX/lib/fcitx5/inputmethod/shanti.conf"|"$PREFIX/share/fcitx5/addon/shanti.conf"|"$PREFIX/share/fcitx5/inputmethod/shanti.conf")
            return 0
            ;;
    esac

    # Multiarch builds place Fcitx modules one directory below lib/. Limit
    # the wildcard to exactly that one directory level.
    case "$path" in
        "$PREFIX"/lib/*/fcitx5/openbangla.so)
            relative="${path#"$PREFIX"/lib/}"
            directory="${relative%/fcitx5/openbangla.so}"
            [[ -n "$directory" && "$directory" != */* ]] && return 0
            ;;
        "$PREFIX"/lib64/*/fcitx5/openbangla.so)
            relative="${path#"$PREFIX"/lib64/}"
            directory="${relative%/fcitx5/openbangla.so}"
            [[ -n "$directory" && "$directory" != */* ]] && return 0
            ;;
        "$PREFIX"/lib/*/fcitx5/addon/openbangla.conf)
            relative="${path#"$PREFIX"/lib/}"
            directory="${relative%/fcitx5/addon/openbangla.conf}"
            [[ -n "$directory" && "$directory" != */* ]] && return 0
            ;;
        "$PREFIX"/lib/*/fcitx5/inputmethod/openbangla.conf)
            relative="${path#"$PREFIX"/lib/}"
            directory="${relative%/fcitx5/inputmethod/openbangla.conf}"
            [[ -n "$directory" && "$directory" != */* ]] && return 0
            ;;
        "$PREFIX"/lib/*/fcitx5/shanti.so)
            relative="${path#"$PREFIX"/lib/}"
            directory="${relative%/fcitx5/shanti.so}"
            [[ -n "$directory" && "$directory" != */* ]] && return 0
            ;;
        "$PREFIX"/lib64/*/fcitx5/shanti.so)
            relative="${path#"$PREFIX"/lib64/}"
            directory="${relative%/fcitx5/shanti.so}"
            [[ -n "$directory" && "$directory" != */* ]] && return 0
            ;;
        "$PREFIX"/lib/*/fcitx5/addon/shanti.conf)
            relative="${path#"$PREFIX"/lib/}"
            directory="${relative%/fcitx5/addon/shanti.conf}"
            [[ -n "$directory" && "$directory" != */* ]] && return 0
            ;;
        "$PREFIX"/lib/*/fcitx5/inputmethod/shanti.conf)
            relative="${path#"$PREFIX"/lib/}"
            directory="${relative%/fcitx5/inputmethod/shanti.conf}"
            [[ -n "$directory" && "$directory" != */* ]] && return 0
            ;;
    esac

    return 1
}
remove_path() {
    local path="$1"
    local allowed=0 root normalized normalized_parent normalized_root cache_target parent

    [[ "$path" == /* && "$path" != "/" ]] ||
        die "Refusing to remove an empty, relative, or root path: $path"

    # Resolve symlinks in parent directories but preserve the final component
    # as a name, so a symlink itself can be unlinked without following its target.
    normalized_parent="$(realpath -m -- "$(dirname -- "$path")" 2>/dev/null)" ||
        die "Could not normalize uninstall target safely: $path"
    normalized="$normalized_parent/${path##*/}"
    [[ "$normalized" != "/" ]] ||
        die "Refusing to remove the filesystem root."

    # Constrain removal to the exact application paths this uninstaller owns;
    # membership somewhere below ~/.local or an XDG root is not sufficient.
    is_allowed_shanti_target "$normalized" ||
        die "Refusing to remove a path that is not an explicit Shanti (or legacy OpenBangla) target: $path"

    # Reject symlinked parent components even when their targets happen to
    # remain inside a managed root; those paths may contain unrelated user data.
    parent="$(dirname -- "$path")"
    while [[ "$parent" != "/" ]]; do
        [[ ! -L "$parent" ]] ||
            die "Refusing to remove through a symlinked parent directory: $parent"
        case "$parent" in
            "$PREFIX"|"$DATA_HOME"|"$CONFIG_HOME"|"$CACHE_HOME")
                break
                ;;
        esac
        parent="$(dirname -- "$parent")"
    done

    for root in "$PREFIX" "$DATA_HOME" "$CONFIG_HOME"; do
        normalized_root="$(realpath -m -- "$root" 2>/dev/null)" || continue
        if [[ "$normalized" == "$normalized_root/"* ]]; then
            allowed=1
            break
        fi
    done
    cache_target="$(realpath -m -- "$CACHE_HOME/openbangla-keyboard" 2>/dev/null)" || cache_target=""
    [[ -n "$cache_target" && "$normalized" == "$cache_target" ]] && allowed=1
    [[ "$allowed" -eq 1 ]] ||
        die "Refusing to remove a path outside the Shanti uninstall targets: $path"

    for root in "$PREFIX" "$DATA_HOME" "$CONFIG_HOME" "$CACHE_HOME"; do
        normalized_root="$(realpath -m -- "$root" 2>/dev/null)" || continue
        [[ "$normalized" != "$normalized_root" ]] ||
            die "Refusing to remove an entire uninstall root: $path"
    done

    if [[ -e "$path" || -L "$path" ]]; then
        if [[ -L "$path" || -f "$path" ]]; then
            rm -f -- "$path"
        elif [[ -d "$path" ]]; then
            assert_no_nested_mounts "$path"
            rm -rf --one-file-system -- "$path"
        else
            die "Refusing to remove an unexpected special file type: $path"
        fi
        log "Removed $path"
    fi
}

remove_dconf_tuple() {
    local key="$1"
    local tuple="$2"
    local value cleaned

    command -v dconf >/dev/null 2>&1 || return 0

    value="$(dconf read "$key" 2>/dev/null || true)"
    [[ -n "$value" && "$value" != "[]" ]] || return 0
    [[ "$value" == *"$tuple"* ]] || return 0

    cleaned="${value//"$tuple"/}"
    cleaned="${cleaned//, ,/, }"
    cleaned="${cleaned//, ]/]}"
    cleaned="${cleaned//[, /[}"

    if [[ -z "$cleaned" ]]; then
        cleaned="[]"
    fi

    dconf write "$key" "$cleaned"
    log "Updated $key"
}

remove_fcitx_profile_entry() {
    local profile="$CONFIG_HOME/fcitx5/profile"
    local tmp

    [[ -f "$profile" ]] || return 0
    grep -Eq '^Name=(openbangla|shanti)$' "$profile" || return 0

    tmp="$(mktemp)"

    awk '
        function flush() {
            if (section != "" && !drop)
                printf "%s", block
        }

        /^\[/ {
            flush()
            section = $0
            block = $0 ORS
            drop = 0
            next
        }

        {
            block = block $0 ORS
            if (section ~ /^\[Groups\/[0-9]+\/Items\/[0-9]+\]$/ &&
                $0 == "Name=openbangla" || $0 == "Name=shanti")
                drop = 1
        }

        END {
            flush()
        }
    ' "$profile" > "$tmp"

    sed -i -E 's/^DefaultIM=(openbangla|shanti)$/DefaultIM=keyboard-us/' "$tmp"

    if ! cmp -s "$profile" "$tmp"; then
        mv -- "$tmp" "$profile"
        log "Removed Shanti entries from $profile"
    else
        rm -f -- "$tmp"
    fi
}

remove_ibus_registration() {
    local component_dir="$DATA_HOME/ibus/component"

    remove_path "$component_dir/openbangla.xml"
    remove_path "$component_dir/shanti.xml"

    if command -v ibus >/dev/null 2>&1; then
        IBUS_COMPONENT_PATH="$component_dir:/usr/share/ibus/component" \
            ibus write-cache >/dev/null 2>&1 || true
        ibus restart >/dev/null 2>&1 || true
    fi

    remove_dconf_tuple \
        "/org/gnome/desktop/input-sources/sources" \
        "('ibus', 'OpenBangla')"

    remove_dconf_tuple \
        "/org/gnome/desktop/input-sources/sources" \
        "('ibus', 'Shanti')"

    remove_dconf_tuple \
        "/org/gnome/desktop/input-sources/mru-sources" \
        "('ibus', 'OpenBangla')"

    remove_dconf_tuple \
        "/org/gnome/desktop/input-sources/mru-sources" \
        "('ibus', 'Shanti')"

    remove_path "$CONFIG_HOME/environment.d/90-openbangla-ibus.conf"
    remove_path "$CONFIG_HOME/environment.d/90-shanti-ibus.conf"

    if command -v systemctl >/dev/null 2>&1; then
        systemctl --user unset-environment IBUS_COMPONENT_PATH >/dev/null 2>&1 || true
    fi
}

remove_fcitx_registration() {
    local path
    for path in "${FCITX_FILES[@]}"; do
        # Remove only regular files found and displayed before the user confirmed.
        [[ -f "$path" ]] || continue
        remove_path "$path"
    done

    remove_fcitx_profile_entry

    if command -v fcitx5-remote >/dev/null 2>&1; then
        fcitx5-remote -r >/dev/null 2>&1 || true
    fi
}

main() {
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --purge-cache)
                PURGE_CACHE=1
                ;;
            --purge-data)
                PURGE_DATA=1
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                die "Unknown option: $1"
                ;;
        esac
        shift
    done

    if [[ $PURGE_DATA -eq 1 ]]; then
        [[ -t 0 ]] || die "--purge-data requires an interactive confirmation; user data will be preserved."
        printf 'User data may include custom layouts and autocorrect data. Targets: %s %s\n' "$DATA_HOME/openbangla-keyboard" "$DATA_HOME/shanti-keyboard"
        read -r -p 'Delete this user-data directory as well? [y/N] ' data_reply
        [[ "$data_reply" =~ ^[Yy]([Ee][Ss])?$ ]] || PURGE_DATA=0
    fi

    if [[ $PURGE_CACHE -eq 1 ]]; then
        [[ -t 0 ]] || die "--purge-cache requires an interactive confirmation; cache data will be preserved."
        printf 'Build/staging cache targets: %s %s\n' "$CACHE_HOME/openbangla-keyboard" "$CACHE_HOME/shanti-keyboard"
        read -r -p 'Delete this cache directory as well? [y/N] ' cache_reply
        [[ "$cache_reply" =~ ^[Yy]([Ee][Ss])?$ ]] || PURGE_CACHE=0
    fi

    [[ -t 0 ]] || die "Interactive confirmation is required before uninstalling."

    printf 'Review these Shanti program and registration paths before continuing:\n'
    for path in \
        "$PREFIX/bin/openbangla-gui" \
        "$PREFIX/bin/openbangla-gui.bin" \
        "$PREFIX/libexec/ibus-engine-openbangla" \
        "$PREFIX/libexec/ibus-engine-openbangla.bin" \
        "$PREFIX/lib/openbangla" \
        "$DATA_HOME/applications/openbangla-keyboard.desktop" \
        "$DATA_HOME/ibus/component/openbangla.xml" \
        "$DATA_HOME/fcitx5/addon/openbangla.conf" \
        "$DATA_HOME/fcitx5/inputmethod/openbangla.conf" \
        "$DATA_HOME/metainfo/io.github.openbangla.keyboard.metainfo.xml" \
        "$DATA_HOME/pixmaps/openbangla-keyboard.png" \
        "$DATA_HOME/.openbangla-keyboard-xdg-migration-v1-complete" \
        "$CONFIG_HOME/environment.d/90-openbangla-ibus.conf" \
        "$DATA_HOME/ibus/component/shanti-flatpak.xml" \
        "$PREFIX/bin/shanti-gui" \
        "$PREFIX/bin/shanti-gui.bin" \
        "$PREFIX/libexec/ibus-engine-shanti" \
        "$PREFIX/libexec/ibus-engine-shanti.bin" \
        "$PREFIX/lib/shanti" \
        "$DATA_HOME/applications/shanti-keyboard.desktop" \
        "$DATA_HOME/ibus/component/shanti.xml" \
        "$DATA_HOME/fcitx5/addon/shanti.conf" \
        "$DATA_HOME/fcitx5/inputmethod/shanti.conf" \
        "$DATA_HOME/metainfo/org.shanti.ShantiKeyboard.metainfo.xml" \
        "$DATA_HOME/pixmaps/shanti-keyboard.png" \
        "$DATA_HOME/.shanti-keyboard-xdg-migration-v1-complete" \
        "$CONFIG_HOME/environment.d/90-shanti-ibus.conf" \
        "$CONFIG_HOME/fcitx5/profile"; do
        printf '  %s\n' "$path"
    done
    for size in 16 32 48 128 512 1024; do
        printf '  %s\n' "$DATA_HOME/icons/hicolor/${size}x${size}/apps/openbangla-keyboard.png"
    done
    for size in 16 32 48 128 512 1024; do
        printf '  %s\n' "$DATA_HOME/icons/hicolor/${size}x${size}/apps/shanti-keyboard.png"
    done
    if [[ $PURGE_CACHE -eq 1 ]]; then
        printf 'Requested cache purge targets: %s %s\n' "$CACHE_HOME/openbangla-keyboard" "$CACHE_HOME/shanti-keyboard"
    fi
    if [[ $PURGE_DATA -eq 1 ]]; then
        printf 'Requested user-data purge targets: %s %s\n' "$DATA_HOME/openbangla-keyboard" "$DATA_HOME/shanti-keyboard"
    else
        printf 'Preserving user data by default: %s %s\n' "$DATA_HOME/openbangla-keyboard" "$DATA_HOME/shanti-keyboard"
    fi
    if [[ -d "$PREFIX" ]]; then
        mapfile -d '' -t FCITX_FILES < <(
            find "$PREFIX" -type f \
                \( -path '*/fcitx5/openbangla.so' -o \
                   -path '*/fcitx5/inputmethod/openbangla.conf' -o \
                   -path '*/fcitx5/addon/openbangla.conf' -o \
                   -path '*/fcitx5/shanti.so' -o \
                   -path '*/fcitx5/inputmethod/shanti.conf' -o \
                   -path '*/fcitx5/addon/shanti.conf' \) \
                -print0 2>/dev/null || true
        )
        printf 'Matching Fcitx files that would be removed:\n'
        for path in "${FCITX_FILES[@]}"; do
            printf '  %s\n' "$path"
        done
    fi
    printf 'The script will remove the listed program files and registrations. User data is preserved unless you separately confirm --purge-data.\n'
    read -r -p 'Proceed with these removals? [y/N] ' reply
    [[ "$reply" =~ ^[Yy]([Ee][Ss])?$ ]] ||
        exit 0

    remove_path "$PREFIX/bin/openbangla-gui"
    remove_path "$PREFIX/bin/openbangla-gui.bin"
    remove_path "$PREFIX/libeec/ibus-engine-openbangla"
    remove_path "$PREFIX/libexec/ibus-engine-openbangla.bin"
    remove_path "$PREFIX/lib/openbangla"
    remove_path "$PREFIX/bin/shanti-gui"
    remove_path "$PREFIX/bin/shanti-gui.bin"
    remove_path "$PREFIX/libexec/ibus-engine-shanti"
    remove_path "$PREFIX/libexec/ibus-engine-shanti.bin"
    remove_path "$PREFIX/lib/shanti"

    if [[ $PURGE_DATA -eq 1 ]]; then
        remove_path "$DATA_HOME/openbangla-keyboard"
        remove_path "$DATA_HOME/shanti-keyboard"
    fi

    remove_path "$DATA_HOME/applications/openbangla-keyboard.desktop"
    remove_path "$DATA_HOME/fcitx5/addon/openbangla.conf"
    remove_path "$DATA_HOME/fcitx5/inputmethod/openbangla.conf"
    remove_path "$DATA_HOME/metainfo/io.github.openbangla.keyboard.metainfo.xml"
    remove_path "$DATA_HOME/pixmaps/openbangla-keyboard.png"
    remove_path "$DATA_HOME/.openbangla-keyboard-xdg-migration-v1-complete"
    remove_path "$DATA_HOME/.shanti-keyboard-xdg-migration-v1-complete"
    remove_path "$DATA_HOME/ibus/component/shanti-flatpak.xml"
    remove_path "$DATA_HOME/applications/shanti-keyboard.desktop"
    remove_path "$DATA_HOME/fcitx5/addon/shanti.conf"
    remove_path "$DATA_HOME/fcitx5/inputmethod/shanti.conf"
    remove_path "$DATA_HOME/metainfo/org.shanti.ShantiKeyboard.metainfo.xml"
    remove_path "$DATA_HOME/pixmaps/shanti-keyboard.png"

    for size in 16 32 48 128 512 1024; do
        remove_path "$DATA_HOME/icons/hicolor/${size}x${size}/apps/openbangla-keyboard.png"
    done
    for size in 16 32 48 128 512 1024; do
        remove_path "$DATA_HOME/icons/hicolor/${size}x${size}/apps/shanti-keyboard.png"
    done

    remove_ibus_registration
    remove_fcitx_registration

    if [[ $PURGE_CACHE -eq 1 ]]; then
        remove_path "$CACHE_HOME/openbangla-keyboard"
        remove_path "$CACHE_HOME/shanti-keyboard"
    fi

    printf 'Shanti Keyboard was removed for the current user.\n'
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    main "$@"
fi
