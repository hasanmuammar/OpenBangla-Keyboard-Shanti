# Flatpak Distribution Experiment — OpenBangla Keyboard Shanti

> **Project status:** Highly Experimental · Vibe-Coded  
> **Phase:** Planning and feasibility assessment  
> **Working branch:** `experiment/flatpak-ibus`  
> **Project goal:** Easy, reliable, safe installation and maintenance of Shanti on its target Linux systems  
> **Experiment:** Evaluate Flatpak as one possible distribution format  
> **Flathub:** Out of scope

## 1. Overview

OpenBangla Keyboard Shanti's broader goal is to make the keyboard easy to install, update, and remove safely on its target Linux systems, while preserving reliable Bengali input and package/download integrity. The project is aimed particularly at **modern atomic Linux operating systems**, including **Bluefin/Dakota, Bazzite, and similar systems**.

**Flatpak is only a candidate distribution format—not the project's main goal and not a predetermined replacement for the existing installer.** This page records a bounded technical investigation to determine whether Flatpak is a useful additional option. The portable, user-local installer remains an existing distribution path and must not be silently replaced or disrupted. A conventional Linux desktop can be used as a comparison environment where practical; compatibility on any target remains unproven until tested.

## 2. Installer GUI requirement

The Flatpak deliverable must include a **dedicated graphical frontend for the installation/maintenance script**, shipped as a compiled application binary. Do not confuse this installer frontend with the existing `openbangla-gui`, which is the keyboard's runtime/configuration application.

The intended user-facing workflow is to launch the Rust + GTK4/libadwaita installer GUI and use it for installation, configuration, update, status, and removal of the host-side Shanti installation. The Flatpak is a lifecycle/configuration frontend, not the runtime keyboard engine itself; the existing script remains responsible for the underlying installation logic. The GUI and script must be packaged together, with a defined and testable interface for passing actions, progress, diagnostics, and exit status. Do not duplicate installation logic in the GUI.

Design assumption: the target host already has its normal input-method framework installed—IBus or Fcitx5. The Shanti Flatpak must not package or replace that framework. The preferred design is for the manager to perform file operations inside its sandbox using narrowly scoped read/write access to the specific user-local Shanti installation and registration paths. `flatpak-spawn --host` is not required merely to copy, verify, back up, or remove files; consider it only if a later refresh operation genuinely cannot be handled through file placement, relogin/restart instructions, or a narrowly scoped D-Bus API.

The frontend's initial scope should be limited to clear, auditable lifecycle actions (install, update, remove, status, and manager-owned configuration), progress/output reporting, and actionable error messages. The selected frontend direction is Rust with GTK4 and libadwaita. A proposed host-operation contract is recorded in [Flatpak-Installer-Protocol.md](./Flatpak-Installer-Protocol.md); it is not yet implemented.

## 3. Why investigate Flatpak?

The reason to investigate Flatpak is to assess whether it can improve distribution and lifecycle management for some users. It is one option alongside the existing portable installer; it should be adopted only if evidence shows it is practical and worth maintaining. An input method is not an ordinary standalone GUI application: its engine, registration files, session services, and desktop input-method framework must work together.

A GUI that launches successfully inside a sandbox is therefore not sufficient evidence that the keyboard works. The key feasibility question is whether a Flatpak-based package can make Bengali typing work reliably in **ordinary host applications**, with permissions that are limited and documented.

No compatibility, security, or maintenance benefit should be assumed before it has been tested.

## 4. Target and scope

### Overall project target

- Make Shanti easier to install, update, and remove safely on modern atomic Linux operating systems, especially Bluefin/Dakota, Bazzite, and similar systems.
- Preserve reliable Bengali input and strengthen installation/package integrity.
- Keep the existing portable installer available unless evidence and a separate decision justify changing that approach.

### This experiment's scope

- Evaluate Flatpak as an optional additional distribution format, not as the project goal.
- If feasible, use GitHub Releases with a hosted Flatpak repository and a `.flatpakref`.

### Validation target

- At least one conventional Linux desktop environment, where available, to identify which behaviours are specific to the primary target systems.

### Input-method backends to investigate

- **IBus:** investigate and test independently.
- **Fcitx5:** investigate and test independently.

Support for one backend must not be taken as proof of support for the other. The project must explicitly document any backend that cannot be made to work.

### Distribution boundaries

- GitHub Releases are the intended distribution channel for this experiment.
- A hosted Flatpak repository must provide the actual application and repository metadata.
- The `.flatpakref` is a reference that tells Flatpak where to find a remote and which application to install; it is not, by itself, the application bundle or repository.
- Repository signing, update behaviour, and removal instructions must be addressed before distribution is considered ready.
- **Flathub submission is not part of this plan.**

## 5. Goals

1. **Serve the overall project goal.** Any packaging format must make installation, updates, removal, and integrity more reliable for the target systems; Flatpak is worthwhile only if it helps achieve that goal.
2. **Establish feasibility first.** Map the current application layout, executable, bundled Qt/runtime libraries, resources, configuration and data paths, and native dependencies before choosing a packaging design.
3. **Keep the existing installer intact.** Do not replace, silently modify, or make Flatpak a prerequisite for the portable build and installer.
4. **Prove host typing works.** Determine how the engine and its registration communicate with the host IBus or Fcitx5 session, and test Bengali typing in host applications.
5. **Use the smallest workable package.** Start with the narrowest proof of concept that answers a real technical question; do not add a full packaging and release pipeline before feasibility is understood.
6. **Minimise sandbox permissions.** Grant only the access supported by demonstrated requirements. Document any unavoidable exceptions.
7. **Make builds reproducible.** If feasible, build from a clean checkout in a controlled environment and automate the relevant checks in GitHub Actions.
8. **Plan safe distribution and maintenance.** Evaluate repository hosting, signing, releases, updates, application removal, and remote removal.
9. **Make claims match evidence.** Keep the package labelled experimental until the stated acceptance checks have been completed.

## 6. Non-goals

This experiment does not currently aim to:

- Submit the application to Flathub.
- Make Flatpak the project's primary goal or assume it should replace the existing portable installer.
- Redesign the application UI.
- Claim support for every Linux distribution or desktop environment.
- Assume that launching the GUI proves input-method integration.
- Add broad host filesystem or session-bus access merely to make a test pass.
- Publish a user-facing Flatpak release before build, host-typing, installation, update, and removal behaviour have been evaluated.

## 7. Current status

### Source-level packaging findings

The following paths and build constraints have been confirmed by source inspection. They are not yet a substitute for running the packaged engine inside Flatpak.

- The IBus target is `ibus-engine-openbangla`, installed under `${LIBEXECDIR}`; with the intended Flatpak prefix this resolves to `/app/libexec/ibus-engine-openbangla`.
- CMake generates `openbangla.xml` from `data/openbangla.xml.in` into the build tree's `data/` directory and installs it to `${DATADIR}/ibus/component`; for a Flatpak prefix this is expected at `/app/share/ibus/component/openbangla.xml`. The generated descriptor must be inspected to ensure its executable path is correct.
- The shared install rules place seven keyboard layout JSON files under `share/openbangla-keyboard/layouts/` and the autocorrect, dictionary, suffix, and regex data files under `share/openbangla-keyboard/data/`.
- On Linux, `src/shared/FileSystem.cpp` first checks for `../share/openbangla-keyboard` relative to the executable directory. For an engine at `/app/libexec/ibus-engine-openbangla`, this resolves to `/app/share/openbangla-keyboard`, matching the bundled resource layout. If that directory is absent, the code falls back to the user's generic XDG data directory, which is not the desired packaged-resource path.
- The top-level CMake configuration unconditionally requires Qt5 Widgets, Network, and Svg and adds the frontend subdirectory even when building with `ENABLE_IBUS=ON`. It also always adds `src/platform`, whose CMake file imports a separate Rust `platform` crate, while `src/engine/CMakeLists.txt` imports the Rust `riti` crate. Thus Rust/Cargo must be available at configure time for the project’s Corrosion integration, even if the intended build target is only `ibus-engine-openbangla`. A target-only build may avoid compiling unrelated targets, but the ordinary project install rules cover the GUI as well; do not assume a partial target build followed by a whole-project install is a valid packaging flow without testing it.
- The existing native staging install used an absolute staging prefix in its generated IBus descriptor. Do not copy that descriptor into a Flatpak build; configure the Flatpak build with `-DCMAKE_INSTALL_PREFIX=/app` and validate the generated descriptor and resource tree after installation.

These findings narrow the next build check, but do not yet prove the runtime contains all required libraries, that the descriptor launches correctly, or that host IBus can communicate with the sandboxed engine.


### Proposed disposable IBus engine probe boundary

For the next build experiment, use the distinct app ID `io.github.hasanmuammar.ShantiIbusEngineProbe`. This is a disposable development identity, not a production application ID and not the existing filesystem-permission probe ID (`io.github.hasanmuammar.ShantiFilesystemProbe`).

The probe should answer only whether the packaged IBus engine can be built and launched from the KDE 5.15-25.08 runtime with its bundled resources. Keep its package boundary to:

- `/app/libexec/ibus-engine-openbangla`
- `/app/share/ibus/component/openbangla.xml`, generated by CMake with the `/app` install prefix
- The engine's bundled runtime resources under `/app/share/openbangla-keyboard/`: seven layouts, the four data JSON files, and the icon referenced by the component descriptor

Do not include the existing Qt configuration GUI, the planned Rust + GTK4/libadwaita installer manager, installer/uninstaller scripts, host wrapper, or host IBus registration in this first engine package. The host wrapper and temporary host component registration are a separate, manual integration test performed only after the packaged engine launches. Do not install the temporary component into the normal user session as part of the package build.

Build with `ENABLE_IBUS=ON`, `ENABLE_FCITX=OFF`, and `CMAKE_INSTALL_PREFIX=/app`. The top-level project still configures the frontend and platform crate, so configuration requires Qt development packages and Rust/Cargo; the engine target depends on the `riti` Rust crate. Because the project has no confirmed engine-only install component, do not run a whole-project install after building only the engine target: that may attempt to install targets that were not built. Instead, stage the engine executable, generated descriptor, and exact resource files explicitly, or build all targets if the manifest intentionally chooses a full-project build. Record which approach is used.

This boundary is a proposal for the disposable engine POC only. It does not establish that the engine can run in the sandbox or connect to host IBus, and it does not change the production app/installer design.

### KDE runtime/SDK probe for the IBus engine POC

The developer checked the KDE 5.15-25.08 runtime and SDK as a candidate build/runtime pair for the separate IBus engine experiment. This is an engine-POC check; it does not select the runtime for the planned Rust + GTK4/libadwaita installer manager.

- Both `org.kde.Platform//5.15-25.08` and `org.kde.Sdk//5.15-25.08` are installed in the user's Flatpak installation.
- The Platform filesystem contains Qt 5 Core (`libQt5Core.so.5.15.18`) and IBus (`libibus-1.0.so.5.0.532`).
- Inside the KDE SDK, CMake, Ninja, Make, and pkg-config are available. pkg-config reports Qt5Core, Qt5Widgets, Qt5Network, and Qt5Svg at 5.15.18, IBus at 1.5.32-rc2, and libzstd at 1.5.7.
- Rust and Cargo were **not** available in the initial KDE SDK shell. Flathub search showed `org.freedesktop.Sdk.Extension.rust-stable//25.08` as an available extension, and installation/setup is being investigated. It has not yet been verified inside the KDE SDK.
- No Flatpak build of the Shanti engine has yet been completed. The next engine-POC milestone is to verify Rust/Cargo availability in the SDK environment, then configure/build with `CMAKE_INSTALL_PREFIX=/app` and inspect the generated component descriptor and bundled resources.

These are environment inventory results only. They do not prove the full project configures in the SDK, that the engine runs inside Flatpak, or that host IBus can discover it or accept Bangla text.


### Completed

- [x] Created the separate `experiment/flatpak-ibus` branch from the validated `develop` revision.
- [x] Recorded the intended GitHub Releases distribution route.
- [x] Recorded that Flathub submission is out of scope.
- [x] Established a staged plan so feasibility is assessed before implementation.

### Not started or not yet verified

- [ ] Source/runtime/resource/configuration path mapping.
- [x] Review the existing installer/uninstaller CLI, its TTY-only choices, release pinning, and host dependencies.
- [x] Review `pmim-ibus` as an input-method packaging precedent and distinguish its Flatpak-contained process from its separately installed host IBus adapter.
- [x] Draft a versioned GUI-to-host-operation contract in [Flatpak-Installer-Protocol.md](./Flatpak-Installer-Protocol.md); it remains a proposal until implemented and tested.
- [ ] Implement the host bridge and non-interactive script support behind tests.
- [ ] Finalise the proof-of-concept filesystem permission model and test in-sandbox writes only to disposable Shanti-specific paths.
- [ ] Select the production manager runtime and SDK; the disposable permission-test manifest currently uses Freedesktop Platform/SDK 26.08.
- [x] Add a disposable Flatpak filesystem-permission test manifest and in-sandbox probe.
- [x] Add a GitHub Actions workflow to build and run the permission probe.
- [ ] Verify the workflow result and correct any runtime/permission issues.
- [ ] Decide and test production path grants based on the actual installer target list.
- [ ] Build the production Rust + GTK4/libadwaita manager manifest.
- [ ] Sandboxed production-manager smoke test.
- [ ] Host IBus integration test.
- [ ] Host Fcitx5 integration test.
- [ ] GitHub-hosted Flatpak repository, signing, and `.flatpakref`.
- [ ] Clean installation, update, and removal tests.
- [ ] Flatpak release publication.

**Current evidence boundary:** The disposable filesystem probe has passed locally on the developer's Fedora VM: all four test roots passed create/write/read checks inside the installed probe Flatpak. This validates only those disposable path grants in that environment. The GitHub Actions result has not been confirmed; the production manager manifest/build, production path grants, modifying installer operation, Rust frontend, and host-typing test remain unvalidated. The project remains experimental.

## 8. Work plan and checklist

Work through the stages in order. Complete and review one focused task at a time; do not treat the checklist as permission to implement every stage at once.

### Stage 1 — Feasibility and design

- [ ] Map the existing executable, bundled runtime/Qt libraries, resources, engine files, configuration paths, and user data paths.
- [ ] Identify native dependencies and how the current installer arranges files.
- [ ] Determine which parts would run inside Flatpak and which must integrate with the host session.
- [ ] Assume the host already provides either IBus or Fcitx5; map the exact user-local engine/module and registration paths for each backend.
- [ ] Assess required host commands or narrow D-Bus interactions to refresh/restart the existing framework, plus environment propagation and host-visible files.
- [ ] Keep the input-method framework out of the Flatpak; decide how the host-compatible Shanti engine payload is installed and registered by the GUI.
- [ ] Select a suitable GNOME runtime/SDK and define initial architecture and test-environment coverage.
- [ ] Record technical blockers and a minimal design before writing a manifest.

**Stage 1 exit condition:** A reviewed design that describes the package contents, host/sandbox responsibilities, required permissions, and a credible test for sending Bengali keystrokes into host applications.

**Next task:** Verify the GitHub Actions run for `tools/flatpak/permission-probe.yml`. The local Fedora VM run passed all four disposable sandbox path checks, but that does not replace CI verification. The workflow triggers on changes to the probe files/README/workflow or via manual dispatch; wrapper-only and PoC-log-only commits do not trigger it. After a confirmed CI run, use the results to refine the production path-grant list.

### Stage 2 — Minimal buildable proof of concept

- [ ] Add a manifest for the smallest target justified by Stage 1, including the compiled installer GUI and script where justified.
- [ ] Build from a clean checkout in a reproducible environment.
- [ ] Verify application startup and resource lookup inside the sandbox.
- [ ] Inspect the resulting package contents and permissions.
- [ ] Add focused build and smoke tests to GitHub Actions.

**Stage 2 exit condition:** A repeatable build and passing sandbox smoke test. This does **not** establish that host input-method integration works.

### Stage 3 — Host input-method integration

- [ ] Test IBus independently in a real desktop session.
- [ ] Test Fcitx5 independently if the feasibility assessment supports it.
- [ ] Verify Bengali typing in ordinary host applications outside the Flatpak sandbox.
- [ ] Check session startup, environment propagation, and behaviour after logout/login or restart.
- [ ] Record results and desktop-specific differences.
- [ ] Remove unjustified permissions and document any necessary exceptions.

**Stage 3 exit condition:** Reproducible host-typing evidence for each backend declared supported. Mark any unverified or unsupported backend clearly.

### Stage 4 — GitHub distribution and lifecycle

- [ ] Define how the Flatpak repository and metadata will be published with GitHub Releases.
- [ ] Configure and verify repository signing.
- [ ] Generate and validate a `.flatpakref` with the correct application ID and repository details.
- [ ] Test installation using only the published instructions on a clean test account/system.
- [ ] Test updating from one build to another.
- [ ] Test removing the application and removing the remote as separate actions.
- [ ] Ensure an interrupted or failed publication cannot silently replace known-good repository metadata.
- [ ] Document prerequisites, supported environments, limitations, updates, and removal.

**Stage 4 exit condition:** A clean test system can install, update, and remove the application through the documented GitHub distribution route.

### Stage 5 — Review and decision

- [ ] Summarise test evidence and known compatibility limits.
- [ ] Review permissions, maintenance burden, signing, update reliability, and user experience.
- [ ] Compare the result with the existing portable installation method.
- [ ] Decide whether to continue, narrow the supported scope, or stop the experiment.

**Stage 5 exit condition:** A decision based on recorded evidence. A GUI launch alone is not grounds to replace the portable installer.

## 9. Acceptance criteria

Do not describe the Flatpak experiment as successful until the relevant claims are backed by recorded test results.

- [ ] Build succeeds reproducibly from a clean checkout.
- [ ] The compiled installer GUI starts in the sandbox and finds its required resources.
- [ ] The GUI invokes the packaged script through a defined interface and reports exit status/errors correctly.
- [ ] Bengali typing works in ordinary host applications for every backend claimed as supported.
- [ ] Required host integration and sandbox permissions are documented and minimised.
- [ ] Installation, update, application removal, and remote removal have been tested.
- [ ] The GitHub-hosted repository metadata and `.flatpakref` are reachable and correctly configured; repository signing is verified.
- [ ] Instructions clearly state supported environments and limitations, and do not imply Flathub availability.
- [ ] Existing portable installation and build workflows remain unaffected.

A checklist item should only be marked complete when there is concrete evidence, such as a CI run, a reproducible test procedure, or recorded manual test results.

## 10. Safety and maintenance constraints

- Keep all Flatpak work isolated on `experiment/flatpak-ibus` until a separate integration decision is made.
- Do not change the existing portable installer as a side effect of this experiment.
- Keep generated files and build outputs in clearly scoped locations.
- Do not use broad cleanup or destructive filesystem operations as a packaging shortcut.
- Avoid permissions that expose unnecessary host files or services.
- Do not publish a release or claim backend support without corresponding evidence.
- Record failures and limitations rather than silently working around them.

## 11. Decision record

| Item | Current decision |
|---|---|
| Project status | Highly Experimental · Vibe-Coded |
| Overall project goal | Easy, reliable, safe installation, updates, removal, and package integrity on target systems |
| Target environments | Modern atomic Linux operating systems, especially Bluefin/Dakota, Bazzite, and similar systems |
| Comparison target | A conventional Linux desktop, where practical |
| Host prerequisite | Existing host IBus or Fcitx5 installation; Shanti Flatpak does not install either framework |
| Input methods | Support/registration for IBus and Fcitx5 assessed separately |
| Flatpak's possible distribution route | GitHub Releases with a hosted Flatpak repository and `.flatpakref`, if the experiment justifies it |
| Flathub | Out of scope |
| Existing portable installer | Retain; no replacement decision has been made |
| Installer frontend | Rust + GTK4/libadwaita compiled binary; lifecycle/configuration GUI, separate from the existing Qt keyboard GUI |
| Host operations | File operations inside the sandbox with narrow user-directory permissions; `flatpak-spawn --host` is optional fallback only |
| Host-operation contract | Drafted in `docs/wiki/Flatpak-Installer-Protocol.md`; lifecycle plan/apply not implemented |
| Read-only host probe | Implemented at `tools/flatpak/host-bridge.sh`; local regression test at `tests/flatpak/test-host-bridge.sh` passed |
| Disposable Flatpak filesystem probe | Manifest, test script and CI workflow added; real sandbox result pending |
| Filesystem access candidate | Narrow permissions to Shanti-owned paths under `~/.local`, selected `~/.local/share` subdirectories, required `~/.config` subdirectories, and the Shanti cache; final grant list must be tested |
| Implementation status | Source review + disposable filesystem-test manifest; no production manifest/build or GUI yet |
| Immediate next task | Verify the CI sandbox test result before finalising production filesystem grants |

## 12. Progress log

| Date | Update |
|---|---|
| 2026-10-10 | Created the isolated experiment branch and initial plan. No Flatpak packaging implementation was added. |
| 2026-10-10 | Expanded the page to clarify targets, scope, non-goals, stage exit conditions, acceptance criteria, and current evidence boundaries. |
| 2026-10-10 | Clarified that Flatpak is only a candidate distribution format; the broader goal is easy, safe installation and maintenance on the target systems. |
| 2026-10-10 | Reviewed established Flatpak host-operation patterns, including ProtonUp-Qt host-spawn use and targeted permissions, Flatseal's permission-provider model, and GTK4/libadwaita lifecycle UI references. Proposed a Rust installer frontend and recorded host-command security/dependency constraints. |
| 2026-10-10 | Added requirement for a compiled installer GUI shipped with the script; distinguished it from the existing keyboard GUI and recorded the sandbox/host boundary as unresolved. |
| 2026-10-10 | Selected Rust + GTK4/libadwaita as the installer frontend direction and reviewed PMIM IBus Flatpak's split sandbox/host adapter model. Documented that PMIM is not a general host-access bypass and still requires separate host-side IBus integration. |
| 2026-10-10 | Clarified the target architecture: IBus or Fcitx5 is already installed on the host; the Flatpak manages Shanti's host-compatible user-local engine payload and registration rather than shipping a private input-method framework. |
| 2026-10-10 | Added `Flatpak-Installer-Protocol.md` with a proposed plan/apply JSONL contract, host-environment/XDG handling, explicit decisions for install and removal, current CLI gaps, and bridge acceptance tests. No script or runtime code was changed. |
| 2026-10-10 | Added the read-only `tools/flatpak/host-bridge.sh` host-context probe and `tests/flatpak/test-host-bridge.sh`; local mocked tests pass, but this probe is not suitable for detecting host processes when run inside the Flatpak namespace. The preferred installer design now uses narrowly scoped host-directory permissions; a separate in-sandbox filesystem probe and minimal-manifest test are still required. No modifying action is implemented. |
| 2026-10-11 | Reassessed the host-command-first design. Narrow, predeclared home-relative filesystem grants are now the default for file placement; `flatpak-spawn --host` is an optional fallback only for a demonstrated host-only refresh/API gap. Updated the installer protocol and clarified the existing host-context probe is not an in-sandbox detector. |
| 2026-10-11 | Added `tools/flatpak/permission-probe.yml`, a disposable sandbox-only file-permission probe and run instructions, plus `.github/workflows/flatpak-permission-probe.yml`. Static YAML-structure and POSIX shell syntax checks passed on local copies. The actual probe has not run here because Flatpak tooling is unavailable; no actual sandbox result has been verified yet. |
| 2026-10-11 | Corrected the XDG mount model: write through sandbox `XDG_*_HOME/<granted-subpath>` paths while using `HOST_XDG_*` only to report host locations. Updated CI to use `actions/checkout@v7` and configure Ubuntu 24.04 user namespaces for Bubblewrap. Static checks passed; the push-triggered Actions result remains unverified in this session. |
| 2026-10-11 | The developer's Fedora VM run of the installed disposable filesystem probe passed all four create/write/read checks. The updated IBus host-wrapper unit test also passed syntax, app-ID validation, missing-command handling, and Flatpak argument forwarding. Neither result proves real host IBus integration; the CI run still needs explicit verification. |
| 2026-10-11 | Completed a source-level packaging-path review: confirmed the engine, generated IBus descriptor, bundled data/layout destinations, Linux relative-resource lookup, and unconditional top-level Qt GUI configure dependencies. Runtime validation is still pending KDE Platform/SDK installation. |

## 13. Related projects and implementation precedents

This is an initial source review of existing projects. Their existence shows that input-method packaging has been attempted with Flatpak or AppImage; it does not establish that Shanti can use the same integration model.

### Fcitx5 Flatpak and engine extensions

- **Main project:** [fcitx/flatpak-fcitx5](https://github.com/fcitx/flatpak-fcitx5)
- **Main manifest:** [org.fcitx.Fcitx5.yaml](https://github.com/fcitx/flatpak-fcitx5/blob/master/org.fcitx.Fcitx5.yaml)
- **Rime extension manifest:** [org.fcitx.Fcitx5.Addon.Rime.yaml](https://github.com/fcitx/flatpak-fcitx5/blob/master/org.fcitx.Fcitx5.Addon.Rime.yaml)
- **Official setup notes:** [Install Fcitx 5 from Flatpak](https://fcitx-im.org/wiki/Install_Fcitx_5/en)

The Fcitx project packages the Fcitx5 framework as a Flatpak and distributes engines as extension packages. The main manifest declares an extension point under `/app/addons`; the Rime manifest is an extension built against that framework and supplies its own dependencies and data. The framework manifest grants session-bus, Wayland/X11 and other explicit permissions, and gives Fcitx its configuration directories.

**Important limitation:** this model packages the framework and its engine extensions together within the Flatpak environment. The Fcitx documentation explicitly warns that a Flatpak Fcitx installation cannot supply the input-method module libraries required by host applications; a suitable host-side input-method module is still needed. Therefore, it is not proof that a separately packaged Shanti engine can be discovered by a host-installed IBus or Fcitx5.

### Fcitx5 Chinese-engine extension example

- **Project:** [Brli/flatpak.fcitx5-mcbopomofo](https://github.com/Brli/flatpak.fcitx5-mcbopomofo)

This repository describes a Flatpak build for the Chinese Bopomofo engine, based on the extension layout from the main Fcitx Flatpak project. Its README demonstrates building an extension into a local Flatpak repository and exporting it as a bundle. This is a useful reference for extension packaging, but it targets the Flatpak Fcitx ecosystem rather than directly proving host integration for a third-party standalone engine.


### Related Flatpak applications and host-integration patterns

These examples are implementation precedents, not a claim that their exact permissions are appropriate for Shanti.

- **[ProtonUp-Qt](https://github.com/DavidoTek/ProtonUp-Qt)** is a Flatpak-distributed installer/updater for compatibility tools. Its Flathub manifest, [`net.davidotek.pupgui2.json`](https://github.com/flathub/net.davidotek.pupgui2/blob/master/net.davidotek.pupgui2.json), grants `--talk-name=org.freedesktop.Flatpak` and several specifically named launcher/data paths rather than simply requesting `--filesystem=host`. Its code contains a concrete `flatpak-spawn --host rm -r …` call for a host-side cleanup case. This demonstrates the supported host-command mechanism in a real installer. Its manifest also grants `--device=all` and has permissions for shell configuration files; these are specific to ProtonUp-Qt's use cases and should **not** be copied into Shanti's manifest.
- **[ProtonPlus](https://github.com/Vysp3r/ProtonPlus)** is a GTK4/libadwaita installer, updater, and remover. Its architecture is a relevant UI and lifecycle reference for a native Linux maintenance app; its current project is primarily Vala, not Rust. Review the current release manifest and actual file access before treating it as a permission precedent.
- **[Flatseal](https://github.com/tchx84/Flatseal)** is a configuration-provider precedent, but for Flatpak permissions rather than keyboard configuration. Its manifest uses targeted read-only access to Flatpak app directories, write access to Flatpak overrides, and named D-Bus interfaces including `org.freedesktop.impl.portal.PermissionStore`; its documentation distinguishes static sandbox permissions from dynamic portal grants. It shows that configuration management can use narrow paths and documented APIs instead of blanket host access.
- **[Warehouse](https://github.com/flattool/warehouse)** is another relevant lifecycle-management UI for Flatpak apps and their user data. It explicitly describes itself as a frontend that facilitates Flatpak operations, rather than replacing Flatpak itself. It is a useful UX reference, but its operations and authorization model are not equivalent to installing a host IBus/Fcitx engine.

Official references: [Flatpak sandbox-permission guidance](https://docs.flatpak.org/en/latest/sandbox-permissions.html), [`flatpak-spawn` command reference](https://docs.flatpak.org/en/latest/flatpak-command-reference.html), and [libflatpak `HostCommand` API](https://docs.flatpak.org/en/latest/libflatpak-api-reference.html).

### Why host-command execution is not the default

Flatpak does not require an unofficial exploit to run a host-side operation. The documented `flatpak-spawn --host` mechanism uses the Flatpak session-bus interface and requires permission to talk to `org.freedesktop.Flatpak`. The corresponding host-command API deliberately lets a trusted application run a command outside its sandbox. This is a supported escape hatch, but it is a **major trust boundary**, not a narrow permission to run only Shanti's installer script: a compromised or misused GUI could launch other commands as the logged-in user. It does not, by itself, grant root access or bypass ordinary Unix user permissions.

For Shanti, the recommended proof of concept is therefore:

1. Build a dedicated Rust + GTK4 + libadwaita installer frontend as the Flatpak's primary binary. Do not confuse it with the existing Qt `openbangla-gui`, which is the keyboard's runtime/configuration application.
2. Keep the existing lifecycle scripts as the authority for archive verification and safe file management, but adapt them so they can run non-interactively inside the Flatpak without depending on host executables.
3. Package the scripts and needed tools in the Flatpak. Grant filesystem access only to the exact user-local target directories required by Shanti. Run copy, backup, update and removal operations inside the sandbox against those exposed paths; do not stage scripts into app data for the purpose of running them on the host.
4. Display `HOST_XDG_*` to identify where data maps on the host, but perform actual file I/O through sandbox `XDG_*_HOME/<granted-subpath>` paths, which Flatpak maps onto the specific host XDG subdirectories. For an unsupported custom XDG path, stop with a clear message instead of opening the absolute host path, writing into an unintended private location, or requesting broad home access.
5. The installed IBus engine and Fcitx5 module must run under the host framework. Keep backend-specific registration separate: the IBus descriptor must point to the host-visible executable; the Fcitx5 module must match the host Fcitx5 ABI. If immediate daemon refresh is unavailable, tell the user to log out and back in or restart the framework manually.
6. Replace terminal-only prompts with explicit, validated operation flags or a structured request/response protocol before wiring the script to a GUI. The GUI must show exact affected paths and choices for migration, replacement, backup and removal.
7. Treat installation, update, configuration, status, and removal as distinct operations. Removal should preserve user data by default; cache/data purging must be an explicit separate choice. Updating the Flatpak frontend itself remains a Flatpak remote operation and is separate from updating the host-installed keyboard.

### Initial permission policy

- Do not request `--filesystem=host`, `--filesystem=home`, `--device=all`, system-bus access, or broad session-bus access just to make the host script work.
- Do not request host-command permission for ordinary file operations. If a future version adds `flatpak-spawn --host`, that capability must be a separate, documented security decision because it can launch arbitrary host commands as the current user.
- Keep host actions fixed to the shipped maintenance scripts and validated operation arguments. Do not provide a generic “run command” or “open terminal as host” feature.
- No `sudo` and no system-wide installation are required for the existing user-local design; do not silently elevate privileges.
- The host IBus/Fcitx5 service is an expected prerequisite, not something this Flatpak should install. Host-side Shanti registration and framework refresh still require validation. The installer frontend's ability to launch does not prove that either engine works in host applications.

**Important evidence boundary:** The projects above demonstrate real Flatpak patterns, and the official Flatpak documentation supports narrow host-directory permissions. No target-system test yet proves that the proposed path grants, engine discovery, ABI compatibility, or framework refresh will work for Shanti.

### PMIM IBus Flatpak: split sandbox/host integration

- **Flathub package:** [`io.github.fm_elpac.pmim_ibus`](https://github.com/flathub/io.github.fm_elpac.pmim_ibus)
- **Application source and installation instructions:** [`fm-elpac/pmim-ibus`](https://github.com/fm-elpac/pmim-ibus)
- **Manifest:** [`io.github.fm_elpac.pmim_ibus.yml`](https://github.com/flathub/io.github.fm_elpac.pmim_ibus/blob/master/io.github.fm_elpac.pmim_ibus.yml)
- **Host IBus setup instructions:** [`doc/安装.md`](https://github.com/fm-elpac/pmim-ibus/blob/main/doc/%E5%AE%89%E8%A3%85.md)

PMIM is a particularly relevant input-method precedent, but it does **not** simply switch off the Flatpak sandbox. Its manifest grants IPC, X11, PulseAudio, network, DRI and creation access to `xdg-run/pmim`; it does not request `org.freedesktop.Flatpak` or use `flatpak-spawn --host` for general host command execution.

Its published installation instructions explicitly say that the IBus interface module (`librush`) must be installed separately. The host IBus component XML then launches the `ibrus` executable with `--flatpak`; the packaged example points to a binary under the Flatpak app's per-application configuration directory, while the manual instructions describe placing the component XML under the host IBus component directory. The shared `xdg-run/pmim` path is the intended communication boundary between the Flatpak app and the host-side adapter. The exact host setup varies by distribution and may require a package or system configuration step.

**What this teaches Shanti:** split the product into an unprivileged Flatpak frontend/data/configuration side and a minimal host-side integration layer when needed. It is a useful pattern for IBus engine execution. However, PMIM's README and installation instructions show that the Flatpak alone is **not** a fully self-contained, one-click host input-method installation. Its manual/AUR/RPM host setup is an explicit prerequisite, so it does not by itself solve Shanti's desired install/update/remove lifecycle.

For Shanti, the default is a third, simpler model: **run the manager and file operations inside the sandbox with narrow permissions to the host user directories that Shanti owns**. No host-command permission is needed merely to place executables, resources, or registration files. A user logout/login or manual input-method restart can be the first-release refresh path.

Only if a tested requirement remains—for example, the project decides that it must refresh the host daemon immediately and cannot do that through a suitable narrow D-Bus API—should we evaluate a host-command fallback using `flatpak-spawn --host`. That is a separate trust decision, not a prerequisite for file placement. A separate host adapter like PMIM's `librush` is another option, but adds its own installation and update lifecycle; Shanti already has a user-local host engine payload, so the adapter route should not be chosen without evidence.

PMIM proves that a Flatpak GUI/data process can participate in an input-method design alongside a host-side engine adapter. It does **not** prove that host registration files, IBus modules, Fcitx5 modules, or a user's `~/.local` installation can be changed automatically from a normal Flatpak without a host integration mechanism.

### IBus Rime AppImage

- **Project:** [hchunhui/ibus-rime.AppImage](https://github.com/hchunhui/ibus-rime.AppImage)

This project packages the Chinese Rime engine as an AppImage. Its documented flow runs the AppImage to install/register the engine, then restarts IBus and adds Rime through the input-method settings. The README notes that some distributions may request administrator authentication. This is a useful comparison for portable packaging, but it is **not a Flatpak solution** and does not meet a requirement to avoid host-side installation or privileged integration.

### What these precedents mean for Shanti

1. The closest Flatpak precedent is the Fcitx5 framework plus extension architecture—not a generic Flatpak application that automatically registers an engine with any host input-method framework.
2. We must distinguish a fully Flatpak-contained input-method stack from an engine intended to integrate with the host's existing IBus/Fcitx5 session. They have different boundaries and compatibility requirements.
3. Shanti's current IBus build installs a component descriptor plus an engine executable. Its Fcitx5 build installs a native module plus metadata. These are not interchangeable packaging shapes, so each backend needs its own feasibility result.
4. The host's IBus or Fcitx5 is an assumed prerequisite. Shanti's Flatpak is a manager for a host-compatible, user-local engine payload—not a replacement input-method framework. Start with explicit file placement under narrowly granted host user directories and require relogin/restart if necessary. Treat `flatpak-spawn --host` or a named D-Bus API as a later enhancement only if a demonstrated workflow needs it.

### Sources reviewed

- Fcitx Flatpak main manifest and extension point: [org.fcitx.Fcitx5.yaml](https://github.com/fcitx/flatpak-fcitx5/blob/master/org.fcitx.Fcitx5.yaml)
- Rime extension manifest: [org.fcitx.Fcitx5.Addon.Rime.yaml](https://github.com/fcitx/flatpak-fcitx5/blob/master/org.fcitx.Fcitx5.Addon.Rime.yaml)
- Fcitx documentation: [Install Fcitx 5](https://fcitx-im.org/wiki/Install_Fcitx_5/en)
- Chinese engine extension example: [flatpak.fcitx5-mcbopomofo](https://github.com/Brli/flatpak.fcitx5-mcbopomofo)
- IBus Rime AppImage: [ibus-rime.AppImage](https://github.com/hchunhui/ibus-rime.AppImage)

### Latest disposable IBus engine probe update — 2026-10-11

The earlier environment-inventory statements above predate the latest local build and are superseded where they conflict with this update. The disposable engine probe has now been built, exported to a dedicated local Flatpak repository, and installed as io.github.hasanmuammar.ShantiIbusEngineProbe on the developer's Fedora Workstation 44 system, using org.kde.Platform//5.15-25.08 and org.kde.Sdk//5.15-25.08.

Confirmed:

- The engine target compiled successfully in the KDE SDK after enabling the Rust stable SDK extension.
- The explicitly staged payload contained 14 files: the engine executable, IBus descriptor, icon, seven layouts, and four data JSON files.
- ldd inside the KDE Platform runtime reported no unresolved shared libraries.
- The minimal disposable Flatpak was exported to a dedicated local repository and installed successfully. This confirms packaging and local installation only.

Launch remains failed/unverified:

- Direct launches with --help and --ibus produced Qt warnings that QCoreApplication::applicationDirPath() was called before a Qt application object was instantiated.
- The engine failed to set its database directory. It fell back to the app-private XDG path under ~/.var/app/io.github.hasanmuammar.ShantiIbusEngineProbe/data/openbangla-keyboard/data; the parent directory existed and was writable, but the final data directory did not exist.
- The engine emitted IBus/GLib criticals because its IBus D-Bus connection was null/not connected. The --ibus launch timed out after 10 seconds (exit status 124).
- Source inspection indicates that src/engine/ibus/main.cpp calls update_with_settings() before creating any Qt application object, while src/shared/FileSystem.cpp uses QCoreApplication::applicationDirPath() to locate bundled Linux resources. The observed warnings and fallback path are consistent with a resource-path initialization-order defect. A source fix has now been committed on `experiment/flatpak-ibus` (`656f0944a26d778171a2f57e438219b7a33ea94f`): `main()` constructs `QCoreApplication` before settings and resource-path initialization. This is a source-level correction only; the developer must pull the commit, rebuild/reinstall the disposable probe, and confirm that database initialization uses `/app/share/openbangla-keyboard/data`. The fix has not yet been runtime-verified.

Next, fix or validate executable-relative resource discovery, rebuild the disposable probe, and verify that database initialization uses /app/share/openbangla-keyboard/data. Then investigate the IBus D-Bus connection through the intended host-wrapper launch path. Do not register a host component or broaden sandbox permissions yet. The package has not been proven to connect to host IBus, appear in the input-method selector, or type Bangla in host applications.

### IBus connection probe follow-up — 2026-10-11

The disposable probe has been rebuilt and reinstalled after the `QCoreApplication` initialization-order fix. The latest launch no longer prints the earlier Qt warnings or database-directory error. The IBus connection failure was then isolated further:

- The wrapper's bare `--command=ibus-engine-openbangla` fails because the binary is installed at `/app/libexec/ibus-engine-openbangla`, while the sandbox's default `PATH` contains only `/app/bin:/usr/bin`.
- Launching the binary by absolute path removes the executable lookup failure, but without an explicit IBus address it still produces IBus/GLib criticals.
- From inside the sandbox, `gdbus` successfully introspected the host IBus daemon using the address returned by host-side `ibus address`. The test used a temporary read-only mount of `~/.cache/ibus`; it therefore proves socket reachability with that mount, not that the mount is necessary.
- Supplying `IBUS_ADDRESS` from `ibus address` to the absolute-path engine launch removed the IBus/GLib criticals. While the process was running, a host-side D-Bus `NameHasOwner` query returned `(true,)` for `org.freedesktop.IBus.OpenBangla`; the captured engine log was empty.

This is evidence that the disposable engine can connect and acquire its expected D-Bus name under the tested launch conditions. It is **not** an end-to-end host IBus integration result: no host component has been registered, the input-method selector has not been checked, and Bangla typing has not been tested. The successful run still included the temporary read-only IBus-cache mount, so rerun without that mount before deciding whether any filesystem grant is needed.

Next, update and test only the experimental wrapper/template to use `/app/libexec/ibus-engine-openbangla` and supply the host IBus address, then repeat the name-ownership check without the cache mount. Keep the production installer, production Flatpak permissions, and normal desktop IBus configuration unchanged until the disposable integration path has passed discovery and text-entry tests.

### Experimental wrapper socket-access update — 2026-10-11

The wrapper and fixed-ID template now pass the host address returned by `ibus address` as `IBUS_ADDRESS`, execute `/app/libexec/ibus-engine-openbangla`, and grant read-only access to the specific Unix socket path extracted from the address. They reject addresses that are not in the expected `unix:path=...` form. This avoids granting access to the entire `~/.cache/ibus` directory. The wrapper unit test now expects the socket-specific `--filesystem=/tmp/ibus-test:ro` argument.

Live disposable-probe observations:

- With `IBUS_ADDRESS` but no filesystem grant, the engine logged IBus/GLib connection criticals.
- With a read-only grant for the entire `~/.cache/ibus` directory plus `IBUS_ADDRESS`, the engine remained running with an empty log and was observed to own `org.freedesktop.IBus.OpenBangla`.
- A five-second run with a read-only grant for the individual socket path exited via timeout (124) with an empty log. A subsequent background run showed the name owned and the engine process running, although an earlier ownership check during another launch had raced and returned `NameHasNoOwner`. Repeated manual tests left stale processes, which were subsequently terminated. The latest cleanup confirmed no engine processes remained and the name was unowned.
- These results support the socket-specific grant as the next experimental configuration, but the wrapper change has not yet been rerun on the VM with the unit test, nor has actual input processing been verified.

No host IBus component has been registered. Engine discovery in the input-method selector, Bangla composition/candidate/commit behavior, focus changes, and session-restart behavior remain untested. Keep this work limited to the disposable probe and experimental wrapper; do not change production installer behavior or production permissions.


### Clean-account host IBus bridge lifecycle — 2026-10-12

This update supersedes earlier statements above that host component registration and live discovery had not yet been tested. The work remains an experimental Fedora-specific proof of concept; the production installer and production Flatpak manifest were not changed.

On Fedora Workstation 44 with GNOME 50.5 (Wayland), IBus 1.5.34, and Flatpak 1.18.4, a separate `shanti-ibus-test` account was created without sudo access. Its user Flatpak installation was initially empty. The KDE 5.15-25.08 runtime was installed from Flathub with normal signature verification. The disposable engine probe was then installed from a copied local OSTree repository at commit `e998c18985bacf9cb64221dbf732412f11f5cb0c84b674bf5bb3912fa98bd3bc`. Because the local repository summary is unsigned, a temporary user-scoped remote with GPG verification disabled was used only for that local test repository; Flathub verification was not disabled.

Confirmed in the clean account:

- The manager installed its component descriptor, launcher, wrapper, and GNOME IBus service drop-in under the test account's home directory. All four files were owned by the test account with expected permissions.
- After reloading and restarting the IBus user service from the test account's own GNOME session, the service was active and `ibus list-engine --name-only` returned `OpenBangla`.
- Running manager uninstall removed all four managed files. The IBus service remained active, and `OpenBangla` was no longer registered.
- The probe app was uninstalled and the temporary unsigned test remote was removed. The test account's remaining Flatpak remote list contained only `flathub`.

Follow-up testing confirmed Bengali text entry in the clean account with the disposable probe and manager-generated host integration. Manager uninstall then removed the four managed files; the IBus service remained active, and `OpenBangla` was no longer registered. The disposable app and temporary local test remote were removed. Bengali typing was also previously confirmed in the primary account using both manual and manager-generated configuration, with the original manual configuration restored afterward.

The seven manager lifecycle unit tests also passed in the local checkout. This remains validation on one Fedora/GNOME environment; candidate selection/commit/backspace/focus testing, coverage on other distributions/desktops, and a portable alternative to the GNOME-specific service drop-in remain open work. Do not integrate the manager into the production installer or broaden production Flatpak permissions based on this result alone.
