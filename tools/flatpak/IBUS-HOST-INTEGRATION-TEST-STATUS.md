# Experimental Flatpak IBus host integration: test status

This document records the current scope and limits of the experimental host-side IBus integration. It is not a production setup guide.

## Scope and safety

The host wrapper and `manage-ibus-flatpak-host.py` prototype are opt-in experiments. They are not installed by the production installer. Use a disposable Flatpak app/repository and a test account where possible. Do not run `openbangla-gui --setup-system` during baseline testing; it can modify host input-source configuration.

The wrapper requires `SHANTI_FLATPAK_APP_ID`, obtains the host IBus address with `ibus address`, passes the IBus address to the Flatpak engine, and grants access to the IBus socket. It also attempts to expose the host OpenBangla settings directory read-only when it exists. Flatpak may still apply sandbox-specific configuration paths, so setting `XDG_CONFIG_HOME` in the wrapper does not by itself prove that the engine reads the host settings file.

## Verified so far

- The wrapper unit test passes for validation, address discovery, argument forwarding, and signal cleanup.
- In a disposable test account, the experimental host descriptor allowed IBus to discover the test engine; Bengali text entry was manually confirmed.
- The manager's install/uninstall lifecycle was exercised in the disposable account. Uninstall removed the managed host files and the engine disappeared from `ibus list-engine`; the user IBus service remained active.
- A rebuilt disposable probe contains the engine binary, seven layout JSON files, and four data JSON files under `/app/share/openbangla-keyboard`.
- The disposable probe was rebuilt and reinstalled after removing temporary `[SHANTI-DEBUG]` instrumentation. The rebuilt binary contains the layout-path fallback, and Bengali typing was manually confirmed with the National (Jatiya) layout.
- The engine was observed under the IBus-managed process tree. Manually terminating the wrapper while OpenBangla remained selected caused IBus to launch it again; this is expected engine lifecycle behavior, not evidence of an independent service.
- The experimental host descriptor names `/home/hasan/.local/bin/shanti-flatpak-probe-launcher`; that launcher sets `SHANTI_FLATPAK_APP_ID=io.github.hasanmuammar.ShantiIbusEngineProbe` and executes `tools/flatpak/ibus-flatpak-host-wrapper.sh`.

These checks establish engine discovery and basic text entry in the tested environment. They do not validate the full input workflow, production packaging, or reliable synchronization of every GUI setting with a running Flatpak engine.

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
