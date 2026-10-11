# Experimental host IBus integration for the Flatpak engine

## Status

This is an investigation, not a working or supported production integration. Do not treat the wrapper or the disposable test descriptor as sufficient to make OpenBangla available in GNOME Settings. The production installer and production manifests must remain unchanged until end-to-end host registration and typing have been demonstrated.

The current prototype uses `tools/flatpak/ibus-flatpak-host-wrapper.sh` to launch the engine inside a Flatpak sandbox while connecting it to the host IBus bus. The wrapper is intentionally not installed by the production installer.

## What has been verified

- The wrapper's shell tests pass for app-ID validation, host IBus address discovery, Flatpak argument forwarding, and process-group cleanup. These tests use a mock `flatpak` command; they do not exercise a real sandbox or IBus registry.
- A direct Flatpak launch of `/app/libexec/ibus-engine-openbangla --ibus` can stay running and own `org.freedesktop.IBus.OpenBangla` while the process is alive.
- A disposable host-side component descriptor can be parsed into an IBus registry cache when its directory is supplied through `IBUS_COMPONENT_PATH`.
- On the investigated Fedora Workstation 44 / GNOME 50.5 / Wayland / IBus 1.5.34 / Flatpak 1.18.4 setup, `ibus list-engine --name-only` still did **not** list `OpenBangla`. GNOME Settings therefore did not offer it as an input source.
- Adding `IBUS_COMPONENT_PATH` to a temporary user-level drop-in for `org.freedesktop.IBus.session.GNOME.service` did not make the engine appear. That drop-in was removed afterward.

## Current blocker and interpretation

The cache-generation test and the active daemon are not equivalent: `ibus write-cache --file=/tmp/...` writes a separate cache file, not the live daemon's registry, and seeing the component in that file does not prove that the running daemon has registered the engine. IBus documents `IBUS_COMPONENT_PATH` as a colon-delimited custom component path; when set, it replaces the default component search path, so any diagnostic value must explicitly include `/usr/share/ibus/component` if standard engines should remain available. The user-service environment override was tried and did not change discovery, so the environment variable alone is not an established fix. See the [IBusRegistry documentation](https://ibus.github.io/docs/ibus-1.5/IBusRegistry.html) and [ibus command manual](https://manpages.debian.org/testing/ibus-data/ibus.1.en.html).

The temporary descriptor emitted GLib critical warnings from `g_variant_new_string` during `ibus write-cache`, and the decoded cache showed `[Invalid UTF-8]` for missing optional `homepage` and `textdomain` fields. This is a concrete warning worth eliminating in future test fixtures, but the investigation did not prove that these warnings explain the discovery failure. The production descriptor template, `data/openbangla.xml.in`, already supplies both fields.

In `src/engine/ibus/main.cpp`, the `--ibus` path requests the `org.freedesktop.IBus.OpenBangla` D-Bus name and provides the engine factory; it does not register the component with the host daemon. Host-side discovery must therefore be validated separately from starting the sandboxed process. Owning the D-Bus name is not evidence that IBus has discovered the engine.

## Safe investigation rules

- Keep all test descriptors and launchers disposable and user-scoped.
- Do not install the experimental component system-wide.
- Do not modify the production installer or production Flatpak permissions based only on the current probe.
- Do not leave a GNOME IBus systemd drop-in behind as a workaround; the tested override did not resolve discovery.
- Do not describe the integration as working until the engine appears in `ibus list-engine`, can be selected in GNOME Settings, and successfully commits text in a real application.
- Keep shell-wrapper tests and real integration tests clearly separate. A passing mock-based wrapper test is not an end-to-end IBus test.

## Next engineering work

1. Improve the disposable component fixture so it includes the metadata fields expected by IBus and produces no avoidable GLib warnings.
2. Capture the GNOME-managed IBus daemon's logs and effective unit configuration, then determine which registry/cache path it actually consumes. Read-only baseline commands are:

   ```sh
   systemctl --user show org.freedesktop.IBus.session.GNOME.service -p ExecStart -p Environment
   journalctl --user -u org.freedesktop.IBus.session.GNOME.service -b --no-pager
   ibus list-engine --name-only | grep -i openbangla || true
   ```
3. Validate the component descriptor and launch semantics against the installed IBus version, including whether the host daemon attempts to execute the descriptor's `<exec>` command and how that command should bridge to Flatpak.
4. Add an end-to-end test only once there is a repeatable, isolated test environment. It must assert engine discovery and text entry, not just process startup or D-Bus name ownership.

## Local disposable artifacts

The investigation used files outside the repository:

- `~/.local/share/ibus/component/shanti-flatpak-probe.xml`
- `~/.local/bin/shanti-flatpak-probe-launcher`
- `/tmp/shanti-ibus-registry.xml`
- `/tmp/shanti-ibus-registry-clean.xml`

They are not production files and are not tracked here. Remove them only when no longer needed for diagnosis. The tested systemd drop-in under `~/.config/systemd/user/org.freedesktop.IBus.session.GNOME.service.d/10-shanti-flatpak.conf` was undone.
