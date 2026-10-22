# Architecture — Shanti Linux Fork

Shanti Keyboard is a Linux-focused fork of [OpenBangla Keyboard](https://github.com/OpenBangla/OpenBangla-Keyboard). It has three distinct delivery paths: a portable user-local installer for IBus or Fcitx5, an IBus engine Flatpak (`org.shanti.ShantiIbus`), and a self-contained Fcitx5 app Flatpak (`org.shanti.ShantiFcitx5`). They have different runtime and host-integration models; instructions for one should not be applied to the other.

## Delivery paths

### Portable user-local installation

- `tools/install.sh` downloads and validates a published release archive, then installs it for the current user.
- `tools/build.sh` is the source-build and release-packaging entry point.
- `tools/portable/common.sh` defines paths and shared helpers.
- `tools/portable/environment.sh` selects the backend and build environment.
- `tools/portable/build.sh` configures, compiles and stages the application.
- `tools/portable/bundle-runtime.sh` collects runtime libraries and required modules.
- `tools/portable/install.sh` installs files and registers desktop/input-method resources.
- `tools/uninstall.sh` removes the user-local installation; user data is preserved unless the corresponding purge option is requested.

The default prefix is `~/.local`. Bundled runtime libraries are placed under `~/.local/lib/shanti`; build and staging state is kept under the XDG cache directory. The portable installer selects one backend per installation: Fcitx5 on KDE/Plasma, IBus elsewhere.

For IBus, the portable installation registers a user component and configures the user service environment so the host IBus daemon can discover it. For Fcitx5, it installs the module and addon metadata and may update the user's Fcitx5 profile.

### IBus-only Flatpak

The developer-built Flatpak manifest is `tools/flatpak/shanti-ibus.yml`, with app ID `org.shanti.ShantiIbus`. It includes the GUI and IBus engine, not the Fcitx5 engine. Its GUI settings and layout data live in the Flatpak's sandboxed data/config locations.

### Self-contained Fcitx5 Flatpak

The manifest is `tools/flatpak/fcitx-app.yml`, with app ID `org.shanti.ShantiFcitx5`. It builds the whole Fcitx5 stack **itself** on the same Qt5 runtime as the IBus app (`org.kde.Platform//5.15-25.08`): pinned modules `xcb-imdkit` 1.0.9, `libuv` 1.53.0, `json-c` 0.16, `fcitx5` 5.1.14 (newest fcitx5 that still supplies `stringutils.h` transitively via `standardpath.h`, which `fcitx5-configtool` 5.1.x assumes; 5.1.15+ requires `PlasmaWaylandProtocols` >= 1.20, absent from the Qt5 SDK), `fcitx5-qt` 5.1.16 (`-DENABLE_QT6=Off`) and `fcitx5-configtool` 5.1.6 (`-DUSE_QT6=Off` — the last configtool line with Qt5/KF5 support), followed by the single Shanti module with the full frontend (`-DBUILD_FRONTEND=ON`). So both bundles ship the **same `shanti-gui`** from one source tree; only the engine backend differs (`ibus-engine-shanti` vs `shanti.so`), and both read the same `~/.config/Shanti/Shanti.conf` store the GUI writes. The self-build was forced by the runtime: Flathub's `org.fcitx.Fcitx5` moved to the Qt6-only 6.11 platform, where Qt5 code cannot run, and there is no Qt5 in that runtime to borrow. The app owns Fcitx5's own session bus, so it must not run alongside Flathub's `org.fcitx.Fcitx5` app. Login startup is via an XDG autostart entry (neither this bundle nor Flathub's Fcitx5 ships a systemd user unit); the settings GUI opens from its desktop entry.

On GNOME, the app can offer to register the Flatpak engine with the host IBus session. The app Settings panel shows registration/service status and provides **Set up / repair** and **Remove integration** actions. Setup writes three app-managed, user-scoped files: an IBus component descriptor, a launcher that invokes the Flatpak engine, and a systemd user-service drop-in for `org.freedesktop.IBus.session.GNOME.service`. It then reloads the user systemd manager and restarts that service. The tray menu also exposes **GNOME IBus Integration…** as a shortcut.

The integration is deliberately limited to GNOME and the IBus Flatpak app ID. It does not install system-wide files. It checks for existing drop-ins that change `IBUS_COMPONENT_PATH` or `ExecStart` and refuses to overwrite them. When a conflict blocks setup, the app offers a confirmed **Back up and continue** action that preserves the selected override before installing the current integration. The app checks registration/service health while open; it does not continuously prove that the engine process is running or that text input works.

The Flatpak manifest grants access to the user IBus component directory, helper directory, GNOME IBus service drop-in directory and user systemd D-Bus service. Setup and input-source discovery have been confirmed working in a GNOME session. Conflict handling, recovery, removal, persistence and actual Bengali input still need broader testing before assuming the same behavior on other accounts or desktops.

## Build and validation boundaries

CI builds portable packages for x86_64 and aarch64 and tests IBus/Fcitx5 package structure. Every push also builds the IBus and Fcitx5 app Flatpaks for both architectures as downloadable CI artifacts; tagged alpha releases attach direct-download Flatpak bundles, with checksums and provenance attestations. Passing build/package workflows do not prove real desktop input, setup recovery, reboot persistence or cross-distribution compatibility.

Both Flatpaks are distributed as downloadable bundles rather than through an OSTree remote, and releases are Flatpak-only — the portable archives are no longer published as release assets (older pre-releases still carry them). Flatpak bundles are listed on the [GitHub Releases](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/releases) page. See [Installation](Installation.md), [Development](Development.md), and the [Flatpak guide](../../tools/flatpak/README.md).

## External projects

The project integrates with [IBus](https://github.com/ibus/ibus), [Fcitx5](https://github.com/fcitx/fcitx5), [Qt](https://github.com/qt/qtbase), [CMake](https://github.com/Kitware/CMake), [Corrosion](https://github.com/corrosion-rs/corrosion), and [Zstandard](https://github.com/facebook/zstd). Toolbx, Distrobox, Podman and Docker are supported options for isolated source builds.
