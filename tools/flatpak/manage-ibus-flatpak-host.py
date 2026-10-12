#!/usr/bin/env python3
"""Install or remove the experimental, user-scoped Flatpak-to-IBus bridge.

This is intentionally separate from the production installer. It targets the
GNOME-managed IBus systemd user service used by the verified Fedora setup.
"""
from __future__ import annotations

import argparse
import os
import re
import shutil
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent
WRAPPER_SOURCE = ROOT / "ibus-flatpak-host-wrapper.sh"
APP_ID_RE = re.compile(r"^[A-Za-z0-9_-]+(?:\.[A-Za-z0-9_-]+)+$")
MARKER = "# Managed by Shanti manage-ibus-flatpak-host.py; do not edit."


def fail(message: str, status: int = 1) -> "NoReturn":
    print(f"shanti-ibus-host: {message}", file=sys.stderr)
    raise SystemExit(status)


def home_path(value: str, label: str) -> Path:
    path = Path(value)
    if not path.is_absolute() or str(path) == "/":
        fail(f"{label} must be an absolute, non-root path")
    if any(c in str(path) for c in "\n\r\0"):
        fail(f"{label} contains an unsupported control character")
    return path


def atomic_create(path: Path, content: str, mode: int = 0o644) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    tmp = Path(tmp_name)
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(content)
        os.chmod(tmp, mode)
        # Never overwrite an existing file, including a symlink.
        os.link(tmp, path)
    except FileExistsError:
        fail(f"refusing to overwrite existing file: {path}")
    finally:
        tmp.unlink(missing_ok=True)


def managed_file(path: Path) -> bool:
    try:
        return path.is_file() and MARKER in path.read_text(encoding="utf-8")
    except (OSError, UnicodeError):
        return False


def build_component(launcher: Path) -> str:
    component = ET.Element("component")
    fields = {
        "name": "org.freedesktop.IBus.OpenBangla",
        "description": "OpenBangla Keyboard (Flatpak engine)",
        "exec": str(launcher),
        "version": "0.1",
        "author": "OpenBangla Keyboard Shanti",
        "license": "GPL-3.0-or-later",
        "homepage": "https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti",
        "textdomain": "openbangla",
    }
    for key, value in fields.items():
        ET.SubElement(component, key).text = value
    engines = ET.SubElement(component, "engines")
    engine = ET.SubElement(engines, "engine")
    for key, value in {
        "name": "OpenBangla",
        "longname": "OpenBangla Keyboard",
        "description": "OpenBangla Bengali input method running in Flatpak",
        "language": "bn",
        "license": "GPL-3.0-or-later",
        "author": "OpenBangla Keyboard Shanti",
        "layout": "us",
        "rank": "80",
        "symbol": "অ",
    }.items():
        ET.SubElement(engine, key).text = value
    ET.indent(component, space="    ")
    return '<?xml version="1.0" encoding="UTF-8"?>\n' + MARKER + "\n" + ET.tostring(component, encoding="unicode") + "\n"


def install(app_id: str) -> None:
    if not APP_ID_RE.fullmatch(app_id):
        fail("--app-id must be a reverse-DNS-style Flatpak app ID", 2)
    if not shutil.which("flatpak"):
        fail("flatpak command not found", 69)
    if not shutil.which("ibus"):
        fail("ibus command not found", 69)
    if not WRAPPER_SOURCE.is_file():
        fail(f"host wrapper not found: {WRAPPER_SOURCE}")
    import subprocess
    result = subprocess.run(
        ["flatpak", "info", app_id],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    if result.returncode:
        fail(f"Flatpak app is not installed or not visible to this user: {app_id}")

    home = home_path(os.environ.get("HOME", ""), "HOME")
    data_home = home_path(os.environ.get("XDG_DATA_HOME", str(home / ".local/share")), "XDG_DATA_HOME")
    config_home = home_path(os.environ.get("XDG_CONFIG_HOME", str(home / ".config")), "XDG_CONFIG_HOME")
    # IBus component paths use ':' as a separator.
    if ":" in str(data_home):
        fail("XDG_DATA_HOME cannot contain ':' because IBus uses it as a path separator")

    libexec = home / ".local/libexec/shanti"
    wrapper = libexec / "ibus-flatpak-host-wrapper.sh"
    launcher = home / ".local/bin/shanti-flatpak-ibus-launcher"
    component = data_home / "ibus/component/shanti-flatpak-openbangla.xml"
    dropin = config_home / "systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-flatpak-ibus.conf"

    if not os.access(home, os.W_OK):
        fail(f"home directory is not writable: {home}")
    for path in (wrapper, launcher, component, dropin):
        if path.exists() or path.is_symlink():
            fail(f"refusing to overwrite existing path: {path}")

    created: list[Path] = []
    try:
        atomic_create(wrapper, WRAPPER_SOURCE.read_text(encoding="utf-8"), 0o755)
        created.append(wrapper)
        launcher_text = f"""#!/usr/bin/env bash
{MARKER}
export SHANTI_FLATPAK_APP_ID={app_id!s}
exec {shlex_quote(str(wrapper))} "$@"
"""
        atomic_create(launcher, launcher_text, 0o755)
        created.append(launcher)
        atomic_create(component, build_component(launcher))
        created.append(component)
        dropin_text = f"""{MARKER}
[Service]
Environment="IBUS_COMPONENT_PATH={component.parent}:{Path('/usr/share/ibus/component')}"
ExecStart=
ExecStart=/usr/bin/ibus-daemon --panel disable --cache=refresh
"""
        atomic_create(dropin, dropin_text)
        created.append(dropin)
    except BaseException:
        for path in reversed(created):
            path.unlink(missing_ok=True)
        raise

    print("Installed the experimental user-scoped host IBus bridge.")
    print(f"  Component: {component}")
    print(f"  Launcher:  {launcher}")
    print(f"  Service override: {dropin}")
    print()
    print("Apply the GNOME IBus service override with:")
    print("  systemctl --user daemon-reload")
    print("  systemctl --user restart org.freedesktop.IBus.session.GNOME.service")
    print("Then verify:")
    print("  ibus list-engine --name-only | grep -i openbangla")
    print("  ibus engine")
    print("Select OpenBangla in GNOME Settings and test Bengali text entry.")


def shlex_quote(value: str) -> str:
    import shlex
    return shlex.quote(value)


def uninstall() -> None:
    home = home_path(os.environ.get("HOME", ""), "HOME")
    data_home = home_path(os.environ.get("XDG_DATA_HOME", str(home / ".local/share")), "XDG_DATA_HOME")
    config_home = home_path(os.environ.get("XDG_CONFIG_HOME", str(home / ".config")), "XDG_CONFIG_HOME")
    paths = [
        home / ".local/libexec/shanti/ibus-flatpak-host-wrapper.sh",
        home / ".local/bin/shanti-flatpak-ibus-launcher",
        data_home / "ibus/component/shanti-flatpak-openbangla.xml",
        config_home / "systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-flatpak-ibus.conf",
    ]
    present = [path for path in paths if path.exists() or path.is_symlink()]
    unmanaged = [path for path in present if not managed_file(path)]
    if unmanaged:
        fail("refusing to remove files not marked as managed by this tool:\n" + "\n".join(map(str, unmanaged)))
    for path in present:
        path.unlink()
        print(f"Removed {path}")
    print("If any files were removed, apply the change with:")
    print("  systemctl --user daemon-reload")
    print("  systemctl --user restart org.freedesktop.IBus.session.GNOME.service")
    print("Empty parent directories are left in place.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    install_parser = sub.add_parser("install", help="install the experimental user-scoped bridge")
    install_parser.add_argument("--app-id", required=True, help="ID of the installed Flatpak engine app")
    sub.add_parser("uninstall", help="remove only files created by this tool")
    args = parser.parse_args()
    if args.command == "install":
        install(args.app_id)
    else:
        uninstall()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
