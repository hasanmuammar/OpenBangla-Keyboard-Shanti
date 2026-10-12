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

## CI workflow

The workflow [Flatpak filesystem permission probe](../../.github/workflows/flatpak-permission-probe.yml) builds and runs this test on an Ubuntu GitHub Actions runner when the probe files change on `experiment/flatpak-ibus` or when manually dispatched. Check the GitHub Actions result to confirm execution; adding the workflow alone does not prove the probe passed. A local host-shell run is explicitly refused by the probe.

## Experimental IBus host integration

Manual host IBus discovery and Bengali text entry have been verified on one Fedora Workstation 44 / GNOME 50.5 setup. The test needed a user-local component descriptor and launcher, plus `IBUS_COMPONENT_PATH` in the GNOME-managed IBus service environment. The production installer and manifest remain unchanged. See [Experimental host IBus integration](IBUS-HOST-INTEGRATION.md) for the working configuration and remaining production work.

## Experimental host IBus bridge manager

The manual setup can be created and removed with `manage-ibus-flatpak-host.py`. This is an opt-in development helper, not part of `tools/install.sh` and not a production-supported installation path. It targets the GNOME-managed IBus systemd user service used by the verified Fedora setup.

Install for an already-installed Flatpak app:

```sh
python3 tools/flatpak/manage-ibus-flatpak-host.py install --app-id io.github.hasanmuammar.ShantiIbusEngineProbe
systemctl --user daemon-reload
systemctl --user restart org.freedesktop.IBus.session.GNOME.service
ibus list-engine --name-only | grep -i openbangla
```

Use the actual installed app ID. The command refuses to overwrite existing target files and creates user-local wrapper, launcher, IBus component metadata, and a GNOME IBus service drop-in. Review the generated paths before restarting the service.

Remove only files created by this helper:

```sh
python3 tools/flatpak/manage-ibus-flatpak-host.py uninstall
systemctl --user daemon-reload
systemctl --user restart org.freedesktop.IBus.session.GNOME.service
```

The helper leaves empty parent directories in place. The service override and launcher behavior have only been manually verified on Fedora Workstation 44 / GNOME 50.5 / IBus 1.5.34. The production installer and production Flatpak manifest remain unchanged.



### Run the host bridge manager tests

The lifecycle tests use temporary directories and mocked host commands; they do not change the current user's IBus configuration:

```sh
python3 -m py_compile tools/flatpak/manage-ibus-flatpak-host.py tools/flatpak/test-manage-ibus-flatpak-host.py
python3 tools/flatpak/test-manage-ibus-flatpak-host.py
bash -n tools/flatpak/ibus-flatpak-host-wrapper.sh
bash tools/flatpak/test-ibus-flatpak-host-wrapper.sh
```

The tests cover generated files and modes, component XML parsing, shell syntax, systemd escaping, overwrite protection, rollback after a simulated creation failure, and safe refusal to remove unmanaged files or symlinks. They are unit tests only. Manual integration testing has confirmed IBus discovery and Bengali text entry on one Fedora Workstation 44 / GNOME 50.5 / IBus 1.5.34 setup. A separate clean user account on the same machine passed manager install, IBus discovery, and uninstall/removal checks; Bengali text entry was not confirmed in that clean account. This does not establish portability to other distributions or desktop environments.
