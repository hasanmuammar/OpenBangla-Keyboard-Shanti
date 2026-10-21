# Repository Guidelines

## Project Overview

Linux-only fork of OpenBangla Keyboard (Qt5 Bengali input method). Remotes: `origin` = this fork (`hasanmuammar/OpenBangla-Keyboard-Shanti`), `upstream` = `OpenBangla/OpenBangla-Keyboard`. All fork development happens on the `develop` branch; Windows/macOS are out of scope.

Layout: `src/engine/` (C++ IBus and Fcitx5 engine plugins, `common/`, plus the `riti` Rust transliteration engine built as a static lib via corrosion), `src/frontend/` (Qt5 GUI → the `openbangla-gui` binary), `src/ibus` engine binary is `ibus-engine-openbangla`, `src/shared/` (shared C++ utilities), `src/platform/` (Rust platform support). Keyboard layouts and language data live in `data/`. Fork-specific docs belong in `docs/wiki/`. The Flatpak manifest and guide are in `tools/flatpak/`, and CI workflows are in `.github/workflows/`.

## Submodules — required before building

Two submodules are build dependencies: `src/engine/riti` and `cmake/corrosion-rs`. A fresh clone or plain `git pull` will NOT build. Always use:

```bash
git pull --recurse-submodules
# or after a fresh clone:
git submodule update --init --recursive
```

## Build and Development

Preferred entry point is `tools/build.sh`; only run manual CMake when diagnosing the build system itself.

```bash
bash tools/build.sh --backend ibus      # or fcitx; default detects from desktop
bash tools/build.sh --backend ibus --package   # release archive in ./dist instead of install
```

Gotchas:

- Default action **builds and installs to the current user's home** — it is not a compile-only step. Use `--package` to only produce the `./dist` archive.
- It may boot a temporary Toolbx/Distrobox/Podman/Docker build environment (prompted unless `-y`); with `-y` it refuses to install Podman or change host packages.
- Build/staging state lives in `${XDG_CACHE_HOME:-$HOME/.cache}/openbangla-keyboard/build`— not in a repo `build/` dir. To force a clean CMake config, remove only that cache path.
- Manual build: enable exactly one backend (`-DENABLE_IBUS=ON -DENABLE_FCITX=OFF` or the reverse), build out of tree with Ninja. In-source builds and both/neither backends are hard errors, as is non-Linux. Deps: Qt5 (Widgets, Network, Svg, DBus), CMake ≥ 3.18, Ninja, Rust/Cargo, libzstd dev, plus ibus-1.0 or Fcitx5Core (≥ 5.0.5) dev headers.

## Generated headers

`src/engine/riti` and `src/platform` are Rust crates whose C headers are cbindgen-generated; `src/platform/include/platform.h` is committed but must not be hand-edited. Regenerate it from `src/platform`:

```bash
cbindgen --config cbindgen.toml --output include/platform.h
```

## Testing and Verification

There is no unit-test suite. Verified checks:

- `.github/workflows/ci.yml` compiles portable IBus/Fcitx5 variants and builds the Flatpak for x86_64 and aarch64. It ignores markdown/asciidoc-only commits.
- `.github/workflows/package-test.yml` validates shell syntax, release archive/checksum integrity, archive paths and symlink containment, and offscreen GUI/data checks.
- Offscreen GUI check that mirrors package-test (no desktop session needed):

```bash
QT_QPA_PLATFORM=offscreen ~/.local/bin/openbangla-gui --version
QT_QPA_PLATFORM=offscreen ~/.local/bin/openbangla-gui --check-data
```

- For installer changes run `bash tools/install.sh` then `bash tools/uninstall.sh`; verify IBus registration with `ibus list-engine | grep -i -A3 -B2 openbangla`.
- Engine behavior changes require actually typing Bangla in a real desktop/input-method session — CI/packaging checks do not cover that, and a successful Flatpak bundle build does not prove host IBus integration works.
- Local Flatpak builds use `tools/flatpak/shanti-ibus.yml` (see its README); `appstreamcli compose` is known to fail on atomic distros (Bluefin, Bazzite) — see the shanti-alpha.1.4 CHANGELOG entry and docs/wiki/Development.md for the fixes.

## Releases

- Pushing a tag matching `shanti-alpha.*` triggers `.github/workflows/release.yml`: x86_64/aarch64 portable archives for both backends plus IBus-only Flatpaks, with checksums and GitHub provenance attestations, attached to a GitHub prerelease.
- Release prep commits touch exactly two files: `CHANGELOG.md` (new `shanti-alpha.X` entry) and `release-tag.txt` (holds the next tag string, e.g. `shanti-alpha.1.4`). `version.txt` stays at the inherited upstream `3.0.0`.
- Pre-releases `shanti-alpha.1` through `shanti-alpha.1.4` are published on GitHub Releases.

## Conventions

- Commit subjects are concise and imperative, describing the actual change (e.g. "Harden Flatpak builds against a broken host appstreamcli compose").
- Keep the portable, user-local installation model: installers target the user's home, never system paths, and release archives must keep bundling `install.sh`/`uninstall.sh` so tarballs install without a source checkout.
- Installer scripts share helper functions via `tools/portable/common.sh`, which ships beside them in the checkout and in every release archive (`install.sh` and `uninstall.sh` both source it). Never re-copy `die`/`log`/path-safety helpers into individual scripts.
- Do not commit local build output, credentials, or machine-specific configuration.
