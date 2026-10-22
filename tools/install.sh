#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/portable/common.sh"
source "$SCRIPT_DIR/portable/install.sh"

REPOSITORY="hasanmuammar/OpenBangla-Keyboard-Shanti"
REQUESTED_VERSION="${SHANTI_VERSION:-}"
RELEASE_VERSION="latest"
RELEASE_BASE=""
VERIFY_PROVENANCE=0
RELEASE_TAG_FILE="$SCRIPT_DIR/../release-tag.txt"

usage() {
    cat <<'EOF'
Usage: tools/install.sh [options]

Install the prebuilt Shanti Keyboard build specified by release-tag.txt for this fork.

Options:
  --version TAG      Install a specific Shanti release tag.
  --verify-provenance Verify signed build provenance (requires gh and a versioned tag).
  -h, --help          Show this help.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)
            [[ $# -ge 2 ]] || die "--version requires a release tag."
            REQUESTED_VERSION="$2"
            shift 2
            ;;
        --verify-provenance)
            VERIFY_PROVENANCE=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            die "Unknown option: $1"
            ;;
    esac
done

detect_backend() {
    case "${XDG_CURRENT_DESKTOP:-}:${XDG_SESSION_DESKTOP:-}:${DESKTOP_SESSION:-}" in
        *KDE*|*kde*|*PLASMA*|*plasma*) OBK_BACKEND=fcitx ;;
        *) OBK_BACKEND=ibus ;;
    esac
    export OBK_BACKEND
}

detect_arch() {
    case "$(uname -m)" in
        x86_64) OBK_ARCH=x86_64 ;;
        aarch64|arm64) OBK_ARCH=aarch64 ;;
        *)
            die "Unsupported Linux architecture: $(uname -m). Prebuilt releases are currently available for x86_64 and ARM64."
            ;;
    esac
    export OBK_ARCH
}

download() {
    local url="$1" output="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fL --retry 3 --retry-delay 2 "$url" -o "$output"
    elif command -v wget >/dev/null 2>&1; then
        wget -q --show-progress "$url" -O "$output"
    else
        die "curl or wget is required to download Shanti Keyboard."
    fi
}

build_urls() {
    local asset="shanti-keyboard_linux_${OBK_ARCH}_${OBK_BACKEND}.tar.gz"
    if [[ -n "$REQUESTED_VERSION" ]]; then
        RELEASE_VERSION="$REQUESTED_VERSION"
    fi
    if [[ ! "$RELEASE_VERSION" =~ ^[A-Za-z0-9._-]+$ ]]; then
        die "Invalid release tag: $RELEASE_VERSION. Use only letters, numbers, dots, underscores, and hyphens."
    fi
    if [[ "$VERIFY_PROVENANCE" -eq 1 ]]; then
        [[ -n "$REQUESTED_VERSION" && "$RELEASE_VERSION" != "latest" ]] ||
            die "--verify-provenance requires a specific release tag (use --version TAG or set release-tag.txt)."
        command -v gh >/dev/null 2>&1 ||
            die "--verify-provenance requires the GitHub CLI (gh). Install it separately and retry."
    fi
    if [[ -n "$REQUESTED_VERSION" ]]; then
        RELEASE_BASE="https://github.com/$REPOSITORY/releases/download/$RELEASE_VERSION"
    else
        RELEASE_BASE="https://github.com/$REPOSITORY/releases/latest/download"
    fi
    OBK_ASSET="$asset"
    OBK_DOWNLOAD_URL="$RELEASE_BASE/$asset"
    OBK_CHECKSUM_URL="$OBK_DOWNLOAD_URL.sha256"
    export RELEASE_VERSION OBK_ASSET OBK_DOWNLOAD_URL OBK_CHECKSUM_URL
}

validate_release_checksum() {
    local archive="$1" checksum="$2" expected_asset="$3"
    local digest filename extra actual_digest
    local -a lines=()

    mapfile -t lines < "$checksum"
    [[ "${#lines[@]}" -eq 1 ]] ||
        die "The release checksum file must contain exactly one SHA-256 record."
    read -r digest filename extra <<< "${lines[0]}"
    [[ "$digest" =~ ^[[:xdigit:]]{64}$ ]] ||
        die "The release checksum does not contain a valid 64-character SHA-256 digest."
    [[ "$filename" == "$expected_asset" ]] ||
        die "The release checksum names an unexpected file: $filename"
    [[ -z "${extra:-}" ]] ||
        die "The release checksum contains unexpected extra fields."

    actual_digest="$(sha256sum -- "$archive")"
    actual_digest="${actual_digest%% *}"
    [[ "${actual_digest,,}" == "${digest,,}" ]] ||
        die "The downloaded release failed SHA-256 checksum verification."
    log "SHA-256 checksum verified."
}

validate_release_archive_paths() {
    local archive="$1" listing="$2"
    command -v python3 >/dev/null 2>&1 ||
        die "Python 3 is required to safely validate release archive members before extraction. No files were installed."

    python3 - "$archive" "$listing" <<'PY'
import posixpath
import sys
import tarfile

archive_path, listing_path = sys.argv[1:3]
allowed_roots = {"bin", "lib", "libexec", "share"}
seen = set()
symlinks = {}
listing = []
total_size = 0
member_count = 0
max_members = 50000
max_total_size = 2 * 1024 * 1024 * 1024

def fail(message):
    print("Archive validation failed: " + message, file=sys.stderr)
    raise SystemExit(1)

try:
    with tarfile.open(archive_path, mode="r:gz") as archive:
        for member in archive:
            member_count += 1
            if member_count > max_members:
                fail(f"too many archive members (more than {max_members})")
            name = member.name
            if name.startswith("/"):
                fail(f"absolute member path: {name!r}")
            if any(ch in name for ch in "\n\r\t"):
                fail("member names containing control separators are not accepted")
            while name.startswith("./"):
                name = name[2:]
            name = name.rstrip("/")
            if name in ("", "."):
                continue

            parts = name.split("/")
            if any(part in ("", ".", "..") for part in parts):
                fail(f"path traversal or non-canonical path: {member.name!r}")
            if parts[0] not in allowed_roots:
                fail(f"unexpected top-level path: {member.name!r}")
            if name in seen:
                fail(f"duplicate archive path: {name!r}")
            seen.add(name)
            listing.append(name)

            if member.issym():
                target = member.linkname
                if not target or posixpath.isabs(target):
                    fail(f"absolute or empty symbolic-link target: {name!r} -> {target!r}")
                resolved = posixpath.normpath(posixpath.join(posixpath.dirname(name), target))
                if resolved in ("", ".", "..") or resolved.startswith("../") or posixpath.isabs(resolved):
                    fail(f"symbolic link escapes extraction root: {name!r} -> {target!r}")
                if resolved.split("/", 1)[0] not in allowed_roots:
                    fail(f"symbolic link targets an unexpected top-level path: {name!r} -> {target!r}")
                symlinks[name] = resolved
            elif member.islnk():
                fail(f"hard links are not accepted in release archives: {name!r}")
            elif not (member.isfile() or member.isdir()):
                fail(f"special or unsupported archive entry type: {name!r}")

            if member.size < 0:
                fail(f"negative member size: {name!r}")
            total_size += member.size
            if total_size > max_total_size:
                fail("expanded archive exceeds the 2 GiB safety limit")

        for name in seen:
            parent = posixpath.dirname(name)
            while parent not in ("", "."):
                if parent in symlinks:
                    fail(f"archive member is nested under a symbolic link: {name!r} (parent {parent!r})")
                parent = posixpath.dirname(parent)

    with open(listing_path, "w", encoding="utf-8") as stream:
        stream.write("\n".join(sorted(listing)))
        stream.write("\n")
except (tarfile.TarError, OSError, EOFError) as exc:
    fail(f"cannot read archive: {exc}")
PY
}
validate_extracted_symlinks() {
    local root="$1" link target resolved
    [[ -d "$root" && ! -L "$root" ]] ||
        die "Refusing to validate an unexpected extraction root: $root"
    while IFS= read -r -d '' link; do
        target="$(readlink -- "$link")" ||
            die "Could not inspect an extracted symbolic link: $link"
        [[ "$target" != /* ]] ||
            die "Refusing an archive containing an absolute symbolic link: $link -> $target"
        resolved="$(realpath -m -- "$(dirname -- "$link")/$target")" ||
            die "Could not resolve an extracted symbolic link safely: $link"
        [[ "$resolved" == "$root" || "$resolved" == "$root/"* ]] ||
            die "Refusing an archive containing a symbolic link that escapes the extraction root: $link -> $target"
    done < <(find "$root" -type l -print0)
}

download_release() {
    local download_root="$OBK_CACHE/downloads"
    local download_dir archive checksum listing staged_root
    mkdir -p "$download_root"
    assert_no_symlink_components "$download_root"
    download_dir="$(mktemp -d "$download_root/$RELEASE_VERSION.XXXXXXXX")" ||
        die "Could not create a fresh download directory."
    archive="$download_dir/$OBK_ASSET"
    checksum="$archive.sha256"
    listing="$download_dir/archive-list.txt"

    log "Downloading the prebuilt Shanti Keyboard for $OBK_ARCH / $OBK_BACKEND."
    download "$OBK_DOWNLOAD_URL" "$archive" ||
        die "Could not download the Shanti Keyboard release."
    download "$OBK_CHECKSUM_URL" "$checksum" ||
        die "Could not download the release checksum."
    validate_release_checksum "$archive" "$checksum" "$OBK_ASSET"

    if [[ "$VERIFY_PROVENANCE" -eq 1 ]]; then
        log "Verifying signed build provenance for release tag $RELEASE_VERSION."
        gh attestation verify "$archive" \
            --repo "$REPOSITORY" \
            --signer-workflow "$REPOSITORY/.github/workflows/release.yml" \
            --source-ref "refs/tags/$RELEASE_VERSION" ||
            die "Signed build provenance verification failed; the archive will not be installed."
    else
        log "SHA-256 detects corruption; use --verify-provenance to also verify the signed build workflow."
    fi

    validate_release_archive_paths "$archive" "$listing"
    staged_root="$OBK_STAGE$OBK_PREFIX"
    mkdir -p "$staged_root"
    assert_no_symlink_components "$staged_root"
    tar --no-same-owner --no-same-permissions -xzf "$archive" -C "$staged_root"
    validate_extracted_symlinks "$staged_root"

    [[ -x "$staged_root/bin/shanti-gui" ]] ||
        die "The downloaded release is incomplete: the GUI executable is missing."
    [[ -d "$staged_root/lib/shanti" ]] ||
        die "The downloaded release is incomplete: the bundled runtime directory is missing."
    [[ -d "$staged_root/share/shanti-keyboard" ]] ||
        die "The downloaded release is incomplete: application data is missing."
    case "$OBK_BACKEND" in
        ibus)
            [[ -x "$staged_root/libexec/ibus-engine-shanti" && -f "$staged_root/share/ibus/component/shanti.xml" ]] ||
                die "The downloaded IBus release is incomplete."
            ;;
        fcitx)
            [[ -f "$staged_root/share/fcitx5/addon/shanti.conf" && -f "$staged_root/share/fcitx5/inputmethod/shanti.conf" ]] ||
                die "The downloaded Fcitx release is incomplete."
            ;;
    esac

    # Smoke-test the staged executable and its data before changing the current install.
    # Redirect XDG_DATA_HOME into the staging tree so checks cannot initialize real user data.
    XDG_DATA_HOME="$staged_root/share" \
        XDG_CONFIG_HOME="$OBK_STAGE/.smoke-config" \
        XDG_CACHE_HOME="$OBK_STAGE/.smoke-cache" \
        QT_QPA_PLATFORM=offscreen "$staged_root/bin/shanti-gui" --version >/dev/null 2>&1 ||
        die "The downloaded GUI cannot start from staging; the existing installation was left untouched."
    XDG_DATA_HOME="$staged_root/share" \
        XDG_CONFIG_HOME="$OBK_STAGE/.smoke-config" \
        XDG_CACHE_HOME="$OBK_STAGE/.smoke-cache" \
        QT_QPA_PLATFORM=offscreen "$staged_root/bin/shanti-gui" --check-data >/dev/null 2>&1 ||
        die "The downloaded runtime cannot find its staged data files; the existing installation was left untouched."

    export OBK_STAGE
}

main() {
    require_linux
    if [[ -z "$REQUESTED_VERSION" && -f "$RELEASE_TAG_FILE" ]]; then
        REQUESTED_VERSION="$(tr -d '[:space:]' < "$RELEASE_TAG_FILE")"
    fi
    detect_backend
    detect_arch
    prepare_paths
    build_urls
    log "Backend: $OBK_BACKEND"
    log "Architecture: $OBK_ARCH"
    log "Release: $RELEASE_VERSION"
    download_release
    install_shanti
    verify_installation
    log "Shanti Keyboard was installed for the current user."
    log "Need to rebuild from source? Use: bash tools/build.sh"
}

if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    main "$@"
fi
