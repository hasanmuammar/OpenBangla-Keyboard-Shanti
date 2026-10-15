# Repository Guidelines

## Project Structure

This is an experimental, Linux-focused fork of OpenBangla Keyboard. C++ application code is organized under `src/`: `engine/` contains common, IBus, Fcitx5, and Rust (`riti`) components; `frontend/` contains the Qt interface; `shared/` contains shared utilities; and `platform/` contains platform support. Keyboard layouts and language data live in `data/`. User-local installation scripts are in `tools/`, fork-specific documentation belongs in `docs/wiki/`, and CI workflows are in `.github/workflows/`. The current experimental Flatpak manifest, build guide, and optional GNOME IBus bridge are in `tools/flatpak/`.

## Build and Development

Build out of tree and enable exactly one input-method backend. For IBus:

```bash
cmake -S . -B build -G Ninja -DENABLE_IBUS=ON -DENABLE_FCITX=OFF
cmake --build build --parallel 2
```

For Fcitx5, set `ENABLE_IBUS=OFF` and `ENABLE_FCITX=ON`. Configuration requires Linux, Qt5, CMake, Ninja, Rust/Cargo, libzstd, and the development files for the selected backend. Do not build inside the source tree.

## Testing

There is no separate top-level unit-test command. GitHub Actions in `.github/workflows/ci.yml` checks source builds, while `package-test.yml` validates release archives and installer behavior. For installer changes, exercise both scripts with `bash tools/install.sh` and `bash tools/uninstall.sh`. After installing, check data discovery with `~/.local/bin/openbangla-gui --check-data`; verify IBus registration with `ibus list-engine | grep -i -A3 -B2 openbangla`. Also test Bangla input in the target desktop/input-method environment when changing engine behavior. The dedicated Flatpak permission and wrapper probe tests were removed; do not recreate those disposable probes or their workflows unless requested. The Shanti-specific Flatpak ID/metadata transformation still needs a successful package rebuild before treating it as verified.

## Code and Documentation Style

Follow the existing conventions in the touched component: C++ uses the project’s CMake configuration and C++17; Rust code is under `src/platform/` and `src/engine/riti/`; shell scripts are in `tools/`. Keep changes focused and use descriptive names consistent with nearby files. Put fork-specific installation and configuration guidance in `docs/wiki/`, not upstream documentation.

## Commits and Pull Requests

Recent history uses concise, imperative commit subjects that describe the actual change (for example, “Stop parsing oversized archives…”). Keep commits similarly direct. Pull requests should explain the user-visible change, list relevant validation commands and results, link related issues when applicable, and include screenshots for frontend or documentation changes where useful. Changes should stay within this fork’s Linux scope; Windows and macOS support are out of scope.

## Scope and Configuration

Use the `develop` branch for fork development. Do not commit local build output, credentials, or machine-specific configuration. Installer and archive changes should preserve the portable, user-local installation model.
