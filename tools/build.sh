#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/portable/common.sh"
source "$SCRIPT_DIR/portable/environment.sh"
source "$SCRIPT_DIR/portable/build.sh"
source "$SCRIPT_DIR/portable/package.sh"
source "$SCRIPT_DIR/portable/install.sh"

ACTION=install
REQUESTED_BACKEND=""
NONINTERACTIVE="${SHANTI_NONINTERACTIVE:-0}"

usage() {
    cat <<'EOF'
Usage: tools/build.sh [options]

Build Shanti Keyboard from source.

Options:
  --backend BACKEND   Build IBus or Fcitx5. Default: detect from desktop.
  --package           Create a prebuilt Linux release archive in ./dist
                      instead of installing the build locally.
  -y, --yes           Do not ask before creating a temporary build environment.
  -h, --help          Show this help.
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --backend)
            [[ $# -ge 2 ]] || die "--backend requires ibus or fcitx."
            REQUESTED_BACKEND="$2"
            shift 2
            ;;
        --package)
            ACTION=package
            shift
            ;;
        -y|--yes)
            NONINTERACTIVE=1
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

case "$REQUESTED_BACKEND" in
    ""|ibus|fcitx)
        ;;
    *)
        die "Unsupported backend: $REQUESTED_BACKEND. Use ibus or fcitx."
        ;;
esac

main() {
    require_linux
    detect_desktop_backend

    if [[ -n "$REQUESTED_BACKEND" ]]; then
        OBK_BACKEND="$REQUESTED_BACKEND"
        export OBK_BACKEND
    fi

    select_builder
    prepare_paths

    if [[ "$ACTION" == package ]]; then
        NONINTERACTIVE=1
    fi

    log "Backend: $OBK_BACKEND"
    log "Build environment: $OBK_BUILDER"
    log "Build mode: $ACTION"

    if [[ "$NONINTERACTIVE" == 1 ]]; then
        if [[ "$OBK_BUILDER" != host ]] && ! builder_exists; then
            if [[ "${OBK_NEEDS_PODMAN_BOOTSTRAP:-0}" == 1 ]]; then
                die "Non-interactive mode will not install Podman or change host packages. Install a supported builder manually, then rerun."
            fi
            OBK_CLEANUP_BUILDER_ON_SUCCESS=1
            export OBK_CLEANUP_BUILDER_ON_SUCCESS
        fi
    else
        confirm_temporary_build
    fi

    build_shanti

    if [[ "$ACTION" == package ]]; then
        package_shanti
        cleanup_builder
        log "Prebuilt package was created in ./dist/."
    else
        install_shanti
        verify_installation
        cleanup_builder
        log "Shanti Keyboard was built and installed for the current user."
    fi
}

main "$@"
