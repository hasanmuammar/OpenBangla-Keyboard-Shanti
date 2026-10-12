#!/usr/bin/env python3
"""Isolated lifecycle tests for the experimental Flatpak-to-IBus manager."""
from __future__ import annotations

import importlib.util
import os
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path
from unittest import mock

SCRIPT = Path(__file__).with_name("manage-ibus-flatpak-host.py")
SPEC = importlib.util.spec_from_file_location("shanti_ibus_manager", SCRIPT)
assert SPEC and SPEC.loader
manager = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(manager)

APP_ID = "io.github.hasanmuammar.ShantiIbusEngineProbe"


class ManagerLifecycleTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.home = self.root / "home"
        self.data = self.root / 'xdg "data'
        self.config = self.root / "xdg config"
        for path in (self.home, self.data, self.config):
            path.mkdir(parents=True)
        self.env = mock.patch.dict(os.environ, {
            "HOME": str(self.home),
            "XDG_DATA_HOME": str(self.data),
            "XDG_CONFIG_HOME": str(self.config),
        }, clear=False)
        self.env.start()
        self.addCleanup(self.env.stop)
        self.which = mock.patch.object(manager.shutil, "which", side_effect=lambda name: f"/mock/bin/{name}")
        self.which.start()
        self.addCleanup(self.which.stop)
        self.real_run = subprocess.run

        def fake_flatpak(command, *args, **kwargs):
            if command == ["flatpak", "info", APP_ID]:
                return subprocess.CompletedProcess(command, 0)
            return self.real_run(command, *args, **kwargs)

        self.run_flatpak = mock.patch("subprocess.run", side_effect=fake_flatpak)
        self.run_flatpak.start()
        self.addCleanup(self.run_flatpak.stop)

    def paths(self) -> list[Path]:
        return [
            self.home / ".local/libexec/shanti/ibus-flatpak-host-wrapper.sh",
            self.home / ".local/bin/shanti-flatpak-ibus-launcher",
            self.data / "ibus/component/shanti-flatpak-openbangla.xml",
            self.config / "systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-flatpak-ibus.conf",
        ]

    def test_install_generates_valid_files_and_uninstall_removes_them(self) -> None:
        manager.install(APP_ID)
        wrapper, launcher, component, dropin = self.paths()
        for path in self.paths():
            self.assertTrue(path.is_file(), path)
            self.assertIn(manager.MARKER_TEXT, path.read_text(encoding="utf-8"))
        self.assertEqual(wrapper.stat().st_mode & 0o777, 0o755)
        self.assertEqual(launcher.stat().st_mode & 0o777, 0o755)
        subprocess.run(["bash", "-n", str(wrapper)], check=True)
        subprocess.run(["bash", "-n", str(launcher)], check=True)
        tree = ET.parse(component)
        self.assertEqual(tree.findtext("./engines/engine/name"), "OpenBangla")
        self.assertEqual(tree.findtext("./engines/engine/layout"), "default")
        self.assertEqual(tree.findtext("./exec"), str(launcher))
        unit = dropin.read_text(encoding="utf-8")
        self.assertIn('Environment="IBUS_COMPONENT_PATH=', unit)
        self.assertIn(r'xdg \"data', unit)
        self.assertIn("%%", manager.systemd_escape("%"))
        manager.uninstall()
        self.assertFalse(any(path.exists() or path.is_symlink() for path in self.paths()))

    def test_refuses_existing_unmanaged_target_without_overwriting(self) -> None:
        wrapper = self.paths()[0]
        wrapper.parent.mkdir(parents=True)
        wrapper.write_text("user data\n", encoding="utf-8")
        with self.assertRaises(SystemExit):
            manager.install(APP_ID)
        self.assertEqual(wrapper.read_text(encoding="utf-8"), "user data\n")
        self.assertFalse(any(path.exists() for path in self.paths()[1:]))

    def test_install_refuses_conflicting_manual_service_dropin(self) -> None:
        conflict = self.config / "systemd/user/org.freedesktop.IBus.session.GNOME.service.d/90-shanti-probe.conf"
        conflict.parent.mkdir(parents=True)
        conflict.write_text(
            '[Service]\\nEnvironment="IBUS_COMPONENT_PATH=/tmp/component:/usr/share/ibus/component"\\n'
            'ExecStart=\\nExecStart=/usr/bin/ibus-daemon --panel disable --cache=refresh\\n',
            encoding="utf-8",
        )
        with self.assertRaisesRegex(SystemExit, "potentially conflicting IBus service drop-in"):
            manager.install(APP_ID)
        self.assertTrue(conflict.exists())
        self.assertFalse(any(path.exists() or path.is_symlink() for path in self.paths()))

    def test_uninstall_refuses_unmanaged_file_and_leaves_everything(self) -> None:
        wrapper, launcher, _, _ = self.paths()
        wrapper.parent.mkdir(parents=True)
        launcher.parent.mkdir(parents=True)
        wrapper.write_text(manager.MARKER + "\nmanaged\n", encoding="utf-8")
        launcher.write_text("#!/bin/sh\nunmanaged\n", encoding="utf-8")
        with self.assertRaises(SystemExit):
            manager.uninstall()
        self.assertTrue(wrapper.exists())
        self.assertTrue(launcher.exists())

    def test_uninstall_refuses_symlink_even_when_target_has_marker(self) -> None:
        wrapper, launcher, _, _ = self.paths()
        wrapper.parent.mkdir(parents=True)
        launcher.parent.mkdir(parents=True)
        target = self.root / "marked-target"
        target.write_text(manager.MARKER + "\n", encoding="utf-8")
        wrapper.write_text(manager.MARKER + "\n", encoding="utf-8")
        launcher.symlink_to(target)
        with self.assertRaises(SystemExit):
            manager.uninstall()
        self.assertTrue(wrapper.exists())
        self.assertTrue(launcher.is_symlink())
        self.assertTrue(target.exists())

    def test_failed_install_rolls_back_previously_created_files(self) -> None:
        original = manager.atomic_create
        calls = 0

        def fail_third(path: Path, content: str, mode: int = 0o644) -> None:
            nonlocal calls
            calls += 1
            if calls == 3:
                raise OSError("simulated disk failure")
            original(path, content, mode)

        with mock.patch.object(manager, "atomic_create", side_effect=fail_third):
            with self.assertRaisesRegex(OSError, "simulated disk failure"):
                manager.install(APP_ID)
        self.assertFalse(any(path.exists() or path.is_symlink() for path in self.paths()))

    def test_invalid_app_id_is_rejected_before_installation(self) -> None:
        with self.assertRaises(SystemExit) as raised:
            manager.install("not an app id")
        self.assertEqual(raised.exception.code, 2)
        self.assertFalse(any(path.exists() for path in self.paths()))


if __name__ == "__main__":
    unittest.main(verbosity=2)
