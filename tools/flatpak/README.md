# Shanti Flatpak

`openbangla.yml` builds the Qt keyboard application and IBus engine as `io.github.hasanmuammar.Shanti`, displayed as **OpenBangla Keyboard Shanti**. Its launcher, icons, and AppStream metadata are renamed for Shanti. The separate user-local installer remains unchanged.

## Build and install

Build from the repository root on a Linux host where Flatpak's Bubblewrap sandbox can create its required namespaces:

```sh
flatpak-builder --disable-rofiles-fuse --force-clean --user --install \
  --default-branch=stable \
  --state-dir=/tmp/openbangla-flatpak-state \
  /tmp/openbangla-flatpak-build tools/flatpak/openbangla.yml
```

The build downloads Cargo dependencies. The Shanti-ID package has been built, installed, and confirmed working by the developer. Flatpak Builder composes AppStream metadata after compilation using `appstreamcli` and its Compose addon. Install both on the build host if composition fails; on Ubuntu, the packages are `appstream` and `appstream-compose`. The failure occurs after the application payload has been compiled and staged.

Run the installed application with:

```sh
flatpak run io.github.hasanmuammar.Shanti
```

The app's AppIndicator, layout changes, and settings changes are confirmed on Bluefin/GNOME with the Shanti app ID. Fcitx5 integration is not provided.

## Publishing

[`openbangla.flatpakref.in`](openbangla.flatpakref.in) is a template only. Publishing still requires a hosted OSTree repository, its exported public signing key, and a generated `.flatpakref` with those values. No repository URL or signing key is currently configured.
