# Experimental Flatpak IBus host integration: test status

This document records the current scope and limits of the experimental host-side IBus integration. It is not a production setup guide.

## Scope and safety

The host wrapper and `manage-ibus-flatpak-host.py` prototype are opt-in experiments. They are not installed by the production installer. Use a disposable Flatpak app/repository and a test account where possible. Do not run `openbangla-gui --setup-system` during baseline testing; it can modify host input-source configuration.

The wrapper requires `SHANTI_FLATPAK_APP_ID`, obtains the host IBus address with `ibus address`, passes the IBus address to the Flatpak engine, and grants access to the IBus socket. If the host OpenBangla Qt settings directory exists, it passes `XDG_CONFIG_HOME` and grants read-only access to that directory.

## Verified so far

- The wrapper unit test passes for validation, address discovery, argument forwarding, and signal cleanup.
- In a disposable test account, the experimental host descriptor allowed IBus to discover the test engine; Bengali text entry was manually confirmed.
- The manager's install/uninstall lifecycle was exercised in the disposable account. Uninstall removed the managed host files and the engine disappeared from `ibus list-engine`; the user IBus service remained active.
- A rebuilt disposable probe contains the engine binary, seven layout JSON files, and four data JSON files under `/app/share/openbangla-keyboard`.
- With the wrapper running, `org.freedesktop.IBus.OpenBangla` was observed to have an owner. A controlled wrapper termination left no matching engine processes in the follow-up process check.

These checks establish engine discovery and basic text entry in the tested environment. They do not establish that every GUI setting is synchronized to a running Flatpak engine or validate the full input workflow.

## Current layout-settings investigation

The GUI's `Layout::setLayout()` stores the special strings `avro_phonetic` and `khipro` for those layout types, and stores a JSON layout path for other layouts. The IBus engine reads the layout path through `gSettings->getLayoutPath()`.

A temporary local diagnostic build reported:

```text
[SHANTI-DEBUG] configured=avro_phonetic exists=0 layouts_dir=/app/share/openbangla-keyboard/layouts
[SHANTI-DEBUG] resolved=avro_phonetic exists=0 layout_set=1
```

The `exists=0` result for `avro_phonetic` is expected because it is a special Riti layout identifier, not a filesystem path; `layout_set=1` means Riti accepted that identifier. This output does not test the JSON-path fallback.

The host settings file was separately observed to contain:

```ini
[layout]
name=National (Jatiya)
path=/home/hasan/shanti-gui-test-prefix/usr/local/share/openbangla-keyboard/layouts/National_Jatiya.json
```

After changing that file temporarily to the same JSON path and launching the wrapper, the engine still reported `configured=avro_phonetic`. Therefore, the host GUI settings value and the value read by the running engine have not yet been shown to agree. The cause is unresolved; do not describe JSON-layout synchronization as verified.

## Next diagnostic steps

1. Restore any temporary edit to `~/.config/OpenBangla/Keyboard.conf` from its backup.
2. Check the wrapper's `host_config_home`, `XDG_CONFIG_HOME`, and filesystem permission arguments.
3. Add temporary diagnostics for the actual QSettings file location and the values read by `gSettings`; rebuild and run only the disposable probe.
4. Test a JSON-based layout and confirm actual text output after path resolution is correct.
5. Remove temporary `[SHANTI-DEBUG]` instrumentation before committing production code.

Do not broaden filesystem permissions or change the production installer to work around this issue before the settings source has been identified.

## Runtime test example

From the repository root, with the disposable probe installed:

```bash
SHANTI_FLATPAK_APP_ID=io.github.hasanmuammar.ShantiIbusEngineProbe \
  timeout 5s bash tools/flatpak/ibus-flatpak-host-wrapper.sh \
  2>&1 | tee /tmp/shanti-layout-test.log

grep -E 'SHANTI-DEBUG|layout|failed|error|critical|resolv' \
  /tmp/shanti-layout-test.log
```

The timeout's `Terminated` message by itself is not evidence of a layout failure or success. Inspect the diagnostic lines and verify behavior by typing text.
