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

The build downloads Cargo dependencies. The last successful complete build used the provisional ID `io.github.openbangla.keyboard` on Bluefin/GNOME. The Shanti ID and metadata changes have not yet been rebuilt; the attempt in the current restricted environment stopped because Bubblewrap could not create a `NETLINK_ROUTE` socket. Rebuild and inspect the exported launcher, icons, and AppStream metadata before publishing.

Run the installed application with:

```sh
flatpak run io.github.hasanmuammar.Shanti
```

The app's AppIndicator, layout changes, and settings changes were confirmed on Bluefin/GNOME under the provisional ID. Host IBus discovery and Bengali typing were separately confirmed on Fedora Workstation 44/GNOME 50.5 through the experimental bridge. This does not establish support across other desktops or distributions. Fcitx5 integration is not provided.

## Optional GNOME IBus bridge

The host bridge is separate from the Flatpak package and targets the GNOME-managed IBus service. It has been tested on Fedora Workstation 44/GNOME 50.5. After installing the Shanti Flatpak, register the host-side component:

```sh
python3 tools/flatpak/manage-ibus-flatpak-host.py install \
  --app-id io.github.hasanmuammar.Shanti
systemctl --user daemon-reload
systemctl --user restart org.freedesktop.IBus.session.GNOME.service
```

Remove the bridge's managed files with:

```sh
python3 tools/flatpak/manage-ibus-flatpak-host.py uninstall
systemctl --user daemon-reload
systemctl --user restart org.freedesktop.IBus.session.GNOME.service
```

This bridge is opt-in and is not part of the production installer. Its scope and platform limits are documented in [Experimental host IBus integration](IBUS-HOST-INTEGRATION.md).

## Publishing

[`openbangla.flatpakref.in`](openbangla.flatpakref.in) is a template only. Publishing still requires a hosted OSTree repository, its exported public signing key, and a generated `.flatpakref` with those values. No repository URL or signing key is currently configured.
