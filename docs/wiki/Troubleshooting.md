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

The IBus-only Flatpak app ID is `org.openbangla.Shanti.IBus`. On GNOME, launch the app and use its **Set up integration** prompt, or open the tray menu and choose **GNOME IBus Integration…**. After successful setup, open Settings → Keyboard → Input Sources and add **OpenBangla**.

If setup refuses to continue, inspect the conflicting user service drop-in reported by the app. The integration intentionally refuses to overwrite an unmanaged drop-in that changes `IBUS_COMPONENT_PATH` or `ExecStart`. Do not remove or edit that file without understanding what created it. The app-managed files are under the user's `~/.local/share` and `~/.config/systemd/user` trees; the integration does not install system-wide files.

For build, permissions, publishing and cleanup details, see [the IBus Flatpak guide](../../tools/flatpak/README.md). A successful CI bundle build alone does not prove that the GNOME IBus setup or Bengali text entry works.

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
