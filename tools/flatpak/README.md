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

This host integration is available only when the app detects both a GNOME session and the Flatpak app ID `org.openbangla.Shanti.IBus`. It does not apply to the portable install or other Flatpak builds.

### Set up and use the input source

1. Launch the Flatpak GUI. If its GNOME registration is missing or incomplete, the app offers **Set up integration**. You can also open the app's **Settings** panel and use the **GNOME IBus Integration** section.
2. In Settings, check the status. **Configured. The GNOME IBus service is active** means the managed registration files are present and the GNOME IBus service is active. Otherwise, use **Set up / repair** and confirm the action.
3. Setup creates or updates the three app-managed files listed below, reloads the user systemd manager, and restarts GNOME's IBus service. It refuses to replace unmanaged files and reports a conflict if another drop-in changes `IBUS_COMPONENT_PATH` or `ExecStart`.
4. Open GNOME Settings → Keyboard → Input Sources, choose **Add Input Source**, and add **OpenBangla**. Select it to start the engine on demand, then type in a host application.

The GUI, registration and input-source discovery have been confirmed working in a GNOME session. While the GUI is open, the app checks registration and IBus service health every five minutes. If a setup that was healthy becomes unhealthy, it can offer setup again. This does not monitor the on-demand engine process while idle and does not prove that typing is working.

Setup writes three user-scoped files:

- `~/.local/share/ibus/component/shanti-flatpak-openbangla.xml` — host IBus component descriptor.
- `~/.local/share/shanti-ibus/ibus-flatpak-host-wrapper.sh` — launcher that invokes the engine inside the Flatpak.
- `~/.config/systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-flatpak-ibus.conf` — user-service drop-in that makes the component visible to GNOME's IBus service.

The host wrapper obtains the current host IBus address and starts the engine inside the Flatpak with access to that IBus socket. It also passes the host OpenBangla configuration directory when present. The engine runs on demand when GNOME selects the input source.

The integration does not install system-wide files. It requires access to the component, helper and systemd drop-in directories, plus the user systemd D-Bus service. In the GUI Settings panel, **Set up / repair** re-creates or updates the app-managed registration after confirmation. **Remove integration** is enabled when app-managed files are found; it asks for confirmation, removes only files containing the Shanti ownership marker, then reloads systemd and restarts GNOME IBus. It does not uninstall the Flatpak or remove the OpenBangla entry from GNOME's saved Input Sources list. The tray menu's **GNOME IBus Integration…** action remains available as a shortcut.

Further validation should cover a clean GNOME account and an account with existing IBus overrides. Also verify Bengali typing in a host application, logout/login persistence, conflict refusal, failed-restart recovery and removal. The app's health check does not replace those tests.

## Publishing

A `.flatpakref` template is not an installable published reference by itself. Publishing requires a hosted OSTree repository, an exported public signing key, and a generated `.flatpakref` with the real repository URL and key. None is currently configured.
