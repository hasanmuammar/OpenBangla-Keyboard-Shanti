# shanti-keysyms

Single source of truth for the key event **keysym → riti virtual-key**
mapping shared by the IBus engine and the Fcitx5 engine. Replaces the two
112-entry tables that used to live in the engines' `keycode.h` headers.

## Layout

- `data/keysyms.tsv` — committed source of truth: `keysym_value`, `keysym_name`, `VC_*` name.
- `src/data.rs` — generated from the TSV, sorted array of `(keysym_value, VC_* name)`.
- `src/lib.rs` — Rust lookup (`pub fn lookup`) + C ABI `shanti_keysyms_lookup`
  (`include/shanti-keysyms.h`), C++ engines link this; unknown keysyms map to
  `VC_UNKNOWN` (0x0046, the engines' historical fallback).
- `generator/keysyms_extract.py` — regeneration script (needs IBus + Fcitx5
  dev headers to probe the real enums; asserts engine tables, IBus enum, and
  Fcitx enum all agree, and that keysym values are unique).

## Why names, not numbers

The table stores riti virtual-key *names*; numeric values are resolved at
build time from the actual riti submodule header (`include_str!` of
`src/engine/riti/include/riti.h`), so riti bumps never require a data
regeneration for virtual-key renumbering — `cargo test` fails if a name is
not defined or the TSV and generated table drift apart.

## Regenerating

```bash
crates/shanti-keysyms/generator/keysyms_extract.py --diff
cargo test  # in this crate; asserts TSV ↔ data.rs ↔ riti.h consistency
```

Then rebuild the engines (`bash tools/build.sh --backend ibus --package` and
`--backend fcitx`) and re-verify typing behaviour before committing.
