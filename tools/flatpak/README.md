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

## Publishing

[`shanti-ibus.flatpakref.in`](shanti-ibus.flatpakref.in) is a template only. Publishing still requires a hosted OSTree repository, its exported public signing key, and a generated `.flatpakref` with those values. No repository URL or signing key is currently configured.
