//! Single source of truth for the key event keysym -> riti virtual-key
//! mapping, shared by the IBus engine and the Fcitx5 engine.
//!
//! Before this crate each engine carried its own 112-entry table in its
//! `keycode.h`. The two tables were extracted, verified byte-identical
//! against each other *and* against the real IBus/Fcitx keysym enums (see
//! `generator/keysyms_extract.py`), and reduced to `data/keysyms.tsv`
//! (committed source of truth) and `data.rs` (generated, sorted array).
//!
//! The table stores riti virtual-key *names* (`VC_*`). Numeric values are
//! resolved at build time from the actual riti header
//! (`src/engine/riti/include/riti.h`, included with `include_str!`), so the
//! mapping stays in sync when the riti submodule is bumped — no numbers are
//! frozen here. The engines consume this over the C ABI via
//! `shanti_keysyms_lookup()` (see `include/shanti-keysyms.h`).

use std::sync::OnceLock;

// Generated from data/keysyms.tsv by generator/keysyms_extract.py - do not
// edit. Sorted ascending by keysym value; values are the same in the IBus
// and Fcitx keysym enums (verified).
include!("data.rs");

const RITI_HEADER: &str = include_str!("../../../src/engine/riti/include/riti.h");

/// Fallback for `VC_UNKNOWN`, which riti's generated header does not define.
/// Both engine `keycode.h` files historically used this exact constant; the
/// tests assert it stays in agreement if riti ever starts defining the name.
pub const VC_UNKNOWN_FALLBACK: u16 = 0x0046;

/// Value riti assigns to its `VC_UNKNOWN` virtual key. Not present in
/// riti's generated header, but engine `keycode.h` files historically used
/// this exact constant; the lookup fallback must agree with it.
pub const VC_UNKNOWN_VALUE: u16 = VC_UNKNOWN_FALLBACK;

/// `#define VC_*  <number>` entries parsed from the committed riti header.
fn riti_vc_defines() -> &'static [(&'static str, u16)] {
    static DEFINES: OnceLock<Vec<(&'static str, u16)>> = OnceLock::new();
    DEFINES.get_or_init(|| {
        let mut defines = Vec::new();
        for line in RITI_HEADER.lines() {
            let rest = match line.trim().strip_prefix("#define") {
                Some(rest) => rest,
                None => continue,
            };
            let mut parts = rest.split_whitespace();
            let name = match parts.next() {
                Some(name) => name,
                None => continue,
            };
            if !name.starts_with("VC_") {
                continue;
            }
            if let Ok(value) = parts.next().unwrap_or_default().parse::<u16>() {
                defines.push((name, value));
            }
        }
        defines
    })
}

/// Numeric value of a riti virtual-key name, taken from the real riti
/// header this crate is compiled against.
pub fn vc_value(name: &str) -> Option<u16> {
    riti_vc_defines()
        .iter()
        .find(|entry| entry.0 == name)
        .map(|entry| entry.1)
}

fn vc_unknown() -> u16 {
    vc_value("VC_UNKNOWN").unwrap_or(VC_UNKNOWN_FALLBACK)
}

/// Map a key event keysym to a riti virtual-key number.
///
/// Keysym values are identical in the IBus and Fcitx enums (both follow X
/// keysyms), so a single table serves both engines. Unmapped keysyms fall
/// back to `VC_UNKNOWN`, matching the previous per-engine behaviour.
pub fn lookup(keysym: u32) -> u16 {
    match KEYSYMS.binary_search_by(|entry| entry.0.cmp(&keysym)) {
        Ok(index) => vc_value(KEYSYMS[index].1).unwrap_or_else(vc_unknown),
        Err(_) => vc_unknown(),
    }
}

/// C ABI for the C++ engine shells: `shanti_keysyms_lookup(keysym)`.
#[no_mangle]
pub extern "C" fn shanti_keysyms_lookup(keysym: u32) -> u16 {
    lookup(keysym)
}

#[cfg(test)]
mod tests {
    use super::*;

    const TSV: &str = include_str!("../data/keysyms.tsv");

    fn tsv_rows() -> Vec<(u32, &'static str, &'static str)> {
        TSV.lines()
            .filter(|line| !line.is_empty() && !line.starts_with('#'))
            .map(|line| {
                let mut cols = line.split('\t');
                let value: u32 = cols.next().expect("keysym value column").parse().expect("numeric keysym value");
                let name = cols.next().expect("keysym name column");
                let vc = cols.next().expect("riti virtual-key column");
                assert!(cols.next().is_none(), "unexpected extra column: {line}");
                (value, name, vc)
            })
            .collect()
    }

    #[test]
    fn generated_table_matches_tsv() {
        let rows = tsv_rows();
        assert_eq!(KEYSYMS.len(), rows.len(), "data.rs and TSV row counts differ");
        for (index, (value, _name, vc)) in rows.iter().enumerate() {
            assert_eq!(KEYSYMS[index].0, *value, "row {index}: keysym value");
            assert_eq!(KEYSYMS[index].1, *vc, "row {index} ({value}): riti key name");
        }
    }

    #[test]
    fn table_sorted_and_values_unique() {
        let rows = tsv_rows();
        for pair in rows.windows(2) {
            assert!(pair[0].0 < pair[1].0, "TSV rows must ascend strictly: {} {}", pair[0].0, pair[1].0);
        }
        for pair in KEYSYMS.windows(2) {
            assert!(pair[0].0 < pair[1].0, "KEYSYMS must ascend strictly: {} {}", pair[0].0, pair[1].0);
        }
    }

    #[test]
    fn every_vc_name_defined_in_riti_header() {
        for entry in KEYSYMS.iter() {
            let value = vc_value(entry.1);
            assert!(value.is_some(), "{} missing from riti.h defines", entry.1);
        }
    }

    #[test]
    fn vc_unknown_agrees_with_engine_fallback() {
        // The engines' hard-coded fallback lives on as VC_UNKNOWN_FALLBACK.
        assert_eq!(VC_UNKNOWN_FALLBACK, 0x0046);
        if let Some(value) = vc_value("VC_UNKNOWN") {
            assert_eq!(value, VC_UNKNOWN_FALLBACK, "riti.h defines VC_UNKNOWN differently");
        }
        // Unmapped keysyms must land on the fallback.
        assert_eq!(lookup(u32::MAX), VC_UNKNOWN_FALLBACK);
    }

    #[test]
    fn well_known_mappings() {
        // Spot probes from the TSV: 0 → '0', grave, dead_grave, shifted A.
        let rows = tsv_rows();
        for (value, name, vc) in rows.iter().take(6) {
            assert_eq!(lookup(*value), vc_value(vc).unwrap(), "{name}");
        }
        // Lowercase 'a' is the alphanumeric anchor used by the old engines.
        assert_eq!(lookup(97), vc_value("VC_A").unwrap());
    }

    #[test]
    fn ffi_matches_lookup() {
        for entry in KEYSYMS.iter().take(10) {
            assert_eq!(shanti_keysyms_lookup(entry.0), lookup(entry.0));
        }
        assert_eq!(shanti_keysyms_lookup(u32::MAX), VC_UNKNOWN_FALLBACK);
    }
}
