# Installation — OpenBangla Keyboard for Linux

The normal installer is designed to use prebuilt Linux releases. It does not require a compiler, CMake, Rust, Qt development packages, or a build container. **However, no Shanti GitHub Release is currently published**, so the normal download-and-install path is not yet available from release assets. Use the source-build workflow below for local use, or the developer-built IBus Flatpak described in the [Flatpak guide](../../tools/flatpak/README.md).

Supported architectures:

- x86_64
- ARM64 (aarch64)

KDE Plasma automatically selects Fcitx5. Other desktop environments select IBus.

## Install

The installer needs Bash, Python 3, GNU tar, `sha256sum`, `realpath`, `find`, `mktemp`, and either `curl` or `wget`. It installs for the current user and does not require compiler, CMake, Rust, Qt development packages, or a build container. Since no Shanti release is currently published, use the source-build instructions below for now; running the installer now will fail because its release archive and checksum do not exist yet.

The installer uses the release tag in `release-tag.txt`. To select a different published release:

```bash
bash tools/install.sh --version TAG
```

Replace `TAG` with an actual published Shanti release tag. This option works only after that tag has a GitHub Release with the expected archive and checksum assets. The installer downloads the archive and its SHA-256 checksum, validates that the checksum names the expected archive and contains exactly one digest, and compares the digest with the downloaded bytes. It also rejects archive paths outside the expected `bin/`, `lib/`, `libexec/`, and `share/` trees, duplicate archive paths, absolute symbolic links, and extracted symbolic links that escape the staging directory. Installation only proceeds after the staged payload passes backend-specific completeness checks.

**Checksum limitation:** the checksum is published alongside the archive, so SHA-256 detects corruption or a mismatched file but does not independently authenticate the publisher if both assets are replaced.

## Verify signed release provenance

New releases are configured to publish GitHub/Sigstore build-provenance attestations for the archive and checksum. To require attestation verification before installing:

```bash
bash tools/install.sh --version TAG --verify-provenance
```

This option requires the GitHub CLI (`gh`), a versioned release tag, and online access to GitHub. Verification is restricted to this repository's release workflow and the selected tag; installation stops if verification fails. It is optional so the normal installer does not gain a mandatory GitHub CLI dependency.

Attestations are only available for releases produced by a workflow version that includes the attestation step. Previously published releases are not retroactively attested.

## Build from source

Source compilation is a separate developer workflow:

```bash
bash tools/build.sh
```

The source build can use Toolbx, Distrobox, Podman, or Docker to isolate development dependencies. Release builds are produced for both x86_64 and ARM64.

## IBus

On GNOME and other non-KDE/Plasma desktops, the installer installs the IBus engine, registers the component under `XDG_DATA_HOME/ibus/component`, rebuilds the IBus cache, and persists the user-local `IBUS_COMPONENT_PATH`.

Verify registration with:

```bash
ibus list-engine | grep -i -A3 -B2 openbangla
```

## Fcitx5

On KDE/Plasma, the installer selects the Fcitx5 backend and installs its module and addon metadata under the user-local prefix.

## Related projects

The installer and Linux integration use or integrate with [Toolbx](https://github.com/containers/toolbox), [Distrobox](https://github.com/89luca89/distrobox), [Podman](https://github.com/containers/podman), [Docker](https://github.com/moby/moby), [IBus](https://github.com/ibus/ibus), [Fcitx5](https://github.com/fcitx/fcitx5), [Qt](https://github.com/qt/qtbase) and [CMake](https://github.com/Kitware/CMake).
