# Experimental IBus-to-Flatpak host-wrapper proof of concept

Status: **prototype only; not production-ready and not end-to-end verified**.

This experiment tests one narrow hypothesis: can host IBus launch Shanti's engine inside an installed Flatpak while the engine connects back to the host IBus session?

It does not install files, edit host IBus configuration, write component metadata, restart IBus, or change any production Flatpak permissions.

## Prototype launcher

`ibus-flatpak-host-wrapper.sh` requires `SHANTI_FLATPAK_APP_ID` to be set explicitly. It launches the sandboxed `ibus-engine-openbangla` command with `--ibus` and requests only the `org.freedesktop.IBus` D-Bus name for that invocation.

Example syntax (replace the placeholder with the actual installed app ID):

```sh
SHANTI_FLATPAK_APP_ID=org.example.Shanti flatpak run --help
SHANTI_FLATPAK_APP_ID=org.example.Shanti bash tools/flatpak/ibus-flatpak-host-wrapper.sh
```

The second command is a direct engine launch smoke test, not a complete IBus integration test. Run it only in a disposable development session where an IBus daemon is active. It may remain running while the engine serves requests; stop it with Ctrl-C. The placeholder ID is not a declared Shanti app ID.

The `--talk-name=org.freedesktop.IBus` option is deliberately narrow, but its effectiveness depends on the installed app's D-Bus policy and Flatpak version. Do not broaden it to the whole session bus to make a test pass. If Flatpak rejects the override or the engine cannot reach the intended IBus service, record the exact error and stop; do not change production permissions on that basis alone.

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
- The engine's data/config path behaviour inside the sandbox has not been validated.
- The engine's D-Bus connection to host IBus has not been verified in a real desktop session.
- Fcitx5 is a separate integration target: its host module ABI requirement is not addressed by this IBus prototype.
- Do not add production filesystem grants, host session-bus permissions, automatic IBus restarts, or installer hooks as part of this experiment.


## Development VM test log

Last updated: 2026-10-11. These are environment-preparation results, not an end-to-end integration result.

Test environment reported by the developer:

- Fedora Workstation 44, GNOME Shell 50.5, Wayland.
- IBus 1.5.34 and Flatpak 1.18.4.
- GCC/G++ 16.2.1, CMake 4.3.0, Qt 5.15.18, Rust/Cargo 1.98.1, and libzstd 1.5.7.
- The checkout is on `experiment/flatpak-ibus` at commit `3f7b705`.

Completed checks:

- The disposable `io.github.hasanmuammar.ShantiFilesystemProbe` Flatpak built, installed, and ran after using `flatpak-builder --disable-rofiles-fuse --user --install --force-clean ...`. All four disposable test-path create/write/read checks passed. The XDG data/config/cache results were under the probe's private `~/.var/app/io.github.hasanmuammar.ShantiFilesystemProbe/` directories; they do not prove general host XDG directory access.
- Wrapper validation rejected an unset `SHANTI_FLATPAK_APP_ID` and malformed value `invalid`, returning exit code 64 in both cases.
- CMake configuration for a native IBus build completed successfully with:
  ```sh
  cmake -S . -B "$HOME/shanti-ibus-build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_IBUS=ON \
    -DENABLE_FCITX=OFF
  ```
  The configured dependencies were IBus 1.5.34, Qt 5.15.18, and libzstd 1.5.7.

Not yet completed:

- The native engine has not yet been compiled, and no `ibus-engine-openbangla` executable has been located in the checkout.
- There is no Shanti Flatpak manifest or confirmed production app ID in this checkout; the installed app is only the disposable filesystem probe.
- The wrapper has not been launched against a real Shanti Flatpak deployment.
- No temporary host IBus component has been registered, and Bangla typing in a host-native application has not been tested.

The initial probe build hit a stale `rofiles-fuse` mount/cleanup error. Unmounting the stale mount did not resolve it; disabling rofiles FUSE allowed the probe build to complete. This workaround was used for the probe only and is not a production build recommendation.

Do not interpret these results as proof of host IBus integration or as authorization to add production filesystem or D-Bus permissions. Continue with native engine compilation, then build a disposable test deployment and complete the end-to-end checklist above.
