# Flatpak packages

Shanti Keyboard ships as two Flatpaks. Pick the one that matches your desktop —
both give you the same Shanti Keyboard engine, the same layouts, and the same
settings GUI. Download the bundle for your architecture from the
[GitHub Releases](https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti/releases)
page (`.x86_64` for most Intel/AMD computers, `.aarch64` for ARM64).

| If your desktop uses... | Install | Flatpak ID |
| --- | --- | --- |
| GNOME (Ubuntu, Fedora Workstation, Fedora Silverblue, GNOME OS, …) | **ShantiIbus** | `org.shanti.ShantiIbus` |
| KDE Plasma / Kinoite, or anywhere you already use Fcitx5 | **ShantiFcitx** | `org.fcitx.Fcitx5.Addon.Shanti` |
| Not sure? | On GNOME → **ShantiIbus**; elsewhere run `echo $XDG_CURRENT_DESKTOP` — `KDE`/`KDE-Live` → **ShantiFcitx**, anything else → **ShantiIbus** consistently |

Flatpak technical fact behind the naming: Fcitx5 addons must be *extensions of
Flathub's `org.fcitx.Fcitx5` app* and Flatpak requires extension app IDs to
start with `org.fcitx.Fcitx5.Addon.` — that is why the Fcitx package is called
`org.fcitx.Fcitx5.Addon.Shanti` rather than a `org.shanti.*` ID.

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

## ShantiFcitx — the Fcitx5 engine addon

`org.fcitx.Fcitx5.Addon.Shanti` is an *addon* for the sandboxed Fcitx5 Flatpak
(`org.fcitx.Fcitx5`, published on Flathub). Installing the sandboxed Fcitx5
from Flathub puts the whole Fcitx5 stack inside the Flatpak sandbox — you do
not need Fcitx5 on the host system at all, including on Kinoite, Silverblue,
or any immutable desktop.

### Install and add the Fcitx input method

1. Add Flathub (per-user, matching the addon's install scope):

   ```sh
   flatpak remote-add --user --if-not-exists flathub \
     https://flathub.org/repo/flathub.flatpakrepo
   ```

2. Install the sandboxed Fcitx5 and then the addon (both per-user; the first
   command is a no-op when Fcitx5 is already installed):

   ```sh
   flatpak install --user flathub org.fcitx.Fcitx5
   flatpak install --user ./shanti-fcitx-addon-x86_64.flatpak
   ```

   During installation, keep the source-remote choice at its default (accept)

3. Make sure the sandboxed Fcitx5 runs at login. On Kinoite/Plasma, enable
   Flatpak Fcitx5 with:

   ```sh
   systemctl --user enable --now org.fcitx.Fcitx5
   ```

   (If your host already runs a native Fcitx5, prefer the native IME instead —
   see [installing from source](../README.md), a portable release archive, or the
   distro package.)

4. Open the Fcitx config area: `flatpak run --user org.fcitx.Fcitx5` exposes
   the addon's **Shanti Keyboard** entry; pick **Shanti** in the Fcitx5
   method list (the method name shown to Fcitx5 is `shanti`). Note: this
   addon build does not include the Qt settings GUI, so its Configure
   launcher entry does nothing — edit `~/.var/app/org.fcitx.Fcitx5/config`
   the same way the portable install does, or use the portable `shanti-gui`.

### Remove

```sh
flatpak uninstall --user org.fcitx.Fcitx5.Addon.Shanti
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

Every push (including PR branches) already builds the IBus Flatpak and the
Fcitx5 addon in CI and uploads them as downloadable artifacts, named
`shanti-ibus-flatpak-x86_64`, `shanti-ibus-flatpak-aarch64`,
`shanti-fcitx-addon-flatpak-x86_64` and `shanti-fcitx-addon-flatpak-aarch64`.
Grab them with `gh` using the run ID:

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

The Fcitx addon builds the same way against the Flathub Fcitx5 runtime:

```sh
flatpak-builder --user --install --force-clean \
  /tmp/shanti-fcitx-build tools/flatpak/fcitx-addon.yml
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
`--user` for a system-wide install. The Flatpak and portable installer are
separate installations.
