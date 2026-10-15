# Input Methods — IBus and Fcitx5

The portable installer builds one input-method backend per installation.

## IBus

IBus is selected for GNOME and other non-KDE/Plasma desktops.

The component file is installed at ~/.local/share/ibus/component/openbangla.xml.

The engine is installed at ~/.local/libexec/ibus-engine-openbangla.

The installer persists IBUS_COMPONENT_PATH for user services so GNOME-managed IBus can discover the component.

## Fcitx5

Fcitx5 is selected for KDE/Plasma.

The fork installs the Fcitx5 module and addon metadata under the user-local prefix and updates the user Fcitx5 profile when possible.

The portable installer does not build both backends at the same time.

## IBus-only Flatpak

The experimental Flatpak uses app ID `org.openbangla.Shanti.IBus` and includes only the IBus engine. On GNOME, its GUI can offer to register the engine with the host IBus session by writing a user component descriptor, launcher script and systemd user-service drop-in. This is separate from the portable installer and its `IBUS_COMPONENT_PATH` setup. See the [Flatpak guide](../../tools/flatpak/README.md) for permissions, conflict handling, removal and current validation limits. A successful Flatpak build does not prove that host IBus registration or Bengali typing works.
## Projects

- [IBus](https://github.com/ibus/ibus) provides the Linux input-method framework used on GNOME and other non-KDE/Plasma desktops.
- [Fcitx5](https://github.com/fcitx/fcitx5) provides the Linux input-method framework used on KDE Plasma.
