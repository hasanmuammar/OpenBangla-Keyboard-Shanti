# Shanti Project Checklist

**Status last reviewed:** 2026-10-15  
**Development branch:** `develop`  
This checklist distinguishes repository implementation, CI evidence, real-desktop validation, and publication. A green build does not imply that the input method has been functionally tested.

## 1. Project scope

- [x] Linux-focused fork of OpenBangla Keyboard.
- [x] User-local portable installation architecture.
- [x] Separate IBus and Fcitx5 portable builds.
- [x] x86_64 and aarch64 build targets.
- [x] Fork-owned alpha tags use the `shanti-alpha.*` pattern.
- [x] Experimental IBus-only Flatpak has its own app ID: `org.openbangla.Shanti.IBus`.

## 2. Portable installer and runtime

- [x] `tools/install.sh` validates release checksums and archive paths before installation.
- [x] Archive and symlink-containment validation is exercised by package CI.
- [x] Portable package checks validate GUI startup and data discovery.
- [x] Uninstaller deletion allowlist and symlink/mount-boundary safeguards are exercised by package CI.
- [ ] Publish a Shanti alpha release and test the installer against the actual release assets.
- [ ] Test installation and removal on real x86_64 and aarch64 systems.
- [ ] Test actual Bengali input through IBus and Fcitx5 on real desktops.
- [ ] Test custom absolute `XDG_DATA_HOME` end-to-end on both backends.

## 3. Flatpak and GNOME IBus integration

- [x] IBus-only Flatpak manifest exists at `tools/flatpak/shanti-ibus.yml`.
- [x] GitHub Actions builds architecture-specific Flatpak bundles for x86_64 and aarch64.
- [x] Flatpak build is included in the main CI workflow.
- [x] App includes a GNOME-only prompt and tray-menu action for managing host IBus registration.
- [x] Setup writes a component descriptor, Flatpak launcher and GNOME IBus user-service drop-in.
- [x] Setup checks for certain conflicting user drop-ins and refuses to overwrite unmanaged files.
- [x] Setup and GNOME input-source discovery confirmed working in a GNOME session.
- [ ] Repeat setup and input-source discovery on a clean GNOME account.
- [ ] Test existing/conflicting IBus service drop-ins.
- [ ] Test failure recovery if the systemd reload or IBus service restart fails partway through setup/removal.
- [ ] Verify actual Bengali typing in host applications through the Flatpak engine.
- [ ] Verify logout/login and reboot persistence.
- [ ] Verify cleanup before/after Flatpak removal and confirm unrelated user files remain untouched.
- [ ] Decide whether the Flatpak's host-integration permissions are appropriately scoped after real-session testing.
- [ ] Configure a hosted OSTree repository and public signing key before publishing a `.flatpakref`.

## 4. Current CI evidence

Latest observed workflow runs for commit `f91018f0` on 2026-10-15:

- [x] [CI run #323](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/actions/runs/38758748698) passed.
- [x] Flatpak bundle build succeeded on x86_64 and aarch64.
- [x] All configured portable source-build jobs passed.
- [x] [Prebuilt package test run #139](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/actions/runs/38758748633) passed.
- [x] Package tests passed for IBus and Fcitx5 on x86_64 and aarch64.
- [ ] Bengali input and additional GNOME host-integration cases are not proven by these CI runs.

These statuses describe the runs observed on the review date. Check the [Actions page](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/actions) for newer results.

## 5. Release and distribution

- [x] Release workflow exists for tags matching `shanti-alpha.*`.
- [x] Workflow is configured to build four portable artifacts: x86_64/IBus, x86_64/Fcitx5, aarch64/IBus, and aarch64/Fcitx5.
- [x] Workflow is configured to create a prerelease and upload archives/checksums.
- [x] Workflow is configured to create build-provenance attestations.
- [ ] Verify a successful tagged release run and inspect all four archives/checksums/attestations.
- [ ] Publish the first Shanti GitHub Release.
- [ ] Test the normal installer against the published release assets.
- [ ] Publish the Flatpak through a hosted OSTree repository before describing it as installable through a `.flatpakref`.

**Current distribution state:** the GitHub Releases endpoint was empty when checked on 2026-10-15. CI workflow artifacts are available for testing, but they are not a published GitHub Release. The portable installer's configured release tag is in `release-tag.txt`; the CMake project version is in `version.txt`.

## 6. Documentation

- [x] English and Bangla README describe the fork.
- [x] Installation, development, architecture, input-method, troubleshooting and uninstallation guides exist.
- [x] Flatpak guide documents the new in-app GNOME IBus setup and current publishing limitation.
- [x] Documentation distinguishes CI/build evidence from functional desktop verification.
- [ ] Re-review documentation whenever the host integration implementation or its permissions change.

## 7. Release readiness definition

Do not describe Shanti as generally released until:
- [ ] A tagged release run passes and publishes all four portable archives, checksums and expected provenance.
- [ ] The installer succeeds against published assets on x86_64 and aarch64.
- [ ] GUI startup and actual Bengali input are verified through IBus and Fcitx5.
- [ ] Uninstallation preserves unrelated files and handles the intended data/cache choices.
- [ ] Flatpak GNOME registration is tested for conflicts, recovery, session persistence and removal.
- [ ] User-facing installation instructions match the actual published artifacts and supported environments.
