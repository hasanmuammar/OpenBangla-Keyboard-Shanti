# Change Log
## Unreleased

* The IBus engine's state machine (settings application, key-event handling, suggestion/commit logic and the riti session) moved into the new `shanti-core` Rust crate behind a small C ABI; `main.cpp` is now pure libibus plumbing that renders the crate's decisions. Engine behaviour is now covered by unit tests that drive the real riti phonetic engine. The Fcitx5 addon and the Qt GUI are unchanged.
* The IBus engine is now Qt-free (rewrite phase 1): settings and path discovery moved to the new `shanti-config` and `shanti-paths` Rust crates, which read the same `~/.config/OpenBangla/Keyboard.conf` store written by the Qt GUI and mirror the `FileSystem.cpp` path contracts exactly. The GUI and the Fcitx5 addon are unchanged.
* The duplicated 112-entry keysym → riti virtual-key tables in the IBus and Fcitx5 engine `keycode.h` headers are now single-sourced in the new `shanti-keysyms` Rust crate; both engines call it over a C ABI, and the table is verified identical on both platforms and against the real IBus/Fcitx keysym enums.
* `package-test.yml` now asserts the shipped IBus engine binary links no Qt libraries, locking the phase-1 contract into CI.
* Deduplicated the uninstaller: `tools/uninstall.sh` now sources the bundled `tools/portable/common.sh` helper library instead of carrying its own copies of `die`, `log`, the mount-point safety check and the XDG/HOME canonicalization logic. Unsafe XDG values now fail closed instead of silently falling back to defaults.

## shanti-alpha.1.4

* Pinned the system toolchain `PATH` for the Flatpak build steps in CI and release workflows, so a Homebrew `appstream` lacking the `appstream-compose` helper can never break the final `appstreamcli compose` step.
* Documented the `appstreamcli compose` local build failure seen on Bluefin, Bazzite and other atomic desktops, with two fixes: exposing the system compose helper to the Homebrew AppStream, or building with the sandboxed `org.flatpak.Builder` app.
* The Flatpak metadata now advertises the release version, so `flatpak list` and software centers show it instead of a blank version.

## shanti-alpha.1.3

* Include `install.sh`, `uninstall.sh`, their `portable/` helpers and `release-tag.txt` inside portable release archives, so a downloaded tarball can install or uninstall without a source checkout.
* Refresh those bundled scripts on upgrade while continuing to preserve user layouts, autocorrect data and other user-owned files.

## shanti-alpha.1.2

* Merged upstream dialog redesign (upstream PR #475): Settings, About, AutoCorrect and Layout Viewer dialogs restyled with light/dark themes, new ToggleSwitch and KeyboardWidget widgets, and layout JSON v3 `transliteration` layout support.
* The GNOME IBus integration panel survives the redesign and now renders on the General page while keeping its backup-or-remove conflict flow.

## shanti-alpha.1.1

* Offer a confirmed backup-and-update flow when an existing IBus service override blocks GNOME integration setup.
* Clarify Flatpak runtime installation and matching Flathub installation scope.

## shanti-alpha.1

Shanti fork changes (not part of the inherited upstream 3.0.0 release):

* Add an experimental IBus-only Flatpak with separate app ID `org.openbangla.Shanti.IBus` and x86_64/aarch64 CI bundles.
* Add an in-app GNOME IBus registration and management flow for the Flatpak, including Settings panel controls and managed user-level component, launcher and systemd drop-in files.
* Add CI coverage for Flatpak bundle builds alongside portable IBus/Fcitx5 builds and package validation.
* Tagged alpha prereleases are configured to publish portable archives and direct-download IBus Flatpak bundles; no OSTree Flatpak remote is configured.

## 3.0.0
* Fcitx5 engine for KDE based distributions ([#202])
* Use [okkhor] as the Avro Phonetic implementation. It is substantially faster than the previous implementation ([#15])
* Add the [Khipro](https://khiprokeyboard.github.io/) layout, a modern shift-free phonetic method for Bangla ([#426](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/426))
* Emoji input support ([#23])
* ANSI encoding support ([#33])
* Smart Quoting feature ([#35])
* Whole word deletion by <kbd>Ctrl</kbd> + <kbd>Backspace</kbd> keypress combination feature ([#36])
* Preserve user's candidate selection if the keypress was a punctuation mark in Phonetic Method ([#363])
* Old style typing in Fixed method ([#20])
* Inclusion of typed English word in Fixed method suggestions.
* Introduce System tray entry for TopBar ([#209])
* Installation support for new Fixed method layouts ([#355])
* Edit National keyboard to conform with Ekushey's National layout ([#281])
* Missing ZWJ character is added in the Borno layout
* Fix focus out behavior of preedit text in iBus engine ([#298], [#300])
* Prevent the preedit from reappearing at the new position after a mouse click in iBus engine ([#313])
* Detect Dark Mode to enable the Dark mode theme of the TopBar
* Performance optimizations ([#16], [#26])
* Add new autocorrect entries ([#48])

[okkhor]: https://github.com/gulshan/okkhor/
[#15]: https://github.com/OpenBangla/riti/pull/15
[#16]: https://github.com/OpenBangla/riti/pull/16
[#20]: https://github.com/OpenBangla/riti/pull/20
[#23]: https://github.com/OpenBangla/riti/pull/23
[#26]: https://github.com/OpenBangla/riti/pull/26
[#33]: https://github.com/OpenBangla/riti/pull/33
[#35]: https://github.com/OpenBangla/riti/pull/35
[#36]: https://github.com/OpenBangla/riti/pull/36
[#48]: https://github.com/OpenBangla/riti/pull/48
[#209]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/209
[#202]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/202
[#281]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/281
[#298]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/298
[#300]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/300
[#313]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/313
[#355]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/355
[#363]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/363

## 2.0.0
* Complete re-implementation of input method functionalities in Rust. ([#107][107]) It is [~1.2x faster in searching][3] dictionary suggestions.
* Dictionary suggestion is now available in Fixed Method Layouts(Provat, National etc.) ([#61][61], [#6][6])
* Phonetic suggestion now includes the typed English word as the last item in the suggestion list. ([#121][121], [#124][124])
* UI Refresh & Polish
    * Uses Material Design Icons. ([#165][165])
    * Merges the About button and it's menu into the Icon button which is also used to move the TopBar. This shrinks the TopBar a little. ([#165][165])
    * Redesign the Layout Viewer Dialog. ([#115][115], [#145][145])
    * Redesign the About Dialog. ([#116][116], [#155][155])
    * Redesign the Settings Dialog. This also merges all the available settings from the settings menu. ([#159][159])
    * Polish the Auto Correct entries Dialog. ([#164][164])
* Introduces a new Fixed Keyboard layout format. ([#143][143])
    * Now all of the keyboard layout we distribute are 58% smaller altogether.
    * Properly supports [other system keyboard layouts][36] instead of just the Qwerty layout.
    * Supports AltGr & ShiftAltGr layer of numerical keys.
    * Automatic conversion of old layout formats.
* Now AltGr layer can be typed with the Right Alt key in Fixed Keyboard methods. ([#152][152])
* **Bug Fix**: Handle the BackSlash key in Fixed Keyboard methods. ([#98][98])
* **Bug Fix**: Crash when using the Arrow keys in phonetic method. ([#123][123])
* **Bug Fix**: Fix writing of `Hasant` + `য` and য-ফলা(`্য`) in Fixed Keyboard methods. ([#117][117], [#163][163])
* **Bug Fix**: Fix Window Manager losing the track of the TopBar issue. ([#153][153], [#171][171])
* Follows XDG Directory Specification for storing user specific data. ([#16][16]) Automatically [migrates user specific files][143] into the new place.
* Changes binary names to be more UNIX like. ([#103][103])
* Huge infrastructural change: We now use Github Actions and Bintray for automating our testing and package deployment. ([#130][130])

[107]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/107
[3]: https://github.com/OpenBangla/riti/pull/3
[61]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/61
[6]: https://github.com/OpenBangla/riti/pull/6
[121]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/121
[124]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/124
[16]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/16
[165]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/165
[116]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/116
[155]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/155
[159]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/159
[164]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/164
[115]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/115
[145]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/145
[143]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/143
[36]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/36
[98]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/98
[123]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/123
[152]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/152
[117]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/117
[163]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/163#issuecomment-689956532
[103]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/103
[153]: https://github.com/OpenBangla/OpenBangla-Keyboard/issues/153
[171]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/171
[130]: https://github.com/OpenBangla/OpenBangla-Keyboard/pull/130
## 1.5.1
* Fixes a dictionary loading issue which was affecting in some scenarios. ([#73](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/73))
* Polished some UI dialogs. ([#66](https://github.com/OpenBangla/OpenBangla-Keyboard/pull/66))
## 1.5.0
* AutoCorrect item is shown as the first suggestion item if available when writing using Avro Phonetic method. ([#42](https://github.com/OpenBangla/OpenBangla-Keyboard/pull/42))
* Performance update for Avro Phonetic suggestion making code. ([#42](https://github.com/OpenBangla/OpenBangla-Keyboard/pull/42))
* Be less restrictive when saving users selection. ([#41](https://github.com/OpenBangla/OpenBangla-Keyboard/pull/41)) 
* Now it is possible to write using AltGr and ShiftAltGr layer when using Fixed layout methods. Fixes bug [#40](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/40).
* Auto-update checking is now configurable from settings dialog. There is now also a menu item in the TopBar for instant update checking. ([#32](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/32))
* Fixed a crash. ([#52](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/52))
## 1.4.0
* Fixes a serious bug [#26](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/26).
* OpenBangla Keyboard will check for updates automatically and notify the user if an update is available.
## 1.3.0
* OpenBangla Keyboard now features a Phonetic AutoCorrect Preference Dialog [#20](https://github.com/OpenBangla/OpenBangla-Keyboard/pull/20). User now can view/edit/add AutoCorrect entries.
* Better Topbar integration with X11 windowing system and reduce the size of TopBar [#22](https://github.com/OpenBangla/OpenBangla-Keyboard/pull/22).
* Let users turn off candidate box [#11](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/11)
* Build system improvement. There is no need to pass `CMAKE_INSTALL_PREFIX` when building.
* Now supports RPM packaging.
* Fixed issue [#17](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/17). OpenBangla Keyboard was failing to store Layout setting when using Qt 5.10 version.
* Improved Settings dialog UI.
* Rewritten Avro Layout conversion code. OpenBangla Keyboard now handles errors more gracefully and reports them to the user.

## 1.2.0
* Fixed a memory leak.
* Fixed a long standing bug [#8](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/8).
## 1.1.0
* Improve Phonetic support. Resolves issue [#2](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/2). Now Phonetic support is mostly complete!
* A new button in TopBar for easily moving it.
* User can select candidate by clicking.
* These new layouts were added: **Munir Optima (uni)**, **Avro Easy**, **National (Jatiya)** and **Probhat**. Thanks to [OmicronLab](https://www.omicronlab.com/) because these layouts were also imported from [Avro Keyboard](https://www.omicronlab.com/avro-keyboard.html)!

## 1.0.0
First release! All things are new!

**Changes after 0.1.0 beta:**
* New feature: Tab to switch between the matches in candidate window
* While typing numbers in phonetic method, pressing : should make it : not ঃ
* Old Reph feature in fixed typing method
* User can select the orientation of candidate window in Settings
* Icons for OpenBangla Keyboard by [@saaiful](https://github.com/saaiful)

**Known issues:**
* [Implement suffix phonetic suggestion](https://github.com/OpenBangla/OpenBangla-Keyboard/issues/2)

## 0.1.0
Beta release
