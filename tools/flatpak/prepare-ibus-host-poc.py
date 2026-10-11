#!/usr/bin/env python3
"""Generate and validate disposable host-side IBus POC files; never registers them."""
from __future__ import annotations

import argparse
import re
import subprocess
import shlex
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parent
APP_ID_DEFAULT = "io.github.hasanmuammar.ShantiIbusEngineProbe"
APP_ID_RE = re.compile(r"^[A-Za-z0-9_-]+(?:\.[A-Za-z0-9_-]+)+$")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app-id", default=APP_ID_DEFAULT)
    parser.add_argument(
        "--wrapper-path",
        required=True,
        help="absolute path where the host wrapper will be installed in the disposable account",
    )
    parser.add_argument(
        "--output-dir",
        help="output directory (defaults to a newly created temporary directory)",
    )
    args = parser.parse_args()

    if not APP_ID_RE.fullmatch(args.app_id):
        parser.error("--app-id must be a reverse-DNS-style identifier")
    wrapper_path = Path(args.wrapper_path)
    if not wrapper_path.is_absolute():
        parser.error("--wrapper-path must be absolute")
    if "\n" in str(wrapper_path) or "\r" in str(wrapper_path):
        parser.error("--wrapper-path must not contain newlines")

    if args.output_dir:
        out = Path(args.output_dir).expanduser().resolve()
        out.mkdir(parents=True, exist_ok=True)
    else:
        out = Path(tempfile.mkdtemp(prefix="shanti-ibus-host-poc-"))

    wrapper_template = (ROOT / "ibus-host-wrapper-poc.sh.in").read_text(encoding="utf-8")
    component_template = (ROOT / "ibus-host-component-poc.xml.in").read_text(encoding="utf-8")
    wrapper = wrapper_template.replace("@SHANTI_POC_APP_ID@", args.app_id)
    component_exec = shlex.quote(str(wrapper_path))
    component = component_template.replace("@SHANTI_POC_WRAPPER_PATH@", escape(component_exec))

    if "@SHANTI_POC_APP_ID@" in wrapper or "@SHANTI_POC_WRAPPER_PATH@" in component:
        raise SystemExit("error: an unsubstituted template token remains")

    wrapper_out = out / "shanti-ibus-flatpak-wrapper"
    component_out = out / "openbangla-flatpak-poc.xml"
    wrapper_out.write_text(wrapper, encoding="utf-8")
    wrapper_out.chmod(0o755)
    component_out.write_text(component, encoding="utf-8")

    subprocess.run(["bash", "-n", str(wrapper_out)], check=True)
    root = ET.parse(component_out).getroot()
    if root.findtext("name") != "org.freedesktop.IBus.OpenBangla":
        raise SystemExit("error: unexpected IBus component name")
    exec_text = root.findtext("exec") or ""
    try:
        exec_argv = shlex.split(exec_text)
    except ValueError as exc:
        raise SystemExit(f"error: invalid component exec quoting: {exc}") from exc
    if exec_argv != [str(wrapper_path)]:
        raise SystemExit("error: component exec must resolve to exactly the wrapper path with no extra arguments")

    print("Validated generated POC files (not installed or registered):")
    print(f"  wrapper:   {wrapper_out}")
    print(f"  component: {component_out}")
    print(f"  app ID:    {args.app_id}")
    print(f"  component exec: {root.findtext('exec')}")
    print("Checks passed: app ID syntax, token substitution, bash -n, XML parse, component name and exec path.")
    print("Review both files before any manual test. This script does not install or register them.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
