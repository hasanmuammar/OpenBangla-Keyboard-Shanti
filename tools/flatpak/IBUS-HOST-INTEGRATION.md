# Experimental host IBus integration for the Flatpak engine

## Status

Host-side engine discovery and Bengali text entry have been verified on Fedora Workstation 44, GNOME 50.5 (Wayland), IBus 1.5.34, and Flatpak 1.18.4 using both the original manual setup and the experimental host-bridge manager. The manager-generated configuration was installed, activated, used to type Bengali, then uninstalled; the original manual configuration was restored and Bengali typing was verified again. This remains experimental and is not installed by the production installer.

## Working architecture

A Flatpak engine process and an IBus component registration are separate requirements. The engine's `--ibus` path requests `org.freedesktop.IBus.OpenBangla` and serves the engine, but it does not register its component with the host daemon.

The working setup uses:

- `tools/flatpak/ibus-flatpak-host-wrapper.sh` to launch `/app/libexec/ibus-engine-openbangla --ibus` inside the selected Flatpak application.
- A user-local launcher that sets `SHANTI_FLATPAK_APP_ID` to the installed app ID and executes the wrapper.
- A user-local component descriptor under the XDG data directory's `ibus/component/` directory whose `<exec>` points to that launcher.
- A user-level systemd drop-in for the GNOME-managed IBus service so its daemon scans the user component directory.

The wrapper obtains the host IBus address with `ibus address`, extracts the Unix socket path, grants read-only access to that specific socket, passes `IBUS_ADDRESS` into the sandbox, and requests the `org.freedesktop.IBus` talk permission plus ownership of `org.freedesktop.IBus.OpenBangla`. It validates the explicit Flatpak app ID and cleans up its process group on exit or signal.

## Host IBus discovery configuration

On the tested Fedora session, the GNOME-managed IBus daemon did not discover the user-local component descriptor with its default environment. Running `ibus write-cache` with an explicit `IBUS_COMPONENT_PATH` only validated the descriptor/cache input; it did not update the live daemon's discovery environment.

The original manual user-level drop-in that resolved discovery was:

Path:

```text
~/.config/systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-probe.conf
```

Contents (the home path is specific to the test account):

```ini
[Service]
Environment="IBUS_COMPONENT_PATH=/home/hasan/.local/share/ibus/component:/usr/share/ibus/component"
ExecStart=
ExecStart=/usr/bin/ibus-daemon --panel disable --cache=refresh
```

Use the actual absolute home directory when reproducing this manual setup. Keep `/usr/share/ibus/component` in the search path so system-provided components remain discoverable. The service's original `ExecStart` must be cleared before replacing it with the drop-in command.

The manager generates equivalent user-scoped files and its own drop-in. It deliberately refuses installation if another unmanaged drop-in appears to change `IBUS_COMPONENT_PATH` or reset `ExecStart`; it does not merge or overwrite existing service settings. When uninstalling, it removes only files carrying its management marker and refuses to remove unmanaged files or symlinks.

Verification commands:

```sh
systemctl --user show org.freedesktop.IBus.session.GNOME.service -p ExecStart -p Environment
ibus list-engine --name-only | grep -i openbangla
ibus engine
systemctl --user status org.freedesktop.IBus.session.GNOME.service --no-pager
```

## Test results

On the single Fedora setup listed above, both the manual configuration and manager-generated configuration were tested:

1. The live GNOME-managed IBus service restarted successfully with the manager-generated drop-in.
2. The live IBus daemon discovered `OpenBangla`.
3. OpenBangla was selectable as an input source.
4. Bengali text entry worked.
5. The manager refused installation when the original manual service drop-in was present.
6. After uninstalling the manager-generated files and restoring the backed-up manual files, OpenBangla remained discoverable and Bengali typing worked.

Local automated validation on the migration branch passed Python compilation, all 7 manager lifecycle tests, and the host wrapper test. GitHub Actions workflows were green at merge time. The manager tests use temporary directories and mocked host commands; they do not themselves validate a real sandbox, D-Bus connection, IBus registration, or text entry.

A separate glyph-rendering issue disappeared after a full machine reboot during earlier testing; the exact cause was not established, and it is not considered an engine change.

## Remaining production work

- Decide how to configure or avoid requiring a GNOME IBus service environment override on supported distributions. The working drop-in is evidence for this Fedora setup, not a portable production solution.
- Test installation, upgrade, removal, stale-entry cleanup, and session restart on clean test accounts and additional supported distributions.
- Add repeatable end-to-end coverage for component discovery, source selection, candidate selection, commit, backspace, and focus changes.
- Review how the host bridge should coexist with other IBus customizations and non-GNOME desktop environments.
- Keep production installer behavior and production Flatpak permissions unchanged until the full lifecycle is designed and validated.

## Safety constraints

- Keep experimental descriptors, launchers, and service overrides user-scoped.
- Do not install the experimental component system-wide.
- Do not treat the disposable filesystem-permission probe or mock wrapper tests as proof of IBus integration.
- Do not broaden Flatpak D-Bus or filesystem grants beyond the tested requirements without a separate review.
