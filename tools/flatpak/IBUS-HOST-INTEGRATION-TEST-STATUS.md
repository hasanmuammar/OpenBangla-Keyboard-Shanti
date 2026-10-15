# Experimental Flatpak IBus host integration: test status

This document records the current scope and limits of the experimental host-side IBus integration. It is not a production setup guide.

## Scope and safety

The host wrapper and `manage-ibus-flatpak-host.py` prototype are opt-in experiments. They are not installed by the production installer. Use a disposable Flatpak app/repository and a test account where possible. Do not run `openbangla-gui --setup-system` during baseline testing; it can modify host input-source configuration.

The wrapper requires `SHANTI_FLATPAK_APP_ID`, obtains the host IBus address with `ibus address`, passes the IBus address to the Flatpak engine, and grants access to the IBus socket. It also attempts to expose the host OpenBangla settings directory read-only when it exists. Flatpak may still apply sandbox-specific configuration paths, so setting `XDG_CONFIG_HOME` in the wrapper does not by itself prove that the engine reads the host settings file.

## Verified so far

- The verified full-app build used the provisional app ID `io.github.openbangla.keyboard`. For publication, the chosen Shanti ID is `io.github.hasanmuammar.Shanti` with display name **OpenBangla Keyboard Shanti**. The manifest now rewrites the sandboxed AppStream metadata and desktop exports for that identity; it must be rebuilt and rechecked before release.
- Latest developer verification (2026-10-15): the complete Flatpak is working in the Bluefin/GNOME environment. The AppIndicator appears, the layout can be changed, and settings changes work as expected. This supersedes the earlier entries below that described GUI settings synchronization and visible AppIndicator behavior as unverified. The result is specific to this environment; cross-desktop compatibility has not been established.

- The wrapper unit test passes for validation, address discovery, argument forwarding, and signal cleanup.
- In a disposable test account, the experimental host descriptor allowed IBus to discover the test engine; Bengali text entry was manually confirmed.
- The manager's install/uninstall lifecycle was exercised in the disposable account. Uninstall removed the managed host files and the engine disappeared from `ibus list-engine`; the user IBus service remained active.
- A rebuilt disposable probe contains the engine binary, seven layout JSON files, and four data JSON files under `/app/share/openbangla-keyboard`.
- The disposable probe was rebuilt and reinstalled after removing temporary `[SHANTI-DEBUG]` instrumentation. The rebuilt binary contains the layout-path fallback, and Bengali typing was manually confirmed with the National (Jatiya) layout.
- The engine was observed under the IBus-managed process tree. Manually terminating the wrapper while OpenBangla remained selected caused IBus to launch it again; this is expected engine lifecycle behavior, not evidence of an independent service.
- The experimental host descriptor names `/home/hasan/.local/bin/shanti-flatpak-probe-launcher`; that launcher sets `SHANTI_FLATPAK_APP_ID=io.github.hasanmuammar.ShantiIbusEngineProbe` and executes `tools/flatpak/ibus-flatpak-host-wrapper.sh`.
- On 2026-10-15, the engine was rebuilt from the current checkout inside `org.kde.Sdk//5.15-25.08` with Rust stable 1.98.1 and installed as a disposable user Flatpak. The experimental wrapper acquired `org.freedesktop.IBus.OpenBangla` on the address returned by `ibus address`; after SIGTERM shutdown, `NameHasOwner` returned false. This verifies startup, connection to the IBus daemon, and wrapper cleanup in this session. That bounded wrapper check did not register a host component or alter IBus service configuration.
- In a subsequent reversible host-discovery check, the manager installed its user-local wrapper, launcher, component descriptor, and GNOME IBus service drop-in. The service remained active and `ibus list-engine --name-only` reported `OpenBangla`; the selected engine remained `xkb:us::eng`. The manager then removed all four marked files, systemd was reloaded, and the IBus service was restarted. OpenBangla disappeared from the engine list, the service remained active, and the original engine selection was unchanged. Bengali text entry was not tested in this run because OpenBangla was not selected.
- A follow-up briefly selected `OpenBangla`; IBus launched the Flatpak and the IBus daemon reported its engine name as owned. The selection was restored to `xkb:us::eng`. No key-to-commit test was run: this session enforces the global engine setting for private input contexts, and the host has no `/dev/uinput` device or suitable text-entry window for safe synthetic key delivery. Removing the manager files and restarting IBus stopped the test app, cleared the engine name, and restored the original session configuration. No runtime code defect was reproduced.
- On 2026-10-15, `tools/flatpak/openbangla.yml` built the complete GUI and IBus engine with KDE Platform/SDK 5.15-25.08 and Rust stable 1.98.1. It includes seven layouts and four data files, plus an app-ID-named desktop launcher and icons. The app installed in the Bluefin GNOME user environment as `io.github.openbangla.keyboard`; `openbangla-gui --check-data` passed inside the sandbox, and GNOME's exported app launcher was present.
- The manager registered the full app with the live GNOME IBus service on Bluefin, and `ibus list-engine --name-only` reported `OpenBangla`. The existing host wrapper grants the Flatpak engine read-only access to `~/.config/OpenBangla`, while the GUI manifest grants write access to that one directory. Layout and settings changes have since been confirmed working by the developer.
- The AppIndicator extension and `org.kde.StatusNotifierWatcher` were active on Bluefin. The manifest's `--talk-name=org.kde.StatusNotifierWatcher` permission allows the GUI to register the status icon, and the developer has confirmed the AppIndicator appears correctly.
- This local build needed network access for Cargo crates. The build host lacked `appstreamcli`; compilation completed, then the build was finalized and exported manually. The `riti` submodule has no committed Cargo.lock, so dependency resolution is not yet pinned for reproducible offline/Flathub builds.

These checks and the latest developer verification establish working packaging, IBus discovery, AppIndicator display, layout changes, and settings changes in the tested Bluefin/GNOME environment. They do not establish compatibility with other distributions/desktops or constitute a published production release.

## Layout-path fallback

In `src/engine/ibus/main.cpp`, `update_with_settings()` now checks whether the configured layout path is an absolute path that does not exist in the engine's filesystem. If a file with the same basename exists under `LayoutsFilePath()`, the engine uses that bundled file instead. This handles a host GUI setting that points to a layout JSON file at a host-only absolute path while the engine is sandboxed.

The fallback does not synchronize settings. It only resolves the path supplied to the engine. It should not rewrite the user's stored host setting.

## Settings-source investigation and remaining limitation

The GUI's `Layout::setLayout()` stores the special strings `avro_phonetic` and `khipro` for those layout types, and stores a JSON layout path for other layouts. The IBus engine reads the configured layout through `gSettings->getLayoutPath()`.

The host settings file was observed to contain:

```ini
[layout]
name=National (Jatiya)
path=/home/hasan/shanti-gui-test-prefix/usr/local/share/openbangla-keyboard/layouts/National_Jatiya.json
```

Within the Flatpak probe, the host path did not exist in the sandbox. Copying the settings file to the probe's Flatpak-private configuration location,
`~/.var/app/io.github.hasanmuammar.ShantiIbusEngineProbe/config/OpenBangla/Keyboard.conf`,
allowed the engine to read the National (Jatiya) setting; the path fallback then resolved the host-only absolute path to
`/app/share/openbangla-keyboard/layouts/National_Jatiya.json`. Bengali typing worked with that layout.

This demonstrates the path fallback and the private-config test workaround. It does **not** establish that changes made later in the host GUI are propagated to the Flatpak-private settings file, or that the running engine reloads those changes. Settings synchronization remains unresolved and must be tested separately.

## Next steps

1. Test whether a host GUI layout change is reflected in the Flatpak engine's effective settings; check both the settings file actually read inside the sandbox and behavior after reselecting/restarting the engine.
2. If synchronization is not automatic, choose and document an explicit settings-sharing design before changing permissions or production installation behavior.
3. Continue testing layout switching and normal input lifecycle in the disposable probe.
4. Keep the experimental manager opt-in; do not promote it to production installation until settings synchronization, lifecycle, and broader input behavior are validated.

Do not broaden filesystem permissions or change the production installer to work around the remaining settings issue before the settings source and desired synchronization behavior have been established.

## Runtime test example

From the repository root, with the disposable probe installed:

```bash
SHANTI_FLATPAK_APP_ID=io.github.hasanmuammar.ShantiIbusEngineProbe \
  timeout 5s bash tools/flatpak/ibus-flatpak-host-wrapper.sh \
  2>&1 | tee /tmp/shanti-layout-test.log

grep -E 'layout|failed|error|critical|resolv' /tmp/shanti-layout-test.log
```

The timeout's `Terminated` message by itself is not evidence of a layout failure or success. Inspect relevant log lines and verify behavior by typing text.
