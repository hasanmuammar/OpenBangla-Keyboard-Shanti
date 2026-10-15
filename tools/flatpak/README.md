# Shanti IBus Flatpak

`shanti-ibus.yml` builds the Qt keyboard application and IBus engine as `org.openbangla.Shanti.IBus`, displayed as **OpenBangla Keyboard Shanti (IBus)**. It does not include the Fcitx5 engine. The app ID is specific to IBus so a future Fcitx5 Flatpak can use a separate identity. The separate user-local installer remains unchanged.

## Build and install

Build from the repository root on a Linux host where Flatpak's Bubblewrap sandbox can create its required namespaces:

```sh
flatpak-builder --disable-rofiles-fuse --force-clean --user --install \
  --default-branch=stable \
  --state-dir=/tmp/shanti-ibus-flatpak-state \
  /tmp/shanti-ibus-flatpak-build tools/flatpak/shanti-ibus.yml
```

The build downloads Cargo dependencies. This IBus-only package has been built, installed, and confirmed working by the developer. Flatpak Builder composes AppStream metadata after compilation using `appstreamcli` and its Compose addon. Install both on the build host if composition fails; on Ubuntu, the packages are `appstream` and `appstream-compose`. The failure occurs after the application payload has been compiled and staged.

GitHub Actions builds this same manifest natively for `x86_64` and `aarch64`, then uploads separate architecture-specific bundles named `shanti-ibus-flatpak-x86_64` and `shanti-ibus-flatpak-aarch64`. Install the bundle matching the device architecture. Steam Deck is x86_64; ARM64 devices use aarch64. Flatpak bundles are architecture-specific, so a single install command that automatically selects either architecture requires publishing both builds in one OSTree repository.

Run the installed application with:

```sh
flatpak run org.openbangla.Shanti.IBus
```

The app's AppIndicator, layout changes, and settings changes are confirmed on Bluefin/GNOME with the Shanti app ID. Fcitx5 integration is not provided.

## GNOME IBus registration

On a GNOME session, the Flatpak checks whether its user-scoped IBus registration is present and the GNOME IBus service is active. If registration is missing, the app offers **Set up integration** at launch. While open, it checks every five minutes and prompts again if a previously healthy integration stops working. Setup writes a user component descriptor and Flatpak launcher, then adds a user-level systemd drop-in so GNOME's IBus service scans the component directory. It restarts that user service. After setup, add **OpenBangla** from Settings → Keyboard → Input Sources. IBus starts the engine on demand when that input source is selected; the periodic check monitors registration and the IBus service, not the idle engine process.

The prompt is limited to the GNOME IBus Flatpak app ID. It does not install system-wide files. If another user drop-in already changes `IBUS_COMPONENT_PATH` or `ExecStart`, setup stops and reports the conflicting file instead of replacing it. The tray menu also includes **GNOME IBus Integration…** to retry setup or remove the app-managed registration before uninstalling. The app needs access to the user IBus component directory, its own helper directory, the GNOME IBus service drop-in directory, and the user systemd D-Bus service to offer this action.

## Publishing

[`shanti-ibus.flatpakref.in`](shanti-ibus.flatpakref.in) is a template only. Publishing still requires a hosted OSTree repository, its exported public signing key, and a generated `.flatpakref` with those values. No repository URL or signing key is currently configured.
