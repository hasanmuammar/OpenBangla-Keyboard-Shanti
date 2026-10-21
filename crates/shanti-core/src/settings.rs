//! Settings application: reads the GUI-written settings store (the same
//! `$XDG_CONFIG_HOME/OpenBangla/Keyboard.conf` file the Qt GUI saves) and
//! maps it onto the riti configuration, mirroring the old
//! `update_with_settings()` in `src/engine/ibus/main.cpp` with its exact
//! defaults.

use riti::config::Config as RitiConfig;
use shanti_config::Config as Store;

/// Viewport settings the platform shell renders from, besides the riti
/// config itself.
pub struct Viewport {
    pub candidates_horizontal: bool,
    pub enter_key_closes_prev_win: bool,
}

/// Read the settings store from disk and map it into `config`.
/// A missing/unreadable store behaves like an empty store (defaults).
pub fn apply(config: &mut RitiConfig) -> Viewport {
    let store = load_store();
    map_store(config, &store)
}

/// Mapping-only variant for tests: map a parsed settings store without any
/// filesystem access.
pub fn map_store(config: &mut RitiConfig, store: &Store) -> Viewport {
    let layout_path = resolve_layout_path(&store.get_string("layout/path", "avro_phonetic"));
    if !config.set_layout_file_path(&layout_path) {
        setting_error("layout file", &layout_path);
    }

    config.set_suggestion_include_english(store.get_bool("settings/PreviewWin/IncludeEnglish", true));
    config.set_phonetic_suggestion(store.get_bool("settings/CandidateWin/Phonetic", true));

    // The database directory is derived from the engine data dir; riti
    // prefixes its suffix/auto-correct filenames onto it.
    let database_dir = shanti_paths::engine_data_dir().join("data");
    if !config.set_database_dir(database_dir.to_string_lossy().as_ref()) {
        setting_error("database directory", database_dir.to_string_lossy().as_ref());
    }

    let viewport = Viewport {
        candidates_horizontal: store.get_bool("settings/CandidateWin/Horizontal", true),
        enter_key_closes_prev_win: store.get_bool("settings/EnterKeyClosesPrevWin", false),
    };

    // Fixed layout / preview behaviour and the remaining toggles, mirroring
    // the old cfg_bool() call block.
    config.set_fixed_suggestion(store.get_bool("settings/FixedLayout/ShowPrevWin", true));
    config.set_fixed_automatic_vowel(store.get_bool("settings/FixedLayout/AutoVowelForm", true));
    config.set_fixed_automatic_chandra(store.get_bool("settings/FixedLayout/AutoChandraPos", true));
    config.set_fixed_traditional_kar(store.get_bool("settings/FixedLayout/TraditionalKar", false));
    config.set_fixed_old_kar_order(store.get_bool("settings/FixedLayout/OldKarOrder", false));
    config.set_fixed_old_reph(store.get_bool("settings/FixedLayout/OldReph", true));
    config.set_fixed_numpad(store.get_bool("settings/FixedLayout/NumberPad", true));
    config.set_ansi_encoding(store.get_bool("settings/ANSI", false));
    config.set_smart_quote(store.get_bool("settings/SmartQuoting", true));

    viewport
}

fn load_store() -> Store {
    match Store::load_file(&shanti_paths::settings_file()) {
        Ok(store) => store,
        Err(_) => Store::parse(""),
    }
}

/// The host GUI can save an absolute path to its layout file. When the
/// engine runs inside a Flatpak, that host path may not exist in the
/// sandbox; resolve bundled layouts by filename against the engine's own
/// data tree (old `resolve_layout_path` fallback).
pub fn resolve_layout_path(configured_path: &str) -> String {
    use std::path::Path;

    let path = Path::new(configured_path);
    if !path.exists() && path.is_absolute() {
        let bundled = shanti_paths::engine_data_dir()
            .join("layouts")
            .join(path.file_name().unwrap_or_default());
        if bundled.is_file() {
            return bundled.to_string_lossy().into_owned();
        }
    }
    configured_path.to_owned()
}

fn setting_error(kind: &str, path: &str) {
    eprintln!("[IM:Shanti core]: Failed to set {}: {}", kind, path);
}
