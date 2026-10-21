# Shanti Rewrite Plan — Rust core, GTK4/libadwaita GUI

**Status:** Adopted roadmap for future implementation. Not started; phases land incrementally and each phase must leave `develop` shippable.
**Last reviewed:** 2026-10-22
**Development branch:** `develop`
**Related docs:** [Project Checklist](PROJECT-CHECKLIST.md) · [Development wiki](wiki/Development.md) · `.github/workflows/` CI contracts

---

## 1. Goal

Rewrite the fork so that Bengali text processing, configuration, platform-glue logic, and the desktop GUI are all maintained in Rust with a modern GTK UI:

1. **C++ → Rust:** Eliminate the C++ codebase (~10k lines: engine plugins, shared utilities, Qt frontend) in favor of Rust crates, building on the ~5.3k-line Rust foundation that already exists in `src/engine/riti/`.
2. **Qt5 → GTK4/libadwaita:** Replace the Qt frontend (`openbangla-gui`) with a GTK4 + libadwaita application pinned to the newest GNOME runtime available on Flathub at implementation start.

Non-negotiable invariants carried across the entire rewrite:

- Linux-only scope (Windows/macOS upstream code stays deleted).
- Portable, user-local installation model (`~/.local` targets, bundled install/uninstall scripts, exact uninstall allowlist paths).
- User data compatibility: settings store location/format, custom layout discovery, autocorrect data, and the `~/.local/share/openbangla-keyboard` data layout — see §6 Compat Gates.
- Both IBus and Fcitx5 portable builds plus the IBus-only Flatpak keep building on x86_64 and aarch64 with green CI, releasing as provenance-attested `shanti-alpha.*` prereleases.
- Real-desktop Bengali typing validated for every engine-visible change (CI cannot prove this; see [Project Checklist §2](PROJECT-CHECKLIST.md)).

## 2. What we are starting with (verified facts)

| Component | Language | Size (2026-10) | Role |
|---|---|---|---|
| `src/engine/riti/` (git submodule) | Rust | ~5.3k lines | The real engine: phonetic algorithm, khipro parser, suggestions, keycodes, config model. Exposes a C API via cbindgen (`include/riti.h`). |
| `src/engine/ibus/main.cpp` + `keycode.h` | C++ | ~545 lines | IBus engine executable (`ibus-engine-openbangla`). Thin glue: libibus preedit/lookup-table UTF-8 text calls, keysym→riti map, pushes the current settings into the session-start `riti_config_set_*` block. |
| `src/engine/fcitx/openbangla.cpp` + `keycode.h` | C++ | ~690 lines | Fcitx5 addon module (`libopenbangla.so`). **Already Qt-free**: reads the same settings store (`$XDG_CONFIG_HOME/OpenBangla/Keyboard.conf`) with Fcitx's own INI parser and `StandardPath` discovery. |
| `src/frontend/` (main, TopBar, KeyboardWidget, SettingsDialog, About/AutoCorrect dialogs, LayoutViewer, FlatpakIbusIntegration, PlatformConfig, SingleInstance, AvroPhonetic, …) | C++/Qt5 | ~4.9k lines + 6 `.ui` files | `openbangla-gui`: floating top bar, settings, layout viewer, GNOME IBus registration panel (fork-specific), desktop-backend auto-setup. |
| `src/shared/` (Settings, FileSystem, Log) | C++/Qt5 | ~500 lines | Settings store: **QSettings** (`QSettings("OpenBangla", "Keyboard")` → `$XDG_CONFIG_HOME/OpenBangla/Keyboard.conf`, INI format). Path- and user-dir discovery helpers. |
| `src/platform/` | Rust | small | Platform support crate, cbindgen C header (committed). |
| `data/`, `src/engine/riti/data/` | JSON | — | Layouts (v3 transliteration format), dictionary, suffix, regex, autocorrect. **Not rewritten.** |

Key consequences of these facts:

- **The engines already are "Rust underneath".** The C++ in them is (a) the IM-platform API surface, (b) ~140-entry keysym translation tables, (c) settings plumbing. There is no hidden mass of business logic to port — it lives in `riti`.
- **The real rewrite volume is the frontend.** ~4.9k lines of Qt, including widgets upstream redesigned in 2026 (PR #475) and fork-only panels (GNOME IBus integration, `PlatformConfig` auto-setup).
- **Only the IBus engine links Qt today.** `src/engine/ibus/CMakeLists.txt` links `libShared Qt5::Core`; the Fcitx5 module is already Qt-free because it re-reads the settings file itself (fcitx `readFromIni`). Phase 1 therefore shrinks to de-Qt-ing the IBus engine — still valuable: one less duplicated settings parser.
- **Upstream merge compatibility decays with every phase.** The fork already manages three known upstream-merge conflict classes (Windows-engine deletions, frontend CMake source lists, SettingsDialog semantics); see §7 for the explicit divergence policy, which must be a conscious decision, not a drift.

## 3. Target architecture

```
~/.local/share/openbangla-keyboard/            (unchanged user-data layout)
├── layouts/                                   (unchanged JSON layouts)
~/.local/share/openbangla-keyboard/            (unchanged user-data layout)
├── layouts/                                   (unchanged JSON layouts)
└── data/                                      (unchanged)
~/.config/OpenBangla/Keyboard.conf             (settings store — QSettings "OpenBangla"/"Keyboard" INI; path unchanged)

workspace (new top-level Rust workspace, once the submodule constrains us)
├── crates/shanti-config       — Rust port of shared/Settings: parses the QSettings INI store,
│                                schema + defaults, unit-tested against real files
├── crates/shanti-paths        — Rust port of shared/FileSystem: PROGRAM/USER dir
│                                discovery, XDG rules, logging via `tracing`
├── crates/shanti-keysyms      — platform keysym → riti VirtualKey tables, single
│                                source, code-generated for both engines (§5 phase 2)
├── crates/shanti-core (or vendored riti) — engine core; ownership decision in §4
├── ibus-engine-shanti         — Rust engine via libibus (gir bindings) or an FFI core
│                                behind the existing C++ wrapper; decided in phase 2
├── fcitx5-shanti              — Rust core behind the C++ addon shell (phase 3 limit)
└── openbangla-gui             — GTK4/libadwaita app (gtk4-rs + libadwaita-rs)
```

Build-system direction: pure Cargo for everything Rust; CMake remains only for the C++ shells that still exist (the fcitx5 addon during the transition). The `corrosion` glue disappears when the last C++ consumer goes — expected at phase 5 completion.

## 4. Decision 0 — who owns the engine core (must be decided before phase 1)

`riti` is an **upstream git submodule** pinned to `OpenBangla/riti` master. Every phase below assumes the Rust crates can evolve on the fork's schedule, so:

- **Option A (recommended): own our riti.** Vendor riti into the workspace as `crates/shanti-core`, keep the license header chain, and treat upstream's riti as an occasional reference for algorithm fixes. Keep `include/riti.h`'s C API as a stable FFI boundary during phases 1–3; later phases may keep or retire that C API.
- **Option B: stay on the upstream submodule.** This blocks schema evolution whenever riti's C API moves (upstream controls it) and limits the GUI rewrite to typed Rust access across a C ABI. Only viable if we accept upstream's feature cadence as ours.

Record the choice — and the vendored commit hash — in the decision log (§9) when made.

## 5. Phases

Each phase has acceptance criteria (AC). Phases are sized so each can ship independently as a `shanti-alpha.*` prerelease.

### Phase 1 — Make settings Qt-free (engines first)

The smallest Rust-first milestone with immediate payoff: the engines stop linking Qt. *(Narrowed on 2026-10-22: inspection showed the Fcitx5 module is already Qt-free — it re-implements settings reading with Fcitx's own INI parser. Phase 1 de-Qt's the IBus engine only; phase 3 replaces the Fcitx settings re-implementation with the shared crate.)*

**Status: in progress on branch `rust-rewrite`.** Tasks:

- [x] `crates/shanti-paths` — Rust port of the `FileSystem.cpp` Linux contract: XDG base-dir discovery, bundled-vs-user data-dir resolution (`linuxDataDirectory()` semantics), settings-file location, user-dir bootstrap; unit-tested.
- [x] `crates/shanti-config` — read-only QSettings INI parser with typed getters matching the exact defaults in `src/shared/Settings.cpp`; unit-tested against a fixture generated by real Qt5.
- [x] Rewire `src/engine/ibus/main.cpp` off Qt (`QCoreApplication`, `QStandardPaths`, `QFileInfo`, `QString`/`QDir` → `std::filesystem` + the new crates' C ABI).
- [x] CMake: root imports the crates via corrosion; IBus engine no longer links `libShared`/`Qt5::Core`; Fcitx5 module unchanged (already Qt-free).
- [x] package-test.yml: engine-binary Qt-free assertion (archive validation); CHANGELOG entry.
- [ ] Real-desktop Bengali typing check through IBus (agent cannot do this — flagged for the maintainer).

**AC:** package-test gains a step proving `lddtree`/`ldd` of the portable archive's `ibus-engine-openbangla` contain **no Qt5 libraries**; `--check-data` and offscreen GUI checks stay green (GUI still builds with Qt); no regression in real-desktop Bengali typing.

### Phase 2 — IBus engine in Rust

Strangler-fig order: core first, wrapper stays until the port is proven.

1. **Keysym tables single-sourced.** The per-platform `keycode.h` tables (ibus and fcitx) hold ~140 duplicated entries mapping platform enums → riti `VC_*`. Generate both engines' tables from one Rust source (`crates/shanti-keysyms`), emitting (a) the Rust map and (b) a tiny C header via cbindgen for any C++ shell still standing. (This codifies the known intentional duplication into build-time generation.)
2. **Binding strategy for libibus** (scout, then pick — record in the decision log):
   - *Preferred if annotations suffice:* gir-generated glib-rs bindings for `ibus-1.0`. libibus is GObject-introspectable — verify coverage of `IBusEngine` and the text/property APIs our plugin uses (preedit, lookup table, auxiliary text, commit).
   - *Fallback:* shrink the existing C++ `main.cpp` by moving all logic (candidates, preedit composition, riti communication, settings refresh) into Rust behind a tiny C ABI. The wrapper shrinks toward irrelevance and dies in phase 5.
   - *Not recommended:* hand-rolling the IBus D-Bus protocol with `zbus` — far more protocol surface to re-implement for equal functionality.
3. Implement engine behaviors in Rust: preedit (`update_preedit_text_with_mode`), lookup table (`update_lookup_table_fast` — preserve today's suggestion → preedit/lookup-table paging semantics), auxiliary text, commits, session-start config refresh, key filtering.
4. Ship `ibus-engine-shanti` in parallel with the C++ engine (distinct engine descriptor, opt-in install path); run the real-desktop Bengali typing matrix (IBus × GNOME/other DEs, x86_64/aarch64) before flipping the default.

**Task list:**
- [ ] Keysym tables generated from `crates/shanti-keysyms` (single source; cbindgen C header emitted only if a C++ shell still needs it).
- [ ] Scouting write-up for libibus GIR coverage, recorded in the decision log.
- [ ] Rust engine implementation (preedit, lookup table, auxiliary, commits, config refresh, key filtering).
- [ ] Parallel-run `ibus-engine-shanti` descriptor + opt-in packaging.
- [ ] Real-desktop typing matrix; default flip decision recorded.

**AC:** the Rust engine passes the same on-device validation as the C++ engine over ≥ 2 desktop sessions; release archives can install either engine; the Fcitx5 portable build stays untouched but green.

### Phase 3 — Fcitx5: Rust core, C++ shell (documented hard limit)

Fcitx5's addon API is a C++ ABI interface; a pure-Rust addon shell is not supported there and fighting the C++ ABI is not worth it for a ~250-line shell. The plan:

1. Replace the Fcitx module's own settings re-implementation (it currently duplicates `Settings.cpp` semantics via fcitx's `readFromIni`) with `crates/shanti-config`, and its `StandardPath` data discovery with `crates/shanti-paths`.
2. Move all remaining logic out of `openbangla.cpp` into the Rust core behind a C ABI (config application, keysym table use via the phase-2 generator, suggestion handling).
3. The C++ shell reduces to: constructor, key dispatch, and the IM-loop calls.

**Task list:**
- [ ] Fcitx settings reader swapped to `shanti-config` (drops the duplicated `booleanValue`/`populateConfig` block); one settings parser for both engines.
- [ ] Data discovery swapped to `shanti-paths`.
- [ ] Remaining engine logic behind the C ABI; C++ shell minimized.
- [ ] Typing matrix on Fcitx5; package-size delta recorded.

**AC:** the typing matrix passes; settings parsing is single-sourced; package size drops alongside the removed dependency.

### Phase 4 — GUI: Qt → GTK4/libadwaita

The largest single phase (~4.9k lines). Ground rules:

- Use gtk4-rs + libadwaita-rs. Pin the Flathub runtime (`org.gnome.Platform`, newest stable at kickoff) and the matching libadwaita version; do not switch versions mid-phase without a decision-log entry (beta/previews are out of scope).
- The GUI is a **control plane**, not an IME: it touches (a) the settings store, (b) desktop-integration registration files, (c) presentation. Port file actions faithfully; do not redesign the integration contract mid-flight.
- The settings **file format** does not change; only the writer moves from QSettings to `shanti-config`.

Port order (each row lands as its own milestone behind CI checks):

| Qt piece | GTK target | Notes & risks |
|---|---|---|
| `main.cpp`, `SingleInstance` | `adw::Application` (GApplication provides unique-instance handling) | Straightforward. |
| `TopBar` + `KeyboardWidget` (floating always-on-top bar, drag positioning, persistence) | GTK window with `gdk::monitor`-aware placement; **Wayland vs X11 behavior differs** — honor the stored position per-platform and verify multi-monitor + fractional scaling explicitly | Highest-risk widget: existing users' stored `topBarWindowPosition` must still apply. Wayland spike before committing. |
| `SettingsDialog` + fork's GNOME IBus Integration panel (backup-or-remove conflict flow) | `adw::PreferencesWindow`-style pages; keep the panel between the page header and the general card (upstream's 2026 layout anchor) | Test setup/refusal/recovery per checklist §3 after every change. |
| `FlatpakIbusIntegration` (component descriptor, launcher, systemd drop-ins, dconf input sources) | Same file actions from the GTK app via `shanti-paths` | Contract files must stay byte-identical — the uninstall allowlist depends on the exact paths. |
| `PlatformConfig` (backend detection, env setup, fcitx startup) | Logic into `shanti-paths`/`shanti-config`; GTK code stays thin | No duplicated logic. |
| `AboutDialog`, `AboutFile`, `AutoCorrectDialog` | gtk4 `AboutDialog`, `adw` widgetry, list/factory pattern | `.ui` files are not ported 1:1; rebuild in code or Adwaita templates. AutoCorrectDialog's preview uses the legacy frontend-internal `AvroPhonetic` parser (phase-1 audit) — drop it and preview through the Rust engine API instead. |
| `LayoutViewer` + `LayoutConverter` (layout image rendering, QSvgRenderer) | `GtkPicture`/draw pipeline with librsvg (gir) or `resvg` | Verify rendered layout images on both backends. |
| `KeyboardWidget` themes (`light.qss`/`dark.qss`) | libadwaita stylesheet + `StyleManager` dark-mode detection | Drop Qt QSS entirely. |
| `Layout` class (v3 layout JSON loading, rule conversion) | Rust port; coordinate the parser with riti's khipro module (decision log) | Golden tests against shipped `data/*.json`. |

Flatpak strategy: keep the Qt/KDE-runtime Flatpak **green through phase 4**; introduce a second manifest (GNOME runtime, GTK app) and switch the hosted bundle only after the GTK variant passes the checklist's real-desktop integration tests (`org.openbangla.Shanti.IBus` app ID, wrapper permission stories, scope rules unchanged). Retire the KDE-runtime manifest afterwards.

**AC:** package-test grows GTK equivalents of the offscreen GUI checks (headless `GDK_BACKEND=x11` run; `--version` and `--check-data` flags preserved on the GTK binary); settings written by the GTK app are read correctly by the engines and vice-versa; the checklist's desktop validation matrix passes after each milestone and formally at the phase-4 release.

### Phase 5 — Retirement of C++ and Qt

1. Delete the remaining C++ (whichever shell survives: the IBus fallback wrapper or the fcitx5 shell); remove Qt/CMake from the build and manifests; drop `corrosion`.
2. Rename binaries/allowlist entries **in one commit together with the uninstall-allowlist migration** — install paths are user-visible contracts (see §6.4).
3. Update `docs/wiki/` for the new build instructions; `tools/build.sh`'s `--backend` interface stays unchanged.
4. Final real-desktop typing matrix + release.

**AC:** release archives contain no Qt; the uninstall allowlist test still passes (it must *not* change — it is the contract); CHANGELOG milestone entry exists.

## 6. Compatibility gates (checked per phase, in package-test where possible)

1. Settings store path + INI format unchanged: `$XDG_CONFIG_HOME/OpenBangla/Keyboard.conf` (QSettings `("OpenBangla", "Keyboard")` org/app store) — round-trip test with a fixture file written by the Qt app.
2. User data/interfaces unchanged: custom-layout discovery, autocorrect data, dictionary behavior.
3. Registration-file paths and desktop-integration contracts unchanged (ibus component, fcitx5 confs, environment.d file, systemd drop-in) — the uninstall allowlist in `package-test.yml` must pass untouched.
4. New binary names stay out of user-visible paths during the transition; final renames happen only at phase 5, alongside the allowlist migration commit.
5. CLI affordances preserved through the GUI rewrite: `openbangla-gui --version`, `--check-data`.
6. No regression in the portable install model or archive layout.

## 7. Upstream divergence policy (explicit, decision-backed)

Through phase 1, upstream-merge semantics stay mostly intact except `src/shared/` (small conflict load; resolved by taking the Rust side). Phases 2–5 progressively make the fork unable to merge `upstream/develop` beyond:

- `data/*.json` layout/data updates (always mergeable),
- upstream riti algorithm fixes (mergeable until Decision 0 vendoring; afterwards cherry-picked),

…plus nothing else — engines and the entire frontend diverge permanently. **This is accepted as the price of the rewrite.** Record the milestone where upstream sync is formally removed from the routine merge process. From that point, monitor upstream only for security fixes or critical behavioral regressions worth re-implementing manually.

## 8. Risks (tracked, updated per phase)

| Risk | Mitigation |
|---|---|
| Upstream riti evolves its C API mid-transition | Vendoring (Decision 0); pin the cbindgen API version; CHANGELOG notes any migration. |
| libibus GObject-introspection coverage insufficient | Phase 2 fallback: shrink the C++ wrapper instead of a full rewrite — a Rust-logic engine still satisfies the §1 goal. |
| GTK floating bar (TopBar) Wayland window-positioning semantics differ from Qt | Dedicated Wayland + X11 spike in phase 4 before committing; Qt stays during the spike. |
| Settings INI edge cases (escaping of `/`, backslashes, arrays in QSettings) | Fixture-based round-trip tests against real-world plist files (upstream-era + fork-era). |
| GNOME IBus integration flow regressions | Checklist §3 validation matrix after each panel change; no panel merge without a real-session test. |
| aarch64-only breakage in Rust dependencies | CI already builds both architectures; extend package-test to cover both Flatpaks during the transition. |
| Maintainer bandwidth stalls mid-phase | Phases land shippable by design; phase 1 alone (Qt-free engines) is worth having even if later phases never happen. |

## 9. Decision log

| Date | Decision | Rationale |
|---|---|---|
| 2026-10-22 | Plan created; phase 1 defined as the first milestone | Settings/paths are small and testable and de-Qt the engines independently of the GUI. |
| 2026-10-22 | Phase 1 started on branch `rust-rewrite`; narrowed to the IBus engine | Inspection showed the Fcitx5 module is already Qt-free (own INI reader). Decision 0 not required for phase 1: the new crates share no types with riti. |
| *(pending)* | Decision 0: vendor riti vs upstream submodule | Needed before phase 1 crates can share types with the core. |
| *(pending)* | IBus binding strategy (gir bindings vs shrunken C++ wrapper) | Scout libibus GIR coverage first. |
| *(pending)* | GTK runtime + libadwaita pin (decided at phase 4 kickoff) | Newest stable GNOME runtime at start; no mid-phase switches. |

*Update this log — with a commit — every time a pending row is resolved.*
