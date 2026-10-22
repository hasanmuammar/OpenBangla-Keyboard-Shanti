#!/usr/bin/env python3
"""Regenerate the shanti-keysyms single-source data.

Rebuilds data/keysyms.tsv (source of truth) and src/data.rs (generated
array) from the engines' keysym tables, verifying the two engines agree
with each other and with the real IBus / Fcitx keysym enums.

Needs a compile toolchain plus the IBus >= 1.5 and Fcitx5 dev headers
(they provide the IBUS_KEY_* / FcitxKey_* values). Run from anywhere
inside a repository checkout:

    ./generator/keysyms_extract.py            # writes TSV + data.rs
    ./generator/keysyms_extract.py --diff     # ... and print a diff summary

Manual probes are compiled into the system temp dir and cleaned up.
"""

import argparse
import os
import re
import subprocess
import tempfile

CRATE_DIR = os.path.dirname(os.path.abspath(__file__)) + "/.."
REPO_ROOT = os.path.abspath(CRATE_DIR + "/../..")
IBUS_KEYCODE_H = REPO_ROOT + "/src/engine/ibus/keycode.h"
FCITX_KEYCODE_H = REPO_ROOT + "/src/engine/fcitx/keycode.h"

IBUS_PATTERN = re.compile(r"IBUS_KEY_(\w+)\]\s*=\s*(VC_\w+)\s*;")
FCITX_PATTERN = re.compile(r"\{\s*FcitxKey_(\w+)\s*,\s*(VC_\w+)\s*\}\s*,")

HEADER = """\
# Single source of keysym mapping (phase-2 task 1).
# Extracted from src/engine/{ibus,fcitx}/keycode.h, verified identical on both
# platforms and probed against the real IBus/Fcitx enums (see generator/keysyms_extract.py).
# columns: keysym_value(X keysyms, decimal)\tkeysym_name\triti_virtual_key
"""


def sh(command, fail_message=None):
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(f"{fail_message or 'command failed'}: {command}\n{result.stderr}")
    return result.stdout.strip()


def parse_engine_tables():
    ibus = IBUS_PATTERN.findall(open(IBUS_KEYCODE_H).read())
    fcitx = FCITX_PATTERN.findall(open(FCITX_KEYCODE_H).read())
    if sorted(ibus) != sorted(fcitx):
        raise SystemExit(
            "the engine tables diverge: "
            f"only in ibus: {sorted(set(ibus) - set(fcitx))} "
            f"only in fcitx: {sorted(set(fcitx) - set(ibus))}"
        )
    return ibus


def probe_keysym_values(pairs):
    """Compile a program against the REAL IBus + Fcitx enums and print the
    keysym value of every mapped name from both, to prove parity."""
    keysym_names = [name for name, _vc in pairs]
    printf_lines = "\n".join(
        f'  printf("%s %d %d\\n", "{name}", IBUS_KEY_{name}, FcitxKey_{name});'
        for name in keysym_names
    )
    probe_source = (
        "#include <ibus.h>\n"
        "#include <fcitx-utils/keysym.h>\n"
        "#include <cstdio>\nint main() {\n" + printf_lines + "\n  return 0;\n}\n"
    )
    ibus_cflags = sh("pkg-config --cflags ibus-1.0", "ibus dev headers not found")
    fcitx_cflags = sh(
        "pkg-config --cflags Fcitx5Utils || echo -I/usr/include/Fcitx5/Utils",
        "fcitx5 dev headers not found",
    )
    with tempfile.TemporaryDirectory(prefix="shanti-keysyms-gen-") as workdir:
        source_path = os.path.join(workdir, "probe.cpp")
        binary_path = os.path.join(workdir, "probe")
        output_path = os.path.join(workdir, "values.txt")
        open(source_path, "w").write(probe_source)
        sh(
            f"g++ {source_path} {ibus_cflags} {fcitx_cflags} -o {binary_path}",
            "probe compilation failed",
        )
        sh(f"{binary_path} > {output_path}", "probe run failed")
        # name -> (ibus value, fcitx value); assert both platforms agree.
        values = {}
        for line in open(output_path):
            name, ibus_value, fcitx_value = line.split()
            if int(ibus_value) != int(fcitx_value):
                raise SystemExit(f"keysym enums disagree for {name}: {ibus_value} vs {fcitx_value}")
            values[name] = int(ibus_value)
        if sorted(values) != sorted(keysym_names):
            raise SystemExit("the probe printed an unexpected name set")
    return values


def build_rows():
    pairs = parse_engine_tables()
    if len({name for name, _ in pairs}) != len(pairs):  # duplicate keysym name
        raise SystemExit("duplicate keysym name in an engine table")
    values = probe_keysym_values(pairs)
    names_to_vc = dict(pairs)
    rows = sorted((value, name, names_to_vc[name]) for name, value in values.items())
    if len({row[0] for row in rows}) != len(rows):
        raise SystemExit("duplicate keysym VALUE across the table - "
                         "single-source lookup by value would be ambiguous")
    return rows


def write_outputs(rows):
    with open(CRATE_DIR + "/data/keysyms.tsv", "w") as tsv:
        tsv.write(HEADER)
        for value, name, vc in rows:
            tsv.write(f"{value}\t{name}\t{vc}\n")
    data_rows = "\n".join(f'    ({value}, "{vc}"),' for value, _name, vc in rows)
    open(CRATE_DIR + "/src/data.rs", "w").write(
        "// GENERATED from data/keysyms.tsv by generator/keysyms_extract.py - do not edit.\n"
        "// Sorted by keysym value (X keysym, valid for both IBus and Fcitx enums).\n\n"
        "pub static KEYSYMS: [(u32, &str); %d] = [\n%s\n];\n" % (len(rows), data_rows)
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--diff", action="store_true", help="report which outputs changed")
    args = parser.parse_args()

    paths = (CRATE_DIR + "/data/keysyms.tsv", CRATE_DIR + "/src/data.rs")
    before = {path: (open(path).read() if os.path.exists(path) else None) for path in paths}

    rows = build_rows()
    write_outputs(rows)
    print(f"OK: {len(rows)} entries, engines + real IBus/Fcitx enums in agreement.")

    for path in paths:
        if open(path).read() != before[path]:
            print(f"changed: {os.path.relpath(path, REPO_ROOT)}")


if __name__ == "__main__":
    main()
