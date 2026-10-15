# Install the IBus Flatpak on GNOME

The Shanti Flatpak packages the OpenBangla GUI and IBus engine as `org.openbangla.Shanti.IBus`. It is intended for GNOME and does not include the Fcitx5 engine. Download the bundle for your system from the [GitHub Releases](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/releases) page:

- `shanti-ibus-x86_64.flatpak` for most Intel and AMD computers.
- `shanti-ibus-aarch64.flatpak` for ARM64 computers.

## Install and add the GNOME input source

1. Make sure Flathub is configured for the same scope you will use for the app. For a per-user install:

   ```sh
   flatpak remote-add --user --if-not-exists flathub \
     https://flathub.org/repo/flathub.flatpakrepo
   ```

   The app needs the separate KDE runtime `org.kde.Platform//5.15-25.08`. Flatpak normally downloads it from Flathub during app installation. The Shanti bundle does not contain the runtime.

2. Install the downloaded bundle. Replace the filename below with your downloaded file:

   ```sh
   flatpak install --user ./shanti-ibus-x86_64.flatpak
   ```

   If Flatpak says the runtime was not found, the remote and app are probably in different scopes. Either install the app with `--user` as above, or add Flathub and install the app system-wide. You can also install the runtime explicitly in the matching scope:

   ```sh
   flatpak install --user flathub org.kde.Platform//5.15-25.08
   ```

3. Launch **OpenBangla Keyboard Shanti (IBus)** from the app grid.
4. On first launch, accept **Set up integration**. Or open the app's **Settings** → **GNOME IBus Integration** and select **Set up / repair**. Confirm the action.
5. Open **GNOME Settings → Keyboard → Input Sources → Add Input Source**, then select **OpenBangla**.
6. Select OpenBangla from the input menu and try typing Bangla in another app.

The app's Settings panel reports whether its registration is present and GNOME's IBus service is active. If an unmanaged IBus override blocks setup, the app shows its path and offers **Back up and continue**. Choose this only if you recognize the file as an old Shanti setup you want to replace; the app preserves it as a backup before installing the current integration. Choose **Cancel** if you do not recognize it. The integration manages user-level files; it does not modify system files.

## Remove

1. In the app, open **Settings → GNOME IBus Integration → Remove integration** and confirm. This removes Shanti's GNOME registration. If the app is unavailable, see [uninstallation](../../docs/wiki/Uninstallation.md) for the managed file paths and manual cleanup guidance.
2. If **OpenBangla** remains in GNOME's input-source list, remove it in **GNOME Settings → Keyboard → Input Sources**.
3. Uninstall the app:

   ```sh
   flatpak uninstall --user org.openbangla.Shanti.IBus
   ```

## Verify a download (optional)

The release includes a `.sha256` file alongside each bundle. From the directory containing both files, run:

```sh
sha256sum --check shanti-ibus-x86_64.flatpak.sha256
```

Use the aarch64 checksum filename for an ARM64 download. Releases also include GitHub build-provenance attestations; see GitHub's [artifact attestation verification](https://cli.github.com/manual/gh_attestation_verify) documentation if you want to verify one with `gh`.

## Build locally

To build and install from a checkout, use Flatpak Builder on a Linux host with Flatpak and Flatpak Builder installed:

```sh
flatpak-builder --user --install --force-clean \
  --default-branch=stable \
  /tmp/shanti-ibus-build tools/flatpak/shanti-ibus.yml
```

This downloads the build dependencies and can take several minutes. Then launch it with:

```sh
flatpak run org.openbangla.Shanti.IBus
```

The release bundles are direct downloads, not a Flatpak remote. If installation says the required runtime was not found, check that Flathub is added to the same installation scope. Then run the explicit runtime install above, omitting `--user` for a system-wide install. The Flatpak and portable installer are separate installations.
