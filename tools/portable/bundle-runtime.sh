#!/usr/bin/env bash
set -Eeuo pipefail

[[ $# -eq 4 ]] || {
    echo "Usage: bundle-runtime.sh RUNTIME_LIB_DIR RUNTIME_PLUGIN_DIR STAGE_DIR QT_LIB_DIR" >&2
    exit 2
}

runtime_lib_dir="$1"
runtime_plugin_dir="$2"
stage_dir="$3"
qt_lib_dir="$4"

[[ "$stage_dir" == /* && "$stage_dir" != "/" && -d "$stage_dir" && ! -L "$stage_dir" ]] || {
    echo "Refusing to bundle runtime into an invalid staging directory: $stage_dir" >&2
    exit 1
}
stage_root="$(realpath -e -- "$stage_dir")" || {
    echo "Could not resolve the staging directory: $stage_dir" >&2
    exit 1
}
runtime_lib_dir="$(realpath -m -- "$runtime_lib_dir")" || {
    echo "Could not resolve the runtime library directory." >&2
    exit 1
}
runtime_plugin_dir="$(realpath -m -- "$runtime_plugin_dir")" || {
    echo "Could not resolve the runtime plugin directory." >&2
    exit 1
}

[[ "$runtime_lib_dir" == "$stage_root/"* && ! -L "$runtime_lib_dir" ]] || {
    echo "Refusing to remove or replace files outside the staged runtime: $runtime_lib_dir" >&2
    exit 1
}
[[ "$runtime_plugin_dir" == "$runtime_lib_dir/"* && ! -L "$runtime_plugin_dir" ]] || {
    echo "Refusing to remove or replace files outside the staged runtime plugins: $runtime_plugin_dir" >&2
    exit 1
}

host_library() {
    case "$1" in
        libc.so*|libm.so*|libpthread.so*|libdl.so*|librt.so*|libresolv.so*|libcrypt.so*|libutil.so*|libanl.so*|libnss_*.so*|libgcc_s.so*|libstdc++.so*|ld-linux*.so*|ld-musl-*.so*|libGL.so*|libEGL.so*|libGLX.so*|libX11.so*|libX11-xcb.so*|libxcb*.so*|libXau.so*|libXdmcp.so*|libXext.so*|libXfixes.so*|libXi.so*|libXrender.so*|libXrandr.so*|libXcursor.so*|libXdamage.so*|libXcomposite.so*|libwayland*.so*|libdecor*.so*|libdrm.so*|libinput.so*|libudev.so*|libfontconfig.so*|libfreetype.so*|libexpat.so*|libdbus-1.so*)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

queue=()
declare -A seen=()

bundle_libproxy_backend() {
    local libproxy
    local module
    local module_dir
    local target

    command -v ldconfig >/dev/null 2>&1 || return 0

    libproxy="$(ldconfig -p 2>/dev/null | awk '$1 == "libproxy.so.1" {print $NF; exit}')"
    [[ -f "$libproxy" ]] || return 0

    module_dir="$(dirname "$(readlink -f "$libproxy")")/libproxy"
    for module in "$module_dir"/libpxbackend-1.0.so*; do
        [[ -e "$module" ]] || continue

        target="$runtime_lib_dir/libpxbackend-1.0.so"
        if [[ -L "$target" ]]; then
            rm -f -- "$target"
        fi

        cp -a "$(readlink -f "$module")" "$target"
        enqueue "$target"
    done
}


enqueue() {
    local library="$1"
    [[ -f "$library" ]] || return 0
    [[ "${seen[$library]:-0}" -eq 1 ]] && return 0
    seen["$library"]=1
    queue+=("$library")
}

enqueue "$stage_dir$HOME/.local/bin/shanti-gui"
enqueue "$stage_dir$HOME/.local/libexec/ibus-engine-shanti"
for library in "$runtime_lib_dir"/*.so* "$runtime_plugin_dir"/platforms/*.so*; do
    enqueue "$library"
done

while ((${#queue[@]})); do
    library="${queue[0]}"
    queue=("${queue[@]:1}")

    while IFS= read -r dependency_line; do
        dependency="${dependency_line##*=> }"
        [[ "$dependency" == /* ]] || continue
        dependency="${dependency%% *}"
        [[ -f "$dependency" ]] || continue

        dependency_name="${dependency##*/}"
        host_library "$dependency_name" && continue

        resolved_dependency="$(readlink -f "$dependency")"
        [[ -f "$resolved_dependency" ]] || continue

        target="$runtime_lib_dir/$dependency_name"
        if [[ -L "$target" ]]; then
            rm -f -- "$target"
        fi
        if [[ ! -e "$target" ]]; then
            cp -a "$resolved_dependency" "$target"
        fi

        enqueue "$target"
    done < <(LD_LIBRARY_PATH="$runtime_lib_dir:$qt_lib_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ldd "$library" 2>/dev/null)
done
