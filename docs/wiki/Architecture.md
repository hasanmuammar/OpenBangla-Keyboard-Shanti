# Architecture — Portable Linux Build and Runtime

The fork keeps the installer modular.

## Entry points

- tools/install.sh — main installation entry point.
- tools/uninstall.sh — removal entry point.

## Portable installer modules

- tools/portable/common.sh — user-local paths and cache locations.
- tools/portable/environment.sh — backend and isolated build-environment selection.
- tools/portable/build.sh — dependency installation, CMake configuration, compilation and staging.
- tools/portable/bundle-runtime.sh — shared-library closure and runtime module handling.
- tools/portable/install.sh — final user-local installation and input-method registration.
- tools/portable/launch-bundled.sh — runtime launcher for the installed GUI and IBus engine.

## Layout

The normal installation prefix is ~/.local.

Bundled runtime libraries live in ~/.local/lib/openbangla.

The IBus component lives in ~/.local/share/ibus/component/openbangla.xml.

Build and staging state is kept outside the installed tree under the XDG cache directory.

## Backend selection

The installer selects exactly one backend:
- KDE/Plasma -> Fcitx5
- other desktops -> IBus
## External projects

The architecture integrates with [Toolbx](https://github.com/containers/toolbox), [Distrobox](https://github.com/89luca89/distrobox), [Podman](https://github.com/containers/podman), [Docker](https://github.com/moby/moby), [Qt](https://github.com/qt/qtbase), [CMake](https://github.com/Kitware/CMake), [Corrosion](https://github.com/corrosion-rs/corrosion), [IBus](https://github.com/ibus/ibus), [Fcitx5](https://github.com/fcitx/fcitx5) and [Zstandard](https://github.com/facebook/zstd).
