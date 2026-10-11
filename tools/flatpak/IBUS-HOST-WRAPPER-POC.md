# Experimental IBus-to-Flatpak host-wrapper proof of concept

Status: **prototype only; not production-ready and not end-to-end verified**.

This experiment tests one narrow hypothesis: can host IBus launch Shanti's engine inside an installed Flatpak while the engine connects back to the host IBus session?

It does not install files, edit host IBus configuration, write component metadata, restart IBus, or change any production Flatpak permissions.

## Prototype launcher

`ibus-flatpak-host-wrapper.sh` requires `SHANTI_FLATPAK_APP_ID` to be set explicitly. It launches the sandboxed `ibus-engine-openbangla` command with `--ibus`, requests permission to talk to `org.freedesktop.IBus`, and requests ownership of `org.freedesktop.IBus.OpenBangla`, which the engine asks the session bus to grant in `--ibus` mode. These narrow grants are a source-informed prototype adjustment; their effectiveness still requires a real end-to-end test.

Example syntax (replace the placeholder with the actual installed app ID):

```sh
SHANTI_FLATPAK_APP_ID=org.example.Shanti flatpak run --help
SHANTI_FLATPAK_APP_ID=org.example.Shanti bash tools/flatpak/ibus-flatpak-host-wrapper.sh
```

The second command is a direct engine launch smoke test, not a complete IBus integration test. Run it only in a disposable development session where an IBus daemon is active. The wrapper runs Flatpak in a separate process group and traps termination to stop the sandbox process group; verify cleanup on the target system before relying on it as a component launcher. The placeholder ID is not a declared Shanti app ID.

The `--talk-name=org.freedesktop.IBus` and `--own-name=org.freedesktop.IBus.OpenBangla` options are deliberately narrow, but their effectiveness depends on the installed app's D-Bus policy and Flatpak version. Do not broaden them to the whole session bus to make a test pass. If Flatpak rejects either override or the engine cannot reach the intended IBus service, record the exact error and stop; do not change production permissions on that basis alone.

## Temporary host component and fixed-ID wrapper templates

Two templates now define the disposable host-launch contract:

- `ibus-host-wrapper-poc.sh.in`: a wrapper template with an explicit app-ID substitution token. It does not depend on an interactive shell exporting `SHANTI_FLATPAK_APP_ID`, which IBus may not inherit.
- `ibus-host-component-poc.xml.in`: a host-side IBus component template whose `<exec>` points to the installed host wrapper. It does not point at `/app/libexec/ibus-engine-openbangla`, because that path exists inside the Flatpak sandbox, not as a host executable.

For this POC, substitute the app ID with `io.github.hasanmuammar.ShantiIbusEngineProbe` and substitute the component's wrapper-path token with the absolute path of the wrapper installed in the disposable test account. Generate the files into a temporary directory first, inspect the results, and check the XML is well-formed before any registration attempt. Do not commit generated files containing a machine-specific home path, and do not install or register these templates on the normal desktop session.

The wrapper itself appends `--ibus` when launching the sandboxed engine, so the host component's `<exec>` contains only the wrapper path; adding `--ibus` to the XML as well would duplicate the argument. The wrapper grants only `org.freedesktop.IBus` talk access and `org.freedesktop.IBus.OpenBangla` name ownership for the launch. These flags are hypotheses to test, not proof that the app's D-Bus policy and Flatpak version permit the required connection.

These are templates, not installed integration files. No host IBus registration, daemon restart, or production permission change has been performed.

To generate and validate concrete files without installing or registering them, run this from the repository root:

```sh
python3 tools/flatpak/prepare-ibus-host-poc.py \
  --app-id io.github.hasanmuammar.ShantiIbusEngineProbe \
  --wrapper-path "$HOME/.local/bin/shanti-ibus-flatpak-poc" \
  --output-dir "$HOME/shanti-ibus-host-poc-generated"

sed -n '1,120p' "$HOME/shanti-ibus-host-poc-generated/shanti-ibus-flatpak-wrapper"
sed -n '1,100p' "$HOME/shanti-ibus-host-poc-generated/openbangla-flatpak-poc.xml"
```

The generator validates the app-ID syntax, substitutes both template tokens, runs `bash -n` on the generated wrapper, parses the component XML, and checks that the XML `exec` resolves to exactly the chosen wrapper path. It shell-quotes the wrapper path for the IBus command field and XML-escapes it. It only writes into the specified output directory; it does not install files, register the component, or restart IBus. The wrapper path may be the planned installation destination and does not need to exist yet.


## Required end-to-end test

A successful direct launch is insufficient. Before this approach can be considered viable, a test deployment and a host-side IBus component entry must be created manually in a disposable account or VM. Install the wrapper into the test account with `install -Dm755 tools/flatpak/ibus-flatpak-host-wrapper.sh "$HOME/.local/bin/shanti-ibus-flatpak-wrapper"` and set `SHANTI_FLATPAK_APP_ID` in the component launch environment. The component's `exec` must point to that installed host wrapper, not to a path inside the Flatpak deployment.

Test in this order:

1. Confirm the wrapper rejects an unset or malformed `SHANTI_FLATPAK_APP_ID` and fails clearly when `flatpak` is unavailable.
2. Confirm the intended Flatpak app is installed and contains `ibus-engine-openbangla`.
3. Run the wrapper with the actual app ID and inspect whether the engine connects to the host IBus daemon without broad session-bus access.
4. Register a temporary IBus component in the disposable environment and confirm the engine appears in the input-method selector.
5. Select it and type Bangla into a host-native text editor; verify composition, candidate selection, commit, backspace, and focus changes.
6. Log out/in or restart the test session, then repeat discovery and text entry.
7. Update and remove the test app and wrapper/component registration; verify there are no stale entries or host configuration changes left behind.

Keep logs and the exact Flatpak/IBus versions with the test result. Do not call the prototype supported until all steps pass on representative desktops.

## Known limitations

- No production app ID or Flatpak manifest is assumed by this script.
- This repository change does not install a host wrapper or component XML.
- The engine's data/config path behaviour at runtime inside the Flatpak has not been validated. The staged install layout is consistent with the engine's relative bundled-resource lookup by source inspection, but the staged engine has not been run.
- The engine's D-Bus connection to host IBus has not been verified in a real desktop session.
- Fcitx5 is a separate integration target: its host module ABI requirement is not addressed by this IBus prototype.
- Do not add production filesystem grants, host session-bus permissions, automatic IBus restarts, or installer hooks as part of this experiment.


## Development VM test log

Last updated: 2026-10-11. These are build and staging results, not an end-to-end integration result.

Test environment reported by the developer:

- Fedora Workstation 44, GNOME Shell 50.5, Wayland.
- IBus 1.5.34 and Flatpak 1.18.4.
- GCC/G++ 16.2.1, CMake 4.3.0, Qt 5.15.18, Rust/Cargo 1.98.1, and libzstd 1.5.7.
- The developer performed these tests on branch `experiment/flatpak-ibus`; the exact source revision used for the latest staged build was not captured in the supplied terminal output.

Completed checks:

- The disposable `io.github.hasanmuammar.ShantiFilesystemProbe` Flatpak built, installed, and ran after using `flatpak-builder --disable-rofiles-fuse --user --install --force-clean ...`. All four disposable test-path create/write/read checks passed. The XDG data/config/cache results were under the probe's private `~/.var/app/io.github.hasanmuammar.ShantiFilesystemProbe/` directories; they do not prove general host XDG directory access.
- The developer pulled commit `e9aec69` and ran `bash tools/flatpak/test-ibus-flatpak-host-wrapper.sh`; it passed for syntax, app-ID input validation, missing-command handling, and exact Flatpak argument forwarding. The test still does not validate sandbox permissions, D-Bus connectivity, IBus discovery, or text entry.
- The native CMake configure completed successfully with IBus 1.5.34, Qt 5.15.18, and libzstd 1.5.7.
- A native Release build completed successfully. The resulting executables were `$HOME/shanti-ibus-build/src/engine/ibus/ibus-engine-openbangla` and `$HOME/shanti-ibus-build/src/frontend/openbangla-gui`; CMake listed the `ibus-engine-openbangla` target.
- `ldd` reported no missing shared libraries for the native engine.
- A separate Release build and install completed under the non-system staging prefix `$HOME/shanti-ibus-stage`, using `-DCMAKE_INSTALL_PREFIX="$HOME/shanti-ibus-stage"`. The staged tree included `libexec/ibus-engine-openbangla`, `share/ibus/component/openbangla.xml`, the keyboard layouts, and dictionary/data files under `share/openbangla-keyboard/`. `ldd` also reported no missing shared libraries for the staged engine.
- The generated staged component descriptor contains absolute paths rooted at `/home/hasan/shanti-ibus-stage`. This is expected for that staging prefix, but demonstrates that the package must be configured with the intended Flatpak prefix (normally `/app`) when building a Flatpak; the staged descriptor must not be copied unchanged into a Flatpak package.
- Running the *build-tree* engine with `--help` did not display help: it entered normal startup and was terminated by the five-second timeout (exit status 124). It logged that `$HOME/.local/share/openbangla-keyboard/data` could not be set as the database directory because that directory did not exist. Source inspection shows Linux resource lookup first checks a bundled `../share/openbangla-keyboard` path relative to the executable and otherwise falls back to the user's generic data directory. The staging tree contains the bundled data in the expected relative layout, but runtime use of that staged layout remains unverified.

Not yet completed:

- No Shanti Flatpak manifest or confirmed production app ID has been identified in this checkout; the installed Flatpak is only the disposable filesystem probe.
- The wrapper has not been launched against a real Shanti Flatpak deployment.
- The staged engine has not been run, and its data/config behaviour at runtime has not been validated.
- No temporary host IBus component has been registered, and Bangla typing in a host-native application has not been tested.
- No login/session restart, upgrade/removal cleanup, or end-to-end IBus discovery test has been completed.

The initial probe build hit a stale `rofiles-fuse` mount/cleanup error. Unmounting the stale mount did not resolve it; disabling rofiles FUSE allowed the probe build to complete. This workaround was used for the probe only and is not a production build recommendation.

Do not interpret these results as proof of host IBus integration or as authorization to add production filesystem or D-Bus permissions. The wrapper unit test is confirmed passing on the development VM. The developer has since confirmed that both `org.kde.Platform//5.15-25.08` and `org.kde.Sdk//5.15-25.08` are installed. The Platform contains Qt 5 Core 5.15.18 and IBus library `libibus-1.0.so.5.0.532`; inside the SDK, CMake/Ninja/Make/pkg-config and Qt5 Widgets/Network/Svg, IBus development metadata, and libzstd are available. Rust and Cargo were missing from the initial SDK shell. Flathub search found `org.freedesktop.Sdk.Extension.rust-stable//25.08`; its installation and availability inside the KDE SDK are not yet verified. Next, verify Rust/Cargo in the SDK, then prepare a disposable Flatpak build configured for `/app` and inspect its component descriptor and resource layout. Only after that should the engine be launched through the wrapper and host IBus discovery/text-entry testing begin in a disposable session.

## Latest disposable engine Flatpak probe — 2026-10-11

This section supersedes earlier statements in this log that no Flatpak engine package had been built. It records the developer's latest terminal output; it is still not an end-to-end IBus integration result.

Build and package results:

- Built the ibus-engine-openbangla target inside org.kde.Sdk//5.15-25.08 with ENABLE_IBUS=ON, ENABLE_FCITX=OFF, the Rust stable SDK extension, and CMAKE_INSTALL_PREFIX=/app.
- Explicitly staged the engine executable, generated component descriptor, icon, seven layout JSON files, and four data JSON files. The staged tree contained 14 files.
- ldd on the staged executable, executed inside org.kde.Platform//5.15-25.08, reported no unresolved shared libraries.
- Built the disposable manifest using org.kde.Platform and org.kde.Sdk version 5.15-25.08. The first flatpak-builder --user --install attempt stalled while Flatpak updated the KDE locale extension; the app was not installed by that attempt.
- Exporting with flatpak-builder --disable-rofiles-fuse --force-clean --repo=... succeeded. Installing from the local repository succeeded. App ID: io.github.hasanmuammar.ShantiIbusEngineProbe; installed size reported as 11.6 MB.
- The manifest command must be /app/libexec/ibus-engine-openbangla; the bare command name failed finalization because the executable is under /app/libexec, not the default command search path.

Launch test findings:

- Running timeout 10s flatpak run io.github.hasanmuammar.ShantiIbusEngineProbe --help and again with --ibus emitted two Qt warnings: QCoreApplication::applicationDirPath: Please instantiate the QApplication object first.
- Both runs logged that setting the database directory failed. The attempted path was the app-private ~/.var/app/io.github.hasanmuammar.ShantiIbusEngineProbe/data/openbangla-keyboard/data. Inspection confirmed that the app-private data/openbangla-keyboard parent existed and was writable, but its data subdirectory did not exist.
- Both direct launches emitted IBus/GLib criticals because the IBus factory received a null/non-D-Bus connection and the bus was not connected. The --ibus run timed out after 10 seconds (exit status 124). This does not demonstrate a healthy engine startup.
- Source inspection of src/shared/FileSystem.cpp shows that Linux bundled-resource lookup depends on QCoreApplication::applicationDirPath(), then checks the sibling ../share/openbangla-keyboard directory before falling back to the generic XDG data directory. The engine's main() called update_with_settings() before constructing any Qt application object. Commit `656f0944a26d778171a2f57e438219b7a33ea94f` on `experiment/flatpak-ibus` now constructs QCoreApplication at the start of main(), before settings and resource-path initialization. This is a source-level fix only; it has not yet been rebuilt and runtime-verified.
- Do not treat clean ldd output or successful package installation as proof that bundled resources are found at runtime or that the engine can connect to host IBus.

Next required work:

1. Pull commit `656f0944a26d778171a2f57e438219b7a33ea94f`, rebuild the engine, and reinstall the disposable probe.
2. Confirm the database path resolves to `/app/share/openbangla-keyboard/data` and the database initialization error disappears. If the Qt warnings remain, stop and inspect the actual binary/source revision.
3. Separately diagnose the IBus connection under the intended host-wrapper/component launch path. Do not register a host component until the data-path issue is fixed and the exact D-Bus failure is understood.
4. Do not broaden session-bus permissions, add host filesystem grants, or modify production installer behavior based only on this test.

The host wrapper, host component registration, IBus discovery, and Bengali typing in a host-native application remain untested.

## Runtime connection probe update — 2026-10-11

The developer rebuilt and reinstalled the disposable `io.github.hasanmuammar.ShantiIbusEngineProbe` after the `QCoreApplication` initialization-order fix. The earlier Qt warnings and app-private database-directory initialization error no longer appear in the latest direct launch. This supports the resource-path fix, although the database path itself was not printed in the supplied output.

The host wrapper as currently written still fails at executable lookup: `bwrap: execvp ibus-engine-openbangla: No such file or directory`. The executable is present at `/app/libexec/ibus-engine-openbangla`, but the sandbox `PATH` is `/app/bin:/usr/bin`; therefore the wrapper's bare `--command=ibus-engine-openbangla` is not resolvable. A direct launch using `--command=/app/libexec/ibus-engine-openbangla` bypasses this lookup failure.

Without an explicit `IBUS_ADDRESS`, that direct launch emits IBus/GLib criticals indicating that `ibus_bus_new()` did not yield a connected bus. A separate `gdbus call --address "$(ibus address)" ... Introspect` from inside the sandbox succeeded when the host IBus cache was temporarily exposed read-only, proving that the host IBus socket is reachable under that test setup. Adding `--env="IBUS_ADDRESS=$(ibus address)"` to the direct launch removed the critical messages. While that launch was running, a host-side `NameHasOwner` query for `org.freedesktop.IBus.OpenBangla` returned `(true,)`; the process remained alive and its captured log was empty.

This validates that the disposable engine can acquire the expected D-Bus name when launched with an absolute executable path and an explicit IBus address. It does **not** validate IBus component registration, engine discovery in the selector, or Bangla text entry. The current wrapper/template has not yet been updated to pass the explicit address and use the absolute executable path. The temporary `--filesystem="$HOME/.cache/ibus:ro"` override was used in the successful test; whether it is actually required with the explicit address has not yet been isolated, so do not make it permanent based on this result.

Next: update and unit-test the experimental wrapper/template for absolute command resolution and IBus address discovery; rerun the name-ownership test without the temporary cache mount; then proceed to host component registration only in a disposable session. Production installer behavior and production permissions remain unchanged.

## Wrapper implementation update — 2026-10-11

The experimental launcher and its fixed-app-ID template now:
- obtain the host IBus address with `ibus address` and pass it to the Flatpak process as `IBUS_ADDRESS`;
- invoke the engine by its absolute sandbox path, `/app/libexec/ibus-engine-openbangla`, because `/app/libexec` is not in the default sandbox `PATH`;
- retain the narrowly scoped `org.freedesktop.IBus` talk and `org.freedesktop.IBus.OpenBangla` own-name options;
- fail clearly if `ibus` is unavailable or address discovery fails.

The wrapper unit test mocks `ibus address` and checks the explicit address, socket-specific read-only filesystem grant, absolute engine command, and signal cleanup against a mock child process. The wrapper and fixed-ID template extract the socket path from a `unix:path=...` address and grant access only to that socket, rather than the full `~/.cache/ibus` directory. The cleanup test does not prove that every Flatpak/bwrap version terminates all sandbox descendants; confirm this with the disposable live probe.

Live disposable-probe observations on 2026-10-11:
- With `IBUS_ADDRESS` but no filesystem grant, the engine logged IBus/GLib connection criticals.
- With the cache directory mounted read-only, the engine remained running with an empty log and was observed to own `org.freedesktop.IBus.OpenBangla`.
- A five-second run with the individual socket path mounted read-only exited through `timeout` (124) with an empty log. A background run later showed the name owned while the engine process was running. Some ownership checks raced startup and returned `NameHasNoOwner`; repeated tests left processes running, and these were subsequently cleaned up. The latest cleanup confirmed no engine processes and no name owner.
- The socket-specific grant is therefore the next experimental configuration to validate through the updated wrapper. The updated unit test and live wrapper launch still need to be run on the developer VM.

No host IBus component has been registered. Engine discovery, actual Bangla input, candidate selection, commit, backspace, focus changes, and session-restart behavior remain untested. Keep this work limited to the disposable probe and experimental wrapper; production installer behavior and production permissions remain unchanged.
