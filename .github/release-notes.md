Shanti Keyboard Shanti is an experimental Linux-focused fork. This alpha release is distributed as Flatpak bundles only:

- Fcitx5 app bundle (`org.shanti.ShantiFcitx5`) for x86_64 and aarch64: one self-contained bundle carrying the whole Fcitx5 stack — the daemon, the config tool, the Qt input-context plugin, our engine and the same `shanti-gui` settings GUI as the IBus bundle — so no host Fcitx5 is needed.
- IBus app bundle (`org.shanti.ShantiIbus`) for x86_64 and aarch64, integrating with the host's GNOME IBus setup.
- Both bundles build on the same Qt5 runtime (`org.kde.Platform//5.15-25.08`) and give the same frontend experience; only the engine backend differs per desktop.
- SHA-256 checksums and GitHub build-provenance attestations for the published Flatpak bundles.

Both bundles pull their runtime from Flathub during installation when Flathub is configured in the same scope. The Fcitx5 bundle builds its Fcitx5 stack from pinned upstream tags (self-contained; not based on Flathub's app) and registers Fcitx5's own session bus, so it must not run alongside Flathub's own `org.fcitx.Fcitx5` app. See the [Flatpak guide](https://github.com/hasanmuammar/Shanti-Keyboard-Shanti/blob/develop/tools/flatpak/README.md) for installation and per-desktop setup.
