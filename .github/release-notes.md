Shanti Keyboard Shanti is an experimental Linux-focused fork. This alpha release includes:

- Portable user-local archives for IBus and Fcitx5 on x86_64 and aarch64.
- IBus-only Flatpak bundles for x86_64 and aarch64. Install the bundle matching your system architecture; the Flatpak's GNOME host-IBus integration is managed from the app's Settings panel.
- SHA-256 checksums and GitHub build-provenance attestations for the published archives and Flatpak bundles.

The Flatpak bundle is a direct download, not a configured Flatpak remote. It requires the separate Flathub runtime `org.kde.Platform//5.15-25.08`, which Flatpak normally downloads during app installation when Flathub is configured in the same user or system scope. See the [Flatpak guide](https://github.com/hasanmuammar/Shanti-Keyboard-Shanti/blob/develop/tools/flatpak/README.md) for installation and GNOME setup. See the [installation guide](https://github.com/hasanmuammar/Shanti-Keyboard-Shanti/blob/develop/docs/wiki/Installation.md) for the portable archives.
