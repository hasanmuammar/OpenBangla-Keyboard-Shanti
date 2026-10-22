# Install Shanti Keyboard

Choose the method that matches your desktop:

## GNOME: IBus Flatpak

## KDE Plasma / Kinoite: Fcitx5 addon Flatpak

Download the Fcitx5 addon bundle for your architecture from [GitHub Releases](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/releases) and install it — Flatpak automatically pulls Flathub's sandboxed Fcitx5 (`org.fcitx.Fcitx5`) if it is not already installed:

```bash
flatpak install --user ./shanti-fcitx-addon-x86_64.flatpak
```

Then pick **Shanti** in the Fcitx5 input-method list (see the [Flatpak guide](../../tools/flatpak/README.md) for the Fcitx5 autostart step on Kinoite).

## GNOME: IBus Flatpak

Download the IBus Flatpak bundle for your architecture from [GitHub Releases](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/releases). Flatpak needs a Flathub remote in the same scope as the app. For a per-user install, add the user remote if needed:

```bash
flatpak remote-add --user --if-not-exists flathub \
  https://flathub.org/repo/flathub.flatpakrepo
```

Install the bundle:

```bash
flatpak install --user ./shanti-ibus-x86_64.flatpak
```

Flatpak should fetch the required KDE runtime (`org.kde.Platform//5.15-25.08`) from Flathub automatically. Use the `aarch64` bundle on ARM64. Keep the scope consistent: use `--user` for both commands, or omit it for both to install system-wide. If Flatpak says the runtime was not found, install it explicitly with `flatpak install --user flathub org.kde.Platform//5.15-25.08` (omit `--user` for system scope). Launch the app and accept the first-run integration prompt. Then add **Shanti** from **GNOME Settings → Keyboard → Input Sources → Add Input Source**.

See the [Flatpak guide](../../tools/flatpak/README.md) for screenshots-free step-by-step setup, verification and removal instructions.

## Portable install

The portable installer is for IBus and Fcitx5 desktops. From a source checkout, run:

```bash
bash tools/install.sh
```

It downloads the release selected in `release-tag.txt`, checks its SHA-256 checksum and installs for the current user. KDE Plasma selects Fcitx5; GNOME and other desktops select IBus. To choose a release explicitly:

```bash
bash tools/install.sh --version shanti-alpha.1
```

Release assets and available versions are listed on [GitHub Releases](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/releases). Optional provenance verification requires `gh`:

```bash
bash tools/install.sh --version shanti-alpha.1 --verify-provenance
```

Every release archive also contains the installer and uninstaller, so a downloaded archive needs no source checkout:

```text
share/shanti-keyboard/tools/install.sh
share/shanti-keyboard/tools/uninstall.sh
share/shanti-keyboard/release-tag.txt
```

After extracting the archive into your home directory they are available under `~/.local/share/shanti-keyboard/`, for example:

```bash
bash ~/.local/share/shanti-keyboard/tools/uninstall.sh
```

## Build from source

For development or local builds:

```bash
bash tools/build.sh
```

See the [development guide](Development.md) for build options. To remove the portable installation, run `bash tools/uninstall.sh`. Flatpak removal is covered in the [Flatpak guide](../../tools/flatpak/README.md).
