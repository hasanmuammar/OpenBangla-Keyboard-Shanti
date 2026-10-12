# Experimental host IBus integration for the Flatpak engine

## Status

Host-side discovery and basic typing have now been verified on Fedora Workstation 44 with GNOME 50.5 (Wayland), IBus 1.5.34, and Flatpak 1.18.4. The setup remains experimental: it uses a disposable user component descriptor and a user-local launcher, neither installed by the production installer. Do not treat this as a production-ready installation path yet.

## Working architecture

The engine runs inside Flatpak, but host IBus needs a component descriptor to discover the engine. Starting the sandboxed process and owning `org.freedesktop.IBus.OpenBangla` alone is not sufficient.

The working test used:

- `tools/flatpak/ibus-flatpak-host-wrapper.sh` to launch `/app/libexec/ibus-engine-openbangla --ibus` inside the selected Flatpak app.
- A user-local launcher that exports `SHANTI_FLATPAK_APP_ID` and invokes the wrapper.
- A user-local descriptor under `~/.local/share/ibus/component/` whose `<exec>` points to that launcher.
- A GNOME-managed IBus service environment that includes the user component directory in `IBUS_COMPONENT_PATH`.

The wrapper discovers the host IBus address with `ibus address`, grants the sandbox read-only access to that specific socket, passes the address as `IBUS_ADDRESS`, and requests only the IBus D-Bus name permissions needed by the engine. It validates the app ID and cleans up its Flatpak process group when stopped.

## IBus daemon discovery configuration

On the tested Fedora setup, the GNOME-managed service did not discover the user-local descriptor by default. Generating a cache with an explicit `IBUS_COMPONENT_PATH` was not enough; the environment must be present in the live GNOME IBus daemon.

The temporary user service drop-in that made discovery work was:

```ini
[Service]
Environment="IBUS_COMPONENT_PATH=/home/hasan/.local/share/ibus/component:/usr/share/ibus/component"
ExecStart=
ExecStart=/usr/bin/ibus-daemon --panel disable --cache=refresh
```

It was placed at:

```text
~/.config/systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-probe.conf
```

This is a test-specific configuration, not a general installation recipe. Replace the example home path with the actual home directory if reproducing the test. The standard component directory remains in the path so built-in and system engines are still discoverable. `--cache=refresh` refreshes the daemon's component cache at startup.

After reloading the user systemd configuration and restarting `org.freedesktop.IBus.session.GNOME.service`, `ibus list-engine --name-only` listed `OpenBangla`, and GNOME Settings offered it as an input source. Selecting it and entering Bengali text worked. A later Bengali glyph-rendering problem disappeared after a full system reboot; no specific missing package or code defect was established as its cause.

Useful verification commands:

```sh
systemctl --user show org.freedesktop.IBus.session.GNOME.service -p ExecStart -p Environment
ibus list-engine --name-only | grep -i openbangla
ibus engine
```

## Test coverage and limitations

The wrapper's shell tests cover app-ID validation, host IBus address discovery, argument forwarding, and process-group cleanup using a mocked `flatpak` command. They do not test a real sandbox, daemon discovery, or typing.

The manual Fedora test verifies that the engine appears in IBus, is selectable in GNOME Settings, and can enter text. It is not an automated end-to-end test and has not been validated across other distributions or desktop environments.

The production installer and production Flatpak manifest have not been changed to install the descriptor, launcher, or systemd drop-in. Before production integration, design a maintainable install/uninstall flow for the host descriptor and launcher, derive paths dynamically rather than hard-coding a home directory, and test cleanup and engine discovery on a clean user session.

## Safe investigation rules

- Keep test descriptors, launchers, and systemd overrides user-scoped and disposable.
- Do not install the experimental component system-wide.
- Do not copy filesystem-permission probe grants into a production manifest.
- Keep mock wrapper tests distinct from real IBus integration tests.
- Do not describe this experimental setup as production-supported until installation, removal, discovery, and text entry are covered by a repeatable test plan.
