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

## Runtime and resource map (initial assessment)

This section records source-level findings for the Flatpak feasibility assessment. It describes the existing portable build; it is not a Flatpak design or a claim that sandboxed input-method integration works.

### Executables and backends

- `openbangla-gui` is the Qt5 Widgets application, built from `src/frontend` and installed to `bin/`.
- The IBus build produces `ibus-engine-openbangla`, installed to `libexec/`. Its generated component descriptor is installed to `share/ibus/component/openbangla.xml`; the descriptor invokes the engine with `--ibus`.
- The Fcitx5 build produces the `openbangla` module and installs it with Fcitx5 addon and input-method metadata. The build enables exactly one backend at a time.
- The portable package therefore represents separate IBus and Fcitx5 builds; the installer selects one based on the detected desktop session.

### Bundled runtime

- The portable build copies Qt5 Core, Gui, Widgets, Network and Svg libraries, plus the Qt XCB platform plugin, into `~/.local/lib/openbangla`.
- `bundle-runtime.sh` walks ELF dependencies with `ldd` and copies selected non-host libraries into that directory. Its explicit host-library exclusions include libc/runtime, graphics, X11/Wayland, font, udev, and D-Bus libraries.
- `launch-bundled.sh` adds `../lib/openbangla` to `LD_LIBRARY_PATH` and executes the corresponding `.bin` binary. The build also writes `bin/qt.conf` to point Qt at the bundled plugin directory.
- This is the current portable-runtime strategy. A Flatpak build should be assessed against its selected runtime and SDK rather than assuming this host-library exclusion list is appropriate inside a sandbox.

### Application resources and mutable data

- CMake defines the application data directory as `share/openbangla-keyboard` under the install prefix.
- On Linux, `FileSystem.cpp` first looks for `../share/openbangla-keyboard` relative to the executable; if absent, it falls back to the user's generic data directory at `openbangla-keyboard`.
- The resource tree includes layouts, data files, and icons. Runtime code references layout JSON files and data such as `autocorrect.json`, `dictionary.json`, `suffix.json`, and `regex.json`.
- The portable installer relocates application resources to `$XDG_DATA_HOME/openbangla-keyboard` (normally `~/.local/share/openbangla-keyboard`). Its merge logic preserves existing custom layouts and autocorrect data; it backs up existing dictionary/suffix/regex files before replacing shipped versions.
- GUI preferences use Qt `QSettings("OpenBangla", "Keyboard")` on Linux. The exact on-disk settings location depends on Qt's platform configuration and XDG environment and should be verified in a runtime test, not inferred solely from the constructor.

### Flatpak implementation and host integration

- The experimental full GUI and IBus Flatpak manifest is `tools/flatpak/openbangla.yml`. A complete package was built and run on Bluefin/GNOME under the provisional ID `io.github.openbangla.keyboard`; the developer confirmed launch, AppIndicator display, layout changes, and settings changes.
- The selected release identity is `io.github.hasanmuammar.Shanti`, displayed as **OpenBangla Keyboard Shanti**. The manifest now rewrites the desktop entry, icons, and AppStream metadata for that identity. A rebuild with those changes is still pending: the latest build attempt was blocked because this environment does not permit Bubblewrap to create a `NETLINK_ROUTE` socket. This is an environment limitation, not a successful verification of the changed package.
- Host IBus discovery and Bengali typing have also been verified through the experimental user-local bridge on Fedora Workstation 44/GNOME 50.5, including a clean test account. These results do not establish support on other desktops or distributions. Fcitx5 integration remains unverified.
- The portable installer also handles XDG desktop/icon/metainfo resources and backend-specific integration files. A sandboxed package must not assume that placing these files inside its private app directory automatically registers the engine with the host.
- Still to verify: published repository/signing and install-update-remove lifecycle; reproducible build of the Shanti-ID package; compatibility outside the tested environments; and Fcitx5 integration.

### Evidence boundary

The portable-build details above come from source review. Flatpak build instructions and current status are in the [Shanti Flatpak guide](../../tools/flatpak/README.md); host bridge behavior and platform limits are in the [experimental IBus integration report](../../tools/flatpak/IBUS-HOST-INTEGRATION.md). The release-ID transformation still needs a successful rebuild, and the environment-specific results do not establish broad platform support.

