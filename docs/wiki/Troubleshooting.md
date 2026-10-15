# Troubleshooting

First identify which installation path you are using. The portable installer and the IBus-only Flatpak use different component paths and host integration.

## Portable installation: OpenBangla missing from GNOME Input Sources

Check the host IBus registry:

```bash
ibus list-engine | grep -i -A3 -B2 openbangla
```

If the portable installation's component is missing, check that the user component exists and that the IBus user service receives the intended component path:

```bash
test -f "${XDG_DATA_HOME:-$HOME/.local/share}/ibus/component/openbangla.xml"
systemctl --user show org.freedesktop.IBus.session.GNOME.service --property=Environment
```

Do not blindly replace an existing IBus service override. If the component path is wrong, review the current user-level systemd drop-ins and the portable installer configuration before changing them. After a deliberate configuration correction, reload the user systemd manager and restart IBus using the desktop's supported procedure.

Warnings about unrelated engines such as Anthy, libpinyin or typing-booster can appear while IBus scans components. They are not evidence of an OpenBangla failure if OpenBangla appears in `ibus list-engine`.

## Flatpak: OpenBangla missing from GNOME Input Sources

The IBus-only Flatpak app ID is `org.openbangla.Shanti.IBus`. This integration appears only in GNOME. Launch the app and accept its **Set up integration** prompt, or open the app's **Settings** panel and use **GNOME IBus Integration → Set up / repair**. Confirm the action; the app writes its managed host files, reloads the user systemd manager and restarts GNOME IBus. Then open GNOME Settings → Keyboard → Input Sources → **Add Input Source** and choose **OpenBangla**.

The OpenBangla Settings panel reports whether its registration files are present and the GNOME IBus service is active. If an unmanaged IBus override blocks setup, the app shows its path and offers **Back up and continue**. Choose this only if you recognize the file as a previous Shanti setup; the app preserves it as a backup before installing the current integration. Otherwise choose **Cancel** and leave the override unchanged. The integration manages files only under the user's `~/.local/share` and `~/.config/systemd/user` directories; it does not install system-wide files. Its five-minute health check monitors registration and service status, not the idle engine process or successful text entry.

For build, permissions, publishing and cleanup details, see [the IBus Flatpak guide](../../tools/flatpak/README.md). A successful CI bundle build alone does not prove that the GNOME IBus setup or Bengali text entry works.

## Flatpak install says the KDE runtime was not found

The Flatpak bundle does not include its KDE runtime. Flatpak normally fetches it from Flathub when installing the app, provided the Flathub remote and app are in the same scope. For a per-user install, add the user Flathub remote and retry the app install:

```bash
flatpak remote-add --user --if-not-exists flathub \
  https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user ./shanti-ibus-x86_64.flatpak
```

For ARM64, use `shanti-ibus-aarch64.flatpak`. If the runtime still cannot be found, install it explicitly with `flatpak install --user flathub org.kde.Platform//5.15-25.08`. For a system-wide install, omit `--user` from all commands and make sure Flathub is configured system-wide. See the [Flatpak install guide](../../tools/flatpak/README.md) for the complete flow.

## GUI reports a missing shared library

The portable installer bundles Qt and required runtime libraries under `~/.local/lib/openbangla`. Re-run `bash tools/install.sh` after changing portable runtime-bundling code, once a matching published release is available. For development builds, use `bash tools/build.sh`.

## GUI launches but layout/data files are missing

Run the packaged data check:

```bash
~/.local/bin/openbangla-gui --check-data
```

For a source build, run the equivalent check against the binary produced by that build. Confirm that the intended XDG data directory is being used before moving or deleting data files.

## Clean rebuild

Remove only the project's cached build directory, then rebuild:

```bash
rm -rf "${XDG_CACHE_HOME:-$HOME/.cache}/openbangla-keyboard/build"
bash tools/build.sh
```

This does not uninstall the application or purge user data.
