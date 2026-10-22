#!/usr/bin/env bash

detect_desktop_backend() {
    case "${XDG_CURRENT_DESKTOP:-}:${XDG_SESSION_DESKTOP:-}:${DESKTOP_SESSION:-}" in
        *KDE*|*kde*|*PLASMA*|*plasma*)
            OBK_BACKEND=fcitx
            ;;
        *)
            OBK_BACKEND=ibus
            ;;
    esac
    export OBK_BACKEND
}


run_host_root() {
    if [[ "$(id -u)" -eq 0 ]]; then
        "$@"
    else
        command -v sudo >/dev/null 2>&1 ||
            die "sudo is required to install the host build environment."
        sudo "$@"
    fi
}

bootstrap_podman() {
    if command -v apt-get >/dev/null 2>&1; then
        log "No isolated build environment found. Installing Podman with apt."
        run_host_root apt-get update
        run_host_root env DEBIAN_FRONTEND=noninteractive apt-get install -y podman
    elif command -v dnf >/dev/null 2>&1; then
        log "No isolated build environment found. Installing Podman with dnf."
        run_host_root dnf install -y podman
    elif command -v pacman >/dev/null 2>&1; then
        die "Podman is not installed. On Arch-based systems, install it through your normal system update workflow and rerun; this script will not run pacman -Sy because that can cause a partial-upgrade state."
    elif command -v zypper >/dev/null 2>&1; then
        log "No isolated build environment found. Installing Podman with zypper."
        run_host_root zypper --non-interactive install podman
    elif command -v apk >/dev/null 2>&1; then
        log "No isolated build environment found. Installing Podman with apk."
        run_host_root apk add podman
    else
        die "No supported build environment or host package manager found. Install Toolbx, Distrobox, Podman, or Docker manually and rerun this installer."
    fi

    command -v podman >/dev/null 2>&1 ||
        die "Podman installation completed but the podman command is still unavailable."

    log "Podman is now available."
}
select_builder() {
    OBK_NEEDS_PODMAN_BOOTSTRAP=0
    OBK_CLEANUP_BUILDER_ON_SUCCESS=0
    OBK_BUILDER_CREATED=0

    if [[ "${SHANTI_BUILD_USE_HOST:-0}" == 1 ]]; then
        OBK_BUILDER=host
    elif command -v toolbox >/dev/null 2>&1; then
        OBK_BUILDER=toolbox
    elif command -v distrobox >/dev/null 2>&1; then
        OBK_BUILDER=distrobox
    elif command -v podman >/dev/null 2>&1; then
        OBK_BUILDER=podman
    elif command -v docker >/dev/null 2>&1; then
        OBK_BUILDER=docker
    else
        OBK_BUILDER=podman
        OBK_NEEDS_PODMAN_BOOTSTRAP=1
    fi

    export OBK_BUILDER OBK_NEEDS_PODMAN_BOOTSTRAP OBK_CLEANUP_BUILDER_ON_SUCCESS OBK_BUILDER_CREATED
}

builder_exists() {
    case "$OBK_BUILDER" in
        toolbox)
            toolbox list 2>/dev/null | grep -q 'shanti-builder'
            ;;
        distrobox)
            distrobox list 2>/dev/null | grep -q 'shanti-builder'
            ;;
        podman)
            podman container exists shanti-builder 2>/dev/null
            ;;
        docker)
            return 1
            ;;
        host)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

confirm_temporary_build() {
    local answer

    if [[ "${OBK_NEEDS_PODMAN_BOOTSTRAP:-0}" == 1 ]] && command -v pacman >/dev/null 2>&1; then
        die "Automatic Podman installation is disabled on Arch-based systems to avoid partial upgrades. Install Podman through your normal system update workflow and rerun."
    fi

    if [[ "$OBK_BUILDER" == docker ]] || ! builder_exists; then
        printf '\n'
        printf '%s\n' "Shanti needs some extra files to build the program."
        printf '%s\n' "This may temporarily use about 1 GB or more of disk space."
        printf '%s\n' "The files are kept inside a temporary build container, not added to your normal installation."
        printf '%s\n' "After Shanti is installed successfully, temporary files may be cleaned up and that space will be freed."
        if [[ "${OBK_NEEDS_PODMAN_BOOTSTRAP:-0}" == 1 ]]; then
            printf '%s\n' "Podman is not installed; continuing will install it as a host system package using the detected package manager."
        fi
        printf '%s ' "Continue with the installation? [Y/n]"

        read -r answer
        case "${answer:-Y}" in
            [Yy]|[Yy][Ee][Ss])
                ;;
            *)
                die "Installation cancelled."
                ;;
        esac

        OBK_CLEANUP_BUILDER_ON_SUCCESS=1
        export OBK_CLEANUP_BUILDER_ON_SUCCESS

        if [[ "${OBK_NEEDS_PODMAN_BOOTSTRAP:-0}" == 1 ]]; then
            if command -v pacman >/dev/null 2>&1; then
                die "Automatic Podman installation is disabled on Arch-based systems to avoid partial upgrades. Install Podman through your normal system update workflow and rerun."
            fi
            bootstrap_podman
        fi
    fi
}

ensure_builder() {
    local created=0

    case "$OBK_BUILDER" in
        toolbox)
            if ! toolbox list 2>/dev/null | grep -q 'shanti-builder'; then
                toolbox create --container shanti-builder
                created=1
            fi
            ;;
        distrobox)
            if ! distrobox list 2>/dev/null | grep -q 'shanti-builder'; then
                distrobox create --name shanti-builder --image ubuntu:24.04
                created=1
            fi
            ;;
        podman)
            local builder_image="debian:13-slim"
            local current_image=""

            if podman container exists shanti-builder 2>/dev/null; then
                current_image="$(podman inspect -f '{{.Config.Image}}' shanti-builder 2>/dev/null || true)"

                if [[ "$current_image" != "$builder_image" ]]; then
                    printf 'Existing container shanti-builder uses image %s; expected %s.\n' "$current_image" "$builder_image" >&2
                    if [[ ! -t 0 ]]; then
                        echo "Refusing to remove the existing container non-interactively. Review it and rerun interactively to choose whether to replace it." >&2
                        exit 1
                    fi
                    read -r -p 'Remove this existing builder container and recreate it? [y/N] ' reply
                    [[ "$reply" =~ ^[Yy]([Ee][Ss])?$ ]] || {
                        echo "Build cancelled. The existing container was preserved." >&2
                        exit 1
                    }
                    podman rm -f shanti-builder >/dev/null
                fi
            fi

            if ! podman container exists shanti-builder 2>/dev/null; then
                created=1
                podman create --pull=missing                     --name shanti-builder                     -v "$OBK_WORKSPACE:/workspace:rw"                     -v "$HOME:$HOME:rw"                     -v "$OBK_CACHE:/cache:rw"                     -v "$OBK_STAGE:/stage:rw"                     -e HOME="$HOME"                     "$builder_image"                     sleep infinity
            fi

            if [[ "$(podman inspect -f '{{.State.Running}}' shanti-builder 2>/dev/null)" != true ]]; then
                    podman start shanti-builder >/dev/null
            fi
            ;;
    esac

    if [[ "$created" -eq 1 ]]; then
        OBK_BUILDER_CREATED=1
        export OBK_BUILDER_CREATED
    fi
}

run_builder() {
    local command="$1"
    local env_prefix

    ensure_builder

    printf -v env_prefix         'export OBK_WORKSPACE=%q OBK_BUILD=%q OBK_STAGE=%q OBK_CACHE=%q OBK_BACKEND=%q OBK_JOBS=%q; '         "$OBK_WORKSPACE" "$OBK_BUILD" "$OBK_STAGE" "$OBK_CACHE" "$OBK_BACKEND" "$OBK_JOBS"

    case "$OBK_BUILDER" in
        host)
            bash -lc "$env_prefix$command"
            ;;
        toolbox)
            toolbox run --container shanti-builder bash -lc "$env_prefix$command"
            ;;
        distrobox)
            distrobox enter --name shanti-builder -- bash -lc "$env_prefix$command"
            ;;
        podman)
            podman exec                 -e OBK_WORKSPACE=/workspace                 -e OBK_BUILD=/cache/build                 -e OBK_STAGE=/stage                 -e OBK_CACHE=/cache                 -e OBK_BACKEND="$OBK_BACKEND"                 -e OBK_JOBS="$OBK_JOBS"                 shanti-builder                 bash -lc "$env_prefix$command"
            ;;
        docker)
            docker run --rm                 -v "$OBK_WORKSPACE:/workspace:rw"                 -v "$HOME:$HOME:rw"                 -v "$OBK_CACHE:/cache:rw"                 -v "$OBK_STAGE:/stage:rw"                 -e HOME="$HOME"                 -e OBK_WORKSPACE=/workspace                 -e OBK_BUILD=/cache/build                 -e OBK_STAGE=/stage                 -e OBK_CACHE=/cache                 -e OBK_BACKEND="$OBK_BACKEND"                 -e OBK_JOBS="$OBK_JOBS"                 ubuntu:24.04 bash -lc "$command"
            ;;
        *)
            die "Unknown build environment: $OBK_BUILDER"
            ;;
    esac

    if [[ "$OBK_BUILDER" == docker ]]; then
        docker run --rm             -v "$OBK_CACHE:/cache:rw"             -v "$OBK_STAGE:/stage:rw"             ubuntu:24.04             chown -R "$(id -u):$(id -g)" /cache/build /stage
    fi
}


cleanup_builder() {
    [[ "${OBK_CLEANUP_BUILDER_ON_SUCCESS:-0}" == 1 ]] || return 0
    [[ "${OBK_BUILDER_CREATED:-0}" == 1 ]] || return 0

    log "Removing the temporary build environment."

    case "$OBK_BUILDER" in
        podman)
            podman rm -f shanti-builder >/dev/null 2>&1 || true
            ;;
        toolbox)
            toolbox rm -f shanti-builder >/dev/null 2>&1 || true
            ;;
        distrobox)
            distrobox rm -f shanti-builder >/dev/null 2>&1 || true
            ;;
    esac

    OBK_BUILDER_CREATED=0
    export OBK_BUILDER_CREATED
}
