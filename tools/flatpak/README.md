# Flatpak packages

Shanti Keyboard ships as two Flatpaks. Pick the one that matches your desktop —
both give you the same Shanti Keyboard engine, the same layouts, and the **same
settings GUI**, built on the same Qt5 runtime (`org.kde.Platform//5.15-25.08`).
The two bundles differ only in the engine backend they carry. Download the
bundle for your architecture from the
[GitHub Releases](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/releases)
page (`.x86_64` for most Intel/AMD computers, `.aarch64` for ARM64).

| If your desktop uses... | Install | Flatpak ID |
| --- | --- | --- |
| GNOME (Ubuntu, Fedora Workstation, Fedora Silverblue, GNOME OS, …) | **ShantiIbus** | `org.shanti.ShantiIbus` |
| KDE Plasma / Kinoite, or anywhere you already use Fcitx5 | **ShantiFcitx5** | `org.shanti.ShantiFcitx5` |
| Not sure? | On GNOME → **ShantiIbus**; elsewhere run `echo $XDG_CURRENT_DESKTOP` — `KDE`/`KDE-Live` → **ShantiFcitx5**, anything else → **ShantiIbus** consistently |

If you are on KDE (including Kinoite), the **ShantiFcitx5** app is completely
self-sufficient — the Fcitx5 keyboard service it needs comes inside the app,
so nothing extra is required on your computer.

---

## ShantiIbus — the IBus engine (for GNOME)

Packages the Shanti settings GUI and the IBus engine as
`org.shanti.ShantiIbus`. It does not include Fcitx5.

### Install and add the GNOME input source

1. Make sure Flathub is configured for the same scope you will use for the app.
   For a per-user install:

   ```sh
   flatpak remote-add --user --if-not-exists flathub \
     https://flathub.org/repo/flathub.flatpakrepo
   ```

   The app needs the separate KDE runtime `org.kde.Platform//5.15-25.08`.
   Flatpak normally downloads it from Flathub during app installation. The
   bundle does not contain the runtime.

2. Install the downloaded bundle. Replace the filename below with your
   downloaded file:

   ```sh
   flatpak install --user ./shanti-ibus-x86_64.flatpak
   ```

   If Flatpak says the runtime was not found, the remote and app are probably
   in different scopes. Either install the app with `--user` as above, or add
   Flathub and install the app system-wide. You can also install the runtime
   explicitly in the matching scope:

   ```sh
   flatpak install --user flathub org.kde.Platform//5.15-25.08
   ```

3. Launch **Shanti Keyboard (IBus)** from the app grid.
4. On first launch, accept **Set up integration**. Or open the app's
   **Settings** → **GNOME IBus Integration** and select **Set up / repair**.
   Confirm the action.
5. Open **GNOME Settings → Keyboard → Input Sources → Add Input Source**, then
   select **Shanti**.
6. Select Shanti from the input menu and try typing Bangla in another app.

The app's Settings panel reports whether its registration is present and
GNOME's IBus service is active. If an unmanaged IBus override blocks setup, the
app shows its path and offers **Back up and continue**. Choose this only if you
recognize the file as an old Shanti setup you want to replace; the app
preserves it as a backup before installing the current integration. Choose
**Cancel** if you do not recognize it. The integration manages user-level
files; it does not modify system files.

### Remove

1. In the app, open **Settings → GNOME IBus Integration → Remove integration**
   and confirm. This removes Shanti's GNOME registration. If the app is
   unavailable, see [uninstallation](../../docs/wiki/Uninstallation.md) for the
   managed file paths and manual cleanup guidance.
2. If **Shanti** remains in GNOME's input-source list, remove it in
   **GNOME Settings → Keyboard → Input Sources**.
3. Uninstall the app:

   ```sh
   flatpak uninstall --user org.shanti.ShantiIbus
   ```

Upgrading from a pre-rebrand release (`org.openbangla.Shanti.IBus`)? Install
the new bundle normally; on first run it copies your settings and user data
from the old `openbangla-keyboard` locations to the new `shanti-keyboard`
locations. After the upgrade you can remove the old app:
`flatpak uninstall --user org.openbangla.Shanti.IBus`.

## ShantiFcitx5 — the Fcitx5 app

This package is for people who use KDE (including Kinoite), or any desktop
where you already type with Fcitx5. Everything it needs — the Fcitx5
keyboard service and the Shanti engine — is inside the app itself, so
**you don't need to install anything extra** on your system.

Two things to know:

- Use this app **or** Flathub's plain Fcitx5 app (`org.fcitx.Fcitx5`) — not
  both at the same time. If Flathub's Fcitx5 is installed, remove it first.
- It comes with the **same Shanti settings app as the IBus package**, so the
  experience is identical either way.

### Install and start typing

1. Add Flathub (the app downloads its base bits from there):

   ```sh
   flatpak remote-add --user --if-not-exists flathub \
     https://flathub.org/repo/flathub.flatpakrepo
   ```

2. Install the downloaded bundle:

   ```sh
   flatpak install --user ./shanti-fcitx5-x86_64.flatpak
   ```

3. Open **Shanti Keyboard (Fcitx5)** from your app grid. On first launch it
   asks:

   > Shanti is not set up with Fcitx5 yet. **Set up integration?**

   Click **Set up integration** — that's all it takes. Shanti gets switched
   on, started now, and started automatically every time you log in.

4. Switch to Shanti with **Ctrl+Space** (the usual input-method switch key)
   and start typing Bangla.

   Every other setting — keyboard layout, phonetic mode, autocorrect, and
   so on — is in the same **Shanti Keyboard (Fcitx5)** window, just like in
   the IBus package.

If anything ever stops working (for example after an update), open the app
again — it notices and offers to repair the setup. The same lives in
**Settings → Fcitx5 Integration**, and in the tray menu as **Fcitx5
Integration…**.

Advanced users can still tweak Fcitx5 itself with the bundled config tool:

```sh
flatpak run --user org.shanti.ShantiFcitx5 --command=fcitx5-configtool
```

### Remove

1. Open the app, go to **Settings → Fcitx5 Integration**, and click
   **Remove integration**. (If you can't open the app anymore, skip this —
   it only removes Shanti's auto-start entry.)

2. Uninstall the app:

   ```sh
   flatpak uninstall --user org.shanti.ShantiFcitx5
   ```

---

## Verify a download (optional)

The release includes a `.sha256` file alongside each bundle. From the directory
containing both files, run:

```sh
sha256sum --check shanti-ibus-x86_64.flatpak.sha256
```

Use the aarch64 checksum filename for an ARM64 download. Releases also include
GitHub build-provenance attestations; see GitHub's
[artifact attestation verification](https://cli.github.com/manual/gh_attestation_verify)
documentation if you want to verify one with `gh`.

# Build locally

### Test a CI build from a branch or PR

Every push (including PR branches) already builds both Flatpaks in CI and
uploads them as downloadable artifacts, named `shanti-ibus-flatpak-x86_64`,
`shanti-ibus-flatpak-aarch64`, `shanti-fcitx5-flatpak-x86_64` and
`shanti-fcitx5-flatpak-aarch64`. Grab them with `gh` using the run ID:

```sh
gh -R hasanmuammar/OpenBangla-Keyboard-Shanti run list --branch rust-rewrite
gh -R hasanmuammar/OpenBangla-Keyboard-Shanti run download <run-id> \
  -n shanti-ibus-flatpak-x86_64 -D /tmp/shanti-flatpak-test
flatpak install --user --reinstall /tmp/shanti-flatpak-test/shanti-ibus-x86_64.flatpak
```

Then follow [Install and add the GNOME input source](#install-and-add-the-gnome-input-source).
Log out and back in so IBus picks up the updated engine.

### Build from a checkout

To build and install the IBus bundle from a checkout, use Flatpak Builder on a
Linux host with Flatpak and Flatpak Builder installed:

```sh
flatpak-builder --user --install --force-clean \
  --default-branch=stable \
  /tmp/shanti-ibus-build tools/flatpak/shanti-ibus.yml
```

This downloads the build dependencies and can take several minutes. Then launch
it with:

```sh
flatpak run --user org.shanti.ShantiIbus
```

The Fcitx5 app builds the same way:

```sh
flatpak-builder --user --install --force-clean \
  --default-branch=stable \
  /tmp/shanti-fcitx-build tools/flatpak/fcitx-app.yml
```

> Tip: `flatpak-builder` requires its state directory (default
> `.flatpak-builder` inside the checkout) to be on the **same filesystem** as
> the target directory. If your clone lives on a different filesystem than
> `/tmp`, pass a matching `--state-dir`:
>
> ```sh
> mkdir -p /tmp/opencode
> flatpak-builder --user --install --force-clean \
>   --state-dir=/tmp/opencode/flatpak-build-state \
>   --default-branch=stable \
>   /tmp/shanti-ibus-build tools/flatpak/shanti-ibus.yml
> ```

### Troubleshooting: build fails at `appstreamcli compose`

`flatpak-builder` finishes by running `appstreamcli compose`. If the
`appstreamcli` found on `PATH` was installed without its compose helper — for
example a Homebrew `appstream` on Bluefin, Bazzite and other atomic desktops —
the build fails near the end with:

```
AppStream Compose binary '…/libexec/appstreamcli-compose' was not found!
Error: appstreamcli compose failed
```

The helper path is compiled into `appstreamcli`. On Bluefin-style systems the
system helper already exists at `/usr/libexec/appstreamcli-compose`; expose it
where the Homebrew AppStream expects it and rebuild:

```sh
brew_cellar="$(brew --cellar appstream)/$(brew list --versions appstream | cut -d' ' -f2)"
mkdir -p "$brew_cellar/libexec"
ln -s /usr/libexec/appstreamcli-compose "$brew_cellar/libexec/appstreamcli-compose"
```

Alternatively, build with the sandboxed Flatpak Builder app, which does not
use the host's `appstreamcli`:

```sh
flatpak run org.flatpak.Builder --user --install --force-clean \
  --default-branch=stable \
  /tmp/shanti-ibus-build tools/flatpak/shanti-ibus.yml
```

The release bundles are direct downloads, not a Flatpak remote. If installation
says the required runtime was not found, check that Flathub is added to the
same installation scope. Then run the explicit runtime install above, omitting
`--user` for a system-wide install. The Flatpak and the portable installer
(from the source checkout's `tools/build.sh`) are separate installations.
