# Architecture — Shanti Linux Fork

Shanti is a Linux-focused fork of [OpenBangla Keyboard](https://github.com/OpenBangla/OpenBangla-Keyboard). It has two distinct delivery paths: a portable user-local installer for IBus or Fcitx5, and an experimental IBus-only Flatpak. They have different runtime and host-integration models; instructions for one should not be applied to the other.

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

The default prefix is `~/.local`. Bundled runtime libraries are placed under `~/.local/lib/openbangla`; build and staging state is kept under the XDG cache directory. The portable installer selects one backend per installation: Fcitx5 on KDE/Plasma, IBus elsewhere.

For IBus, the portable installation registers a user component and configures the user service environment so the host IBus daemon can discover it. For Fcitx5, it installs the module and addon metadata and may update the user's Fcitx5 profile.

### IBus-only Flatpak

The developer-built Flatpak manifest is `tools/flatpak/shanti-ibus.yml`, with app ID `org.openbangla.Shanti.IBus`. It includes the GUI and IBus engine, not the Fcitx5 engine. Its GUI settings and layout data live in the Flatpak's sandboxed data/config locations.

On GNOME, the app can offer to register the Flatpak engine with the host IBus session. The app Settings panel shows registration/service status and provides **Set up / repair** and **Remove integration** actions. Setup writes three app-managed, user-scoped files: an IBus component descriptor, a launcher that invokes the Flatpak engine, and a systemd user-service drop-in for `org.freedesktop.IBus.session.GNOME.service`. It then reloads the user systemd manager and restarts that service. The tray menu also exposes **GNOME IBus Integration…** as a shortcut.

The integration is deliberately limited to GNOME and the IBus Flatpak app ID. It does not install system-wide files. It checks for existing drop-ins that change `IBUS_COMPONENT_PATH` or `ExecStart` and refuses to proceed when it detects a potential conflict. The app checks registration/service health while open; it does not continuously prove that the engine process is running or that text input works.

The Flatpak manifest grants access to the user IBus component directory, helper directory, GNOME IBus service drop-in directory and user systemd D-Bus service. Setup and input-source discovery have been confirmed working in a GNOME session. Conflict handling, recovery, removal, persistence and actual Bengali input still need broader testing before assuming the same behavior on other accounts or desktops.

## Build and validation boundaries

CI builds portable packages for x86_64 and aarch64 and tests IBus/Fcitx5 package structure. It also builds the IBus Flatpak for x86_64 and aarch64. Passing build/package workflows do not prove real desktop input, setup recovery, reboot persistence or cross-distribution compatibility.

No Flatpak OSTree repository is configured, and the repository currently has no published GitHub release. Therefore, the portable installer cannot yet be assumed to download a published release. See [Installation](Installation.md), [Development](Development.md), and the [Flatpak guide](../../tools/flatpak/README.md).

## External projects

The project integrates with [IBus](https://github.com/ibus/ibus), [Fcitx5](https://github.com/fcitx/fcitx5), [Qt](https://github.com/qt/qtbase), [CMake](https://github.com/Kitware/CMake), [Corrosion](https://github.com/corrosion-rs/corrosion), and [Zstandard](https://github.com/facebook/zstd). Toolbx, Distrobox, Podman and Docker are supported options for isolated source builds.
