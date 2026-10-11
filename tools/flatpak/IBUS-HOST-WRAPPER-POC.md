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

The second command is a direct engine launch smoke test, not a complete IBus integration test. Run it only in a disposable development session where an IBus daemon is active. It may remain running while the engine serves requests; stop it with Ctrl-C. The placeholder ID is not a declared Shanti app ID.

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
