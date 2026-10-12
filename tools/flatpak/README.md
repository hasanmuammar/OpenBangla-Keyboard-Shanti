# Shanti Flatpak filesystem-permission probe

This is a disposable test application, not the Shanti manager and not a production manifest. It tests four narrowly scoped, Shanti-specific permission aliases:

- `~/.local/shanti-flatpak-permission-probe`
- `xdg-data/shanti-flatpak-permission-probe`
- `xdg-config/shanti-flatpak-permission-probe`
- `xdg-cache/shanti-flatpak-permission-probe`

The probe refuses to run outside Flatpak (it checks `FLATPAK_ID` and `/.flatpak-info`). For the XDG aliases, Flatpak exposes host subdirectories under corresponding sandbox paths `$XDG_DATA_HOME/shanti-flatpak-permission-probe`, `$XDG_CONFIG_HOME/shanti-flatpak-permission-probe`, and `$XDG_CACHE_HOME/shanti-flatpak-permission-probe`. The probe must write through those sandbox-visible paths. It reports `HOST_XDG_*` as a reference to the corresponding host locations; it does not open those absolute host paths directly.

The probe creates a temporary child directory and marker file inside each root, reads the marker back, then removes the marker and temporary child. It never writes to Shanti's actual install, resource, IBus, Fcitx5, or configuration paths. Flatpak may pre-create the permission roots before launching the app, so empty `shanti-flatpak-permission-probe` directories can remain after a run.

The manifest deliberately grants only those test paths. It has no network, host-command, system-bus, home-wide, or host-wide permission. Do not copy its test grants into the production manifest as a substitute for reviewing the real install paths.

## Build and run

Install the matching Freedesktop SDK/runtime 26.08 from the configured Flatpak remote if not already installed. Then, from the repository root:

```sh
flatpak-builder --user --install --force-clean /tmp/shanti-fs-probe-build tools/flatpak/permission-probe.yml
flatpak run io.github.hasanmuammar.ShantiFilesystemProbe
```

The probe prints sandbox XDG paths, host XDG locations reported by Flatpak, and one PASS/FAIL result per test root. A passing result establishes only that these disposable path grants can create directories and read/write a marker through the sandbox-visible mounts in this environment. It does not establish that the eventual production paths, IBus/Fcitx5 registration, module ABI, host typing, or daemon refresh will work.

## Custom XDG test

For a second test, use a disposable user account configured with non-default host `XDG_DATA_HOME`, `XDG_CONFIG_HOME`, and `XDG_CACHE_HOME` values **before installing this user-scoped probe app**. Then run the probe and confirm that writes through sandbox `XDG_*_HOME/shanti-flatpak-permission-probe` paths appear in the corresponding custom host XDG directories. Avoid changing XDG variables only for `flatpak run`: those variables can also affect the Flatpak client's user installation lookup. Use an account and scratch paths that contain no Shanti production data.

## Current limitation

The workflow [Flatpak filesystem permission probe](../../.github/workflows/flatpak-permission-probe.yml) builds and runs this test on an Ubuntu GitHub Actions runner when the probe files change on `experiment/flatpak-ibus` or when manually dispatched. The result must be checked in GitHub Actions; adding the workflow is not itself evidence that the sandbox test passed. A local host-shell run is explicitly refused by the probe.

## Experimental IBus host integration

Host IBus discovery and manual text entry have now been verified on one Fedora Workstation 44 / GNOME 50.5 setup using a user-local component descriptor, launcher wrapper, and GNOME IBus service environment override. The production installer and manifest are unchanged; this remains an experimental manual setup. See [Experimental host IBus integration](IBUS-HOST-INTEGRATION.md) for the configuration that worked and the remaining productionization requirements.
