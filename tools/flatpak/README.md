# Shanti IBus Flatpak

The manifest `shanti-ibus.yml` builds the Qt keyboard GUI and IBus engine as `org.openbangla.Shanti.IBus`, displayed as **OpenBangla Keyboard Shanti (IBus)**. It does not include the Fcitx5 engine. The portable user-local installer is a separate delivery path and is not modified by this Flatpak.

## Current status

- GitHub Actions builds architecture-specific Flatpak bundles for x86_64 and aarch64.
- The latest CI run on 2026-10-15 succeeded for both Flatpak architectures; the same run also passed the configured portable source-build matrix.
- The GUI and in-app host-IBus registration have been confirmed working in a GNOME session. Conflict handling, recovery, removal and session persistence still need broader testing.
- No hosted OSTree repository or public signing key is configured. There is no published Flatpak install option yet.
- The repository currently has no published GitHub release for the portable installer either.

## Build and install locally

Run from the repository root on a Linux host where Flatpak Builder and Bubblewrap can create the required namespaces:

```sh
flatpak-builder --disable-rofiles-fuse --force-clean --user --install \
  --default-branch=stable \
  --state-dir=/tmp/shanti-ibus-flatpak-state \
  /tmp/shanti-ibus-flatpak-build tools/flatpak/shanti-ibus.yml
```

The build downloads Cargo dependencies. Flatpak Builder composes AppStream metadata after compilation using `appstreamcli` and its Compose addon. If composition fails on Ubuntu, install both `appstream` and `appstream-compose`; this can fail after the application payload has already compiled and staged.

Launch the installed app:

```sh
flatpak run org.openbangla.Shanti.IBus
```

GitHub Actions also creates downloadable workflow artifacts. The artifact names are `shanti-ibus-flatpak-x86_64` and `shanti-ibus-flatpak-aarch64`; each contains an architecture-specific bundle named `shanti-ibus-x86_64.flatpak` or `shanti-ibus-aarch64.flatpak`. Download the artifact from the [CI workflow run](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/actions), extract it, then install the matching bundle:

```sh
flatpak install --user ./shanti-ibus-x86_64.flatpak
```

Use the aarch64 filename on ARM64 systems. Do not use the x86_64 bundle on ARM64 or vice versa.

## GNOME IBus registration

The app offers this integration only when it detects the GNOME desktop and its own Flatpak app ID. If the host IBus registration is missing, the app offers **Set up integration**. This setup and GNOME input-source discovery have been confirmed working in a GNOME session. While the GUI remains open, it checks every five minutes and can prompt again if a previously healthy registration/service state becomes unhealthy. The periodic check monitors registration and service health; it does not continuously test the idle engine process or typing.

Setup writes three user-scoped files:

- `~/.local/share/ibus/component/shanti-flatpak-openbangla.xml` — host IBus component descriptor.
- `~/.local/share/shanti-ibus/ibus-flatpak-host-wrapper.sh` — launcher that invokes the engine inside the Flatpak.
- `~/.config/systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-flatpak-ibus.conf` — user-service drop-in that makes the component visible to GNOME's IBus service.

The app then reloads the user systemd manager and restarts `org.freedesktop.IBus.session.GNOME.service`. After setup, open Settings → Keyboard → Input Sources and add **OpenBangla**. The engine is launched on demand when that input source is selected.

The integration does not install system-wide files. It requires access to the component, helper and systemd drop-in directories, plus the user systemd D-Bus service. If another user drop-in changes `IBUS_COMPONENT_PATH` or `ExecStart`, setup refuses to proceed rather than overwriting that file. Use the tray menu's **GNOME IBus Integration…** action to retry setup or remove the app-managed registration.

Further validation should cover a clean GNOME account and an account with existing IBus overrides. Also verify Bengali typing in a host application, logout/login persistence, conflict refusal, failed-restart recovery and removal. The app's health check does not replace those tests.

## Publishing

A `.flatpakref` template is not an installable published reference by itself. Publishing requires a hosted OSTree repository, an exported public signing key, and a generated `.flatpakref` with the real repository URL and key. None is currently configured.
