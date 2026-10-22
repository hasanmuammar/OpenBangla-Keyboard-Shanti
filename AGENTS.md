# Repository Guidelines

## Project Overview

Shanti Keyboard — a Linux-only fork of OpenBangla Keyboard (Qt5 Bengali input method), rebranded from the pre-rebrand "OpenBangla Keyboard Shanti" naming. Remotes: `origin` = this fork (`hasanmuammar/OpenBangla-Keyboard-Shanti`), `upstream` = `OpenBangla/OpenBangla-Keyboard`. All fork development happens on the `develop` branch; Windows/macOS are out of scope.

Layout: `src/engine/` (C++ IBus and Fcitx5 engine plugins, `common/`, plus the `riti` Rust transliteration engine built as a static lib via corrosion), `src/frontend/` (Qt5 GUI → the `shanti-gui` binary), `src/engine/ibus` engine binary is `ibus-engine-shanti`, `src/shared/` (shared C++ utilities), `src/platform/` (Rust platform support), `crates/` (std-only Rust crates for settings (`shanti-config`), path discovery (`shanti-paths`), the shared engine state machine (`shanti-core`) and keysyms (`shanti-keysyms`); test with plain `cargo test` per crate; their CMake import targets use underscores — `shanti_config`, `shanti_paths`… — because corrosion maps crate-name dashes to underscores). Keyboard layouts and language data live in `data/`. Fork-specific docs belong in `docs/wiki/`. Flatpak manifests (IBus app `org.shanti.ShantiIbus` + self-contained Fcitx5 app `org.shanti.ShantiFcitx5`, which builds the whole Fcitx5 stack itself on the same Qt5 runtime; the retired addon manifest `fcitx-addon.yml` is gone) and the user guide are in `tools/flatpak/`, and CI workflows are in `.github/workflows/`. The adopted `docs/REWRITE-PLAN.md` is the phased roadmap for the eventual Rust (core) + GTK4/libadwaita (GUI) rewrite — consult it and its decision log before any rewrite-adjacent work.

## Naming map (after the 2026-10 rebrand)

- Binaries: `shanti-gui` (Qt GUI), `ibus-engine-shanti` (IBus engine); Fcitx5 addon id `shanti` (module `shanti.so`).
- IBus bus/component: `org.freedesktop.IBus.Shanti`; engine name "Shanti"; Fcitx5 input-method display name "Shanti Keyboard".
- Flatpaks: IBus app `org.shanti.ShantiIbus`; Fcitx5 app `org.shanti.ShantiFcitx5` (self-contained: `fcitx-app.yml` builds the whole Fcitx5 stack on the same Qt5 runtime as the IBus app — pinned `xcb-imdkit` 1.0.9, `libuv` 1.53.0, `json-c` 0.16, `fcitx5` 5.1.14 (newest fcitx5 that still supplies `stringutils.h` transitively via `standardpath.h` for configtool 5.1.x; 5.1.15+ instead requires `PlasmaWaylandProtocols` >= 1.20, absent from the Qt5 SDK), `fcitx5-qt` 5.1.16 with `-DENABLE_QT6=Off`, `fcitx5-configtool` 5.1.6 with `-DUSE_QT6=Off` (last configtool line with Qt5/KF5 support) — and ships the same `shanti-gui` GUI; module pins upgrade = bump tags + fix commit SHAs via `git ls-remote`; the app owns Fcitx5's session bus and must not run alongside Flathub's Fcitx5 app).
- User data: `~/.local/share/shanti-keyboard`; settings store `~/.config/Shanti/Shanti.conf` (`QSettings("Shanti", "Shanti")`); bundled data `<exe_dir>/../share/shanti-keyboard`; build cache `~/.cache/shanti-keyboard`.
- Release assets: Flatpak bundles `shanti-ibus-<arch>.flatpak` and `shanti-fcitx5-<arch>.flatpak` (+ `.sha256`, both arches; portable `.tar.gz` archives are retired as release assets — older pre-releases still carry theirs, which `tools/install.sh --version` can still fetch); installer env override `SHANTI_VERSION` remains for from-checkout portable installs.
- Legacy (pre-rebrand) names — `openbangla-keyboard` data tree, `~/.config/OpenBangla/Keyboard.conf`, `ibus-engine-openbangla`, `openbangla-gui`, `org.openbangla.Shanti.IBus`, `~/.cache/openbangla-keyboard`, env.d `90-openbangla-ibus.conf` — are still recognized by the installers/uninstaller and migrated on first run, but every NEW artifact uses the Shanti names. Keep legacy allowlist/migration references intact wherever they exist (marked `legacy`).

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
- It may boot a temporary Toolbx/Distrobox/Podman/Docker build environment (prompted unless `-y`); with `-y` it refuses to install Podman or change host packages. The builder container is named `shanti-builder` (a leftover `openbangla-builder` container from a pre-rebrand checkout is not reused; delete it manually).
- Build/staging state lives in `${XDG_CACHE_HOME:-$HOME/.cache}/shanti-keyboard/build` — not in a repo `build/` dir. To force a clean CMake config, remove only that cache path.
- Manual build: enable exactly one backend (`-DENABLE_IBUS=ON -DENABLE_FCITX=OFF` or the reverse), build out of tree with Ninja. In-source builds and both/neither backends are hard errors, as is non-Linux. `-DBUILD_FRONTEND=OFF` skips the Qt GUI (engines are Qt-free; both Flatpak manifests keep the GUI on so both bundles ship the same `shanti-gui`). Deps: Qt5 (Widgets, Network, Svg, DBus), CMake ≥ 3.18, Ninja, Rust/Cargo, libzstd dev, plus ibus-1.0 or Fcitx5Core (≥ 5.0.5) dev headers.

## Generated headers

`src/engine/riti` and `src/platform` are Rust crates whose C headers are cbindgen-generated; `src/platform/include/platform.h` is committed but must not be hand-edited. Regenerate it from `src/platform`:

```bash
cbindgen --config cbindgen.toml --output include/platform.h
```

## Testing and Verification

Unit tests live in the Rust crates (`cargo test` per crate in `crates/*`); shell changes are covered by `bash -n` and package-test.

- `.github/workflows/ci.yml` compiles portable IBus/Fcitx5 variants and builds both Flatpaks (IBus app + Fcitx5 app) for x86_64 and aarch64, gcc only. It ignores markdown/asciidoc-only commits.
- `.github/workflows/package-test.yml` validates shell syntax, release archive/checksum integrity, archive paths and symlink containment, and offscreen GUI/data checks.
- Offscreen GUI check that mirrors package-test (no desktop session needed):

```bash
QT_QPA_PLATFORM=offscreen ~/.local/bin/shanti-gui --version
QT_QPA_PLATFORM=offscreen ~/.local/bin/shanti-gui --check-data
```

- For installer changes run `bash tools/install.sh` then `bash tools/uninstall.sh`; verify IBus registration with `ibus list-engine | grep -i -A3 -B2 shanti`.
- Engine behavior changes require actually typing Bangla in a real desktop/input-method session — CI/packaging checks do not cover that, and a successful Flatpak bundle build does not prove host IBus/Fcitx integration works.
- Local Flatpak builds use `tools/flatpak/shanti-ibus.yml` and `tools/flatpak/fcitx-app.yml` (see that directory's README); the Fcitx5 one needs `--default-branch=stable` explicitly (the local default branch is `master`, CI/release use `stable`). `appstreamcli compose` is known to fail on atomic distros (Bluefin, Bazzite) — see the shanti-alpha.1.4 CHANGELOG entry and docs/wiki/Development.md for the fixes. Every CI push, including PR branches, builds both Flatpaks and uploads them as downloadable artifacts (`shanti-ibus-flatpak-x86_64`, `shanti-fcitx5-flatpak-x86_64`, `-aarch64`); the README documents testing a PR build with `gh run download` — this is the way to validate engine changes on a real desktop before release. Local builder gotcha: `flatpak-builder`'s state dir must be on the same filesystem as the target dir (pass `--state-dir` under `/tmp` if the clone lives elsewhere).

## Releases

- Pushing a tag matching `shanti-alpha.*` triggers `.github/workflows/release.yml`: x86_64/aarch64 Flatpak bundles for both apps (IBus `org.shanti.ShantiIbus` via `shanti-ibus.yml`, Fcitx5 app `org.shanti.ShantiFcitx5` via `fcitx-app.yml` with `--default-branch=stable`), with checksums and GitHub provenance attestations, attached to a GitHub prerelease. Releases are Flatpak-only; no portable archives.
- Release prep commits touch exactly two files: `CHANGELOG.md` (new `shanti-alpha.X` entry) and `release-tag.txt` (holds the next tag string, e.g. `shanti-alpha.1.5`). `version.txt` stays at the inherited upstream `3.0.0`.
- Pre-releases `shanti-alpha.1` through `shanti-alpha.1.4` are published on GitHub Releases (under the pre-rebrand naming).

## Conventions

- Commit subjects are concise and imperative, describing the actual change (e.g. "Harden Flatpak builds against a broken host appstreamcli compose").
- Keep the portable, user-local installation model: installers target the user's home, never system paths, and release archives must keep bundling `install.sh`/`uninstall.sh` so tarballs install without a source checkout.
- Installer scripts share helper functions via `tools/portable/common.sh`, which ships beside them in the checkout and in every release archive (`install.sh` and `uninstall.sh` both source it). Never re-copy `die`/`log`/path-safety helpers into individual scripts.
- Do not commit local build output, credentials, or machine-specific configuration.
