# Uninstallation

## Portable user-local installation

Remove the current-user installation:

```bash
bash tools/uninstall.sh
```

The uninstaller removes Shanti's installed binaries, bundled runtime, desktop entry, icons, metadata, input-method registration and fork-specific environment configuration. It may also remove the Shanti (and legacy OpenBangla) entries from the user Fcitx5 profile when present. It cleans both the current Shanti names and the legacy pre-rebrand names (`openbangla-gui`, `ibus-engine-openbangla`, `openbangla-keyboard`, `org.freedesktop.IBus.OpenBangla` registration). It does not remove the source checkout or system-wide IBus, Fcitx5, Qt, compilers or other shared packages.

**User data is preserved by default.** The ordinary uninstall does not purge personal settings/layout data or the cache. Review the script's prompts and options before requesting deletion:

```bash
bash tools/uninstall.sh --purge-data
bash tools/uninstall.sh --purge-cache
```

Use the purge options only when you intend to remove the corresponding user data or build/staging cache. The script asks for confirmation for destructive purge operations. Do not assume that uninstalling the application also deletes every user preference.

## IBus-only Flatpak

The Flatpak's GNOME IBus integration is separate from the portable installer. In the app's **Settings** panel, open **GNOME IBus Integration** and choose **Remove integration**. Confirm the action. This removes the app-managed component descriptor, launcher and GNOME IBus service drop-in, then reloads systemd and restarts GNOME IBus. You can also use the tray menu's **GNOME IBus Integration…** action. Remove the **Shanti** entry from GNOME Settings → Keyboard → Input Sources if it remains in the saved list. Then uninstall the Flatpak separately using your software manager or `flatpak uninstall --user org.shanti.ShantiIbus`.

The Fcitx5 Flatpak (`org.shanti.ShantiFcitx5`) is removed with `flatpak uninstall --user org.shanti.ShantiFcitx5`. If you created the suggested autostart entry, remove `~/.config/autostart/org.shanti.ShantiFcitx5.desktop` as well. The bundle carries its own Fcitx5 stack per user; nothing on the host references it.

If the GUI is no longer available, inspect these paths before manually removing anything:

- `~/.local/share/ibus/component/shanti-flatpak.xml` (current) or `-openbangla.xml` (pre-rebrand)
- `~/.local/share/shanti-ibus/ibus-flatpak-host-wrapper.sh`
- `~/.config/systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-flatpak-ibus.conf`

Only remove files that were created by this Shanti integration. Do not delete an unrelated user service override. After manual cleanup, reload the user systemd manager and restart the GNOME IBus service. Flatpak app data and the portable installation are separate; removing one does not automatically remove the other.
