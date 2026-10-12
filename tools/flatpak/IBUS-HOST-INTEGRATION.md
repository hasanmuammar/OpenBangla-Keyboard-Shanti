# Experimental host IBus integration for the Flatpak engine

## Status

Host-side engine discovery and Bengali text entry have been verified manually on Fedora Workstation 44, GNOME 50.5 (Wayland), IBus 1.5.34, and Flatpak 1.18.4. The tested setup is still experimental: it uses a disposable user-local IBus component descriptor, a user-local launcher, and a user-level systemd drop-in. These are not installed by the production installer.

## Working architecture

A Flatpak engine process and an IBus component registration are separate requirements. The engine's `--ibus` path requests `org.freedesktop.IBus.OpenBangla` and serves the engine, but it does not register its component with the host daemon.

The working test used:

- `tools/flatpak/ibus-flatpak-host-wrapper.sh` to launch `/app/libexec/ibus-engine-openbangla --ibus` inside the selected Flatpak application.
- A user-local launcher that sets `SHANTI_FLATPAK_APP_ID` to the installed probe app ID and executes the wrapper.
- A user-local component descriptor under `~/.local/share/ibus/component/` whose `<exec>` points to that launcher.
- An environment override for the GNOME-managed IBus service so its daemon scans the user component directory.

The wrapper obtains the host IBus address with `ibus address`, extracts the Unix socket path, grants read-only access to that specific socket, passes `IBUS_ADDRESS` into the sandbox, and requests the `org.freedesktop.IBus` talk permission plus ownership of `org.freedesktop.IBus.OpenBangla`. It validates the explicit Flatpak app ID and cleans up its process group on exit or signal.

## Host IBus discovery configuration

On the tested Fedora session, the GNOME-managed IBus daemon did not discover the user-local component descriptor with its default environment. Running `ibus write-cache` with an explicit `IBUS_COMPONENT_PATH` only validated the descriptor/cache input; it did not update the live daemon's discovery environment.

The user-level drop-in that resolved discovery was:

Path:

```text
~/.config/systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-probe.conf
```

Contents:

```ini
[Service]
Environment="IBUS_COMPONENT_PATH=/home/hasan/.local/share/ibus/component:/usr/share/ibus/component"
ExecStart=
ExecStart=/usr/bin/ibus-daemon --panel disable --cache=refresh
```

The home path above is specific to the test account; use the actual absolute home directory when reproducing it. Keep `/usr/share/ibus/component` in the search path so system-provided components remain discoverable. The service's original `ExecStart` must be cleared before replacing it with the drop-in command.

After `systemctl --user daemon-reload` and restarting `org.freedesktop.IBus.session.GNOME.service`, its effective environment included `IBUS_COMPONENT_PATH`; `ibus list-engine --name-only` listed `OpenBangla`, and GNOME Settings offered OpenBangla as an input source. Selecting the engine and entering Bengali text worked. A separate glyph-rendering issue disappeared after a full machine reboot; the exact cause was not established, and it is not considered an engine change.

Verification commands:

```sh
systemctl --user show org.freedesktop.IBus.session.GNOME.service -p ExecStart -p Environment
ibus list-engine --name-only | grep -i openbangla
ibus engine
```

## Test results

The manual end-to-end check confirmed all of the following on the single Fedora setup listed above:

1. The live IBus daemon discovered `OpenBangla`.
2. GNOME Settings displayed it as an available input source.
3. The user could select it and enter Bengali text in an application.

The wrapper's `tools/flatpak/test-ibus-flatpak-host-wrapper.sh` test separately covers app-ID validation, IBus address discovery, Flatpak argument forwarding, and signal/process-group cleanup using a mocked Flatpak command. Those unit tests do not validate a real sandbox, D-Bus connection, IBus registration, or text entry.

## Remaining production work

- Design an installer/uninstaller for the host-side launcher and component descriptor. Do not hard-code a developer home directory.
- Decide how to configure or avoid requiring a GNOME IBus service environment override on supported distributions. The manual drop-in is evidence for this test machine, not a portable production solution.
- Test installation, upgrade, removal, stale-entry cleanup, and session restart on clean test accounts.
- Add repeatable end-to-end coverage for component discovery, source selection, candidate selection, commit, backspace, and focus changes.
- Keep production installer behavior and production Flatpak permissions unchanged until the full lifecycle is designed and validated.

## Safety constraints

- Keep experimental descriptors, launchers, and service overrides user-scoped.
- Do not install the experimental component system-wide.
- Do not treat the disposable filesystem-permission probe or mock wrapper tests as proof of IBus integration.
- Do not broaden Flatpak D-Bus or filesystem grants beyond the tested requirements without a separate review.
