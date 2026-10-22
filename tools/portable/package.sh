#!/usr/bin/env bash

package_shanti() {
    run_builder '
        set -Eeuo pipefail

        install_root="$OBK_STAGE$HOME/.local"
        [[ -d "$install_root" ]] ||
            { echo "Build completed without producing an install tree." >&2; exit 1; }

        arch="$(uname -m)"

        case "$arch" in
            x86_64)
                artifact_arch=x86_64
                ;;
            aarch64|arm64)
                artifact_arch=aarch64
                ;;
            *)
                echo "Unsupported release architecture: $arch" >&2
                exit 1
                ;;
        esac

        artifact_name="shanti-keyboard_linux_${artifact_arch}_${OBK_BACKEND}.tar.gz"
        output_dir="$OBK_WORKSPACE/dist"
        archive_path="$output_dir/$artifact_name"
        checksum_path="$archive_path.sha256"

        [[ "$OBK_WORKSPACE" == /* && "$OBK_WORKSPACE" != "/" ]] || {
            echo "Refusing to package from an invalid workspace path." >&2
            exit 1
        }
        if [[ -L "$output_dir" ]]; then
            echo "Refusing to write through a symlinked output directory: $output_dir" >&2
            exit 1
        fi
        if [[ -e "$output_dir" && ! -d "$output_dir" ]]; then
            echo "Package output path exists and is not a directory: $output_dir" >&2
            exit 1
        fi
        mkdir -p "$output_dir"

        for existing in "$archive_path" "$checksum_path"; do
            if [[ -L "$existing" ]]; then
                echo "Refusing to overwrite a symlink: $existing" >&2
                exit 1
            fi
            if [[ -e "$existing" && ! -f "$existing" ]]; then
                echo "Refusing to overwrite a non-regular file: $existing" >&2
                exit 1
            fi
        done

        if [[ -e "$archive_path" || -e "$checksum_path" ]]; then
            if [[ ! -t 0 ]]; then
                echo "Package output already exists; refusing to overwrite without an interactive choice:" >&2
                [[ ! -e "$archive_path" ]] || printf "  %s\n" "$archive_path" >&2
                [[ ! -e "$checksum_path" ]] || printf "  %s\n" "$checksum_path" >&2
                exit 1
            fi
            printf "The following generated files already exist and would be replaced:\n"
            [[ ! -e "$archive_path" ]] || printf "  %s\n" "$archive_path"
            [[ ! -e "$checksum_path" ]] || printf "  %s\n" "$checksum_path"
            read -r -p "Replace only these two generated files? [y/N] " reply
            [[ "$reply" =~ ^[Yy]([Ee][Ss])?$ ]] || {
                echo "Packaging cancelled. Existing files were preserved." >&2
                exit 1
            }
        fi

        component="$install_root/share/ibus/component/shanti.xml"
        if [[ -f "$component" ]]; then
            sed -i -E \
                -e "s#<exec>[^<]*</exec>#<exec>@OBK_PREFIX@/libexec/ibus-engine-shanti --ibus</exec>#" \
                -e "s#<icon>[^<]*</icon>#<icon>@OBK_DATA_HOME@/shanti-keyboard/icons/Shanti-Keyboard.png</icon>#" \
                "$component"
        fi

        desktop="$install_root/share/applications/shanti-keyboard.desktop"
        if [[ -f "$desktop" ]]; then
            sed -i -E "s#^Exec=.*\$#Exec=\"@OBK_PREFIX@/bin/shanti-gui\" %f#" "$desktop"
        fi

        # Ship the user-facing installer and uninstaller inside the archive so a
        # downloaded release can be installed, repaired or removed without a
        # source checkout. The tools/ layout and release-tag.txt are mirrored so
        # that install.sh keeps resolving its libraries and release tag.
        data_root="$install_root/share/shanti-keyboard"
        release_tools_root="$data_root/tools"
        [[ -d "$OBK_WORKSPACE/tools/portable" ]] || {
            echo "Refusing to package without the portable tools directory." >&2
            exit 1
        }
        mkdir -p "$release_tools_root/portable"
        cp -a "$OBK_WORKSPACE/tools/install.sh" "$release_tools_root/install.sh"
        cp -a "$OBK_WORKSPACE/tools/uninstall.sh" "$release_tools_root/uninstall.sh"
        cp -a "$OBK_WORKSPACE"/tools/portable/*.sh "$release_tools_root/portable/"
        cp -a "$OBK_WORKSPACE/release-tag.txt" "$data_root/release-tag.txt"
        chmod 0755 "$release_tools_root/install.sh" "$release_tools_root/uninstall.sh"

        tar -C "$install_root" -czf "$output_dir/$artifact_name" .

        (
            cd "$output_dir"
            sha256sum "$artifact_name" > "$artifact_name.sha256"
        )

        printf "Created %s\n" "$output_dir/$artifact_name"
    '
}
