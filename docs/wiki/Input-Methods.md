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

The experimental Flatpak uses app ID `org.openbangla.Shanti.IBus` and includes only the IBus engine. On GNOME, the app offers first-launch setup and has **GNOME IBus Integration** controls in its Settings panel to show status, set up or repair the host registration, and remove it. Setup writes a user component descriptor, launcher script and systemd user-service drop-in, then reloads systemd and restarts GNOME IBus. This is separate from the portable installer's `IBUS_COMPONENT_PATH` setup. See the [Flatpak guide](../../tools/flatpak/README.md) for the step-by-step flow, permissions and removal details.
## Projects

- [IBus](https://github.com/ibus/ibus) provides the Linux input-method framework used on GNOME and other non-KDE/Plasma desktops.
- [Fcitx5](https://github.com/fcitx/fcitx5) provides the Linux input-method framework used on KDE Plasma.
