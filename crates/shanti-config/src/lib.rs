/*
 *  Shanti config — read-only reader for the OpenBangla Keyboard settings
 *  store shared with the Qt GUI.
 *
 *  The store is an INI file written by QSettings("OpenBangla", "Keyboard")
 *  (NativeFormat == IniFormat on Linux) at
 *  $XDG_CONFIG_HOME/OpenBangla/Keyboard.conf. Ground truth for the format
 *  was captured from Qt 5.15 (see tests/fixtures/keyboard.conf):
 *
 *      [layout]
 *      name=Avro Phonetic
 *      path=avro_phonetic
 *
 *      [settings]
 *      ANSI=false
 *      CandidateWin\Horizontal=true
 *      FixedLayout\ShowPrevWin=true
 *      TrayInfoCount=3
 *
 *      [window]
 *      positions\TopBar=@Point(120 240)
 *      visibility\TopBar=true
 *
 *  Key lookup mirrors src/shared/Settings.cpp ("settings/FixedLayout/X"):
 *  the first path component selects the section, the remaining components
 *  are joined with '\' to form the key, exactly as QSettings serializes it.
 *
 *  The writer stays in Qt (frontend) until the GUI rewrite (phase 4); this
 *  crate deliberately refuses to write so both formats can never diverge.
 */

use std::collections::HashMap;
use std::ffi::{c_char, c_int, CString};
use std::path::Path;
use std::sync::RwLock;

#[derive(Debug, Default, PartialEq)]
pub struct Config {
    entries: HashMap<(String, String), String>,
}

impl Config {
    /// Parse QSettings-style INI text. Comments (';'/'#'), blank lines and
    /// section headers `[...]` behave like QSettings; keys and values are
    /// trimmed. Duplicate `(section, key)` entries: last wins (QSettings
    /// semantics). Values are kept raw (including @-prefixed variants such
    /// as "@Point(x y)").
    pub fn parse(text: &str) -> Config {
        let mut entries = HashMap::new();
        let mut section = String::new();
        for raw_line in text.lines() {
            let line = raw_line.trim();
            if line.is_empty() || line.starts_with(';') || line.starts_with('#') {
                continue;
            }
            if line.starts_with('[') && line.ends_with(']') {
                section = line[1..line.len() - 1].trim().to_string();
                continue;
            }
            if let Some(eq) = line.find('=') {
                let key = line[..eq].trim().to_string();
                let value = line[eq + 1..].trim().to_string();
                if !key.is_empty() {
                    entries.insert((section.clone(), key), value);
                }
            }
            // Lines without '=' are not representable in QSettings INI
            // output; skip them instead of failing on hand-edited files.
        }
        Config { entries }
    }

    /// Load a store from disk; a missing file is an empty store (defaults).
    pub fn load_file(path: &Path) -> std::io::Result<Config> {
        match std::fs::read_to_string(path) {
            Ok(text) => Ok(Config::parse(&text)),
            Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(Config::default()),
            Err(e) => Err(e),
        }
    }

    /// Map a Settings.cpp-style key ("settings/FixedLayout/X") onto the
    /// (section, key) pair as serialized by QSettings: first component is
    /// the section, the rest are joined with '\'.
    fn location(qt_key: &str) -> (String, String) {
        let mut parts = qt_key.split('/');
        let section = parts.next().unwrap_or("").to_string();
        let key = parts.collect::<Vec<_>>().join("\\");
        (section, key)
    }

    fn raw(&self, qt_key: &str) -> Option<&String> {
        let loc = Self::location(qt_key);
        self.entries.get(&loc)
    }

    /// QVariant::toBool for the values the GUI writes: "true"/"1" → true,
    /// "false"/"0" → false, anything else → dflt.
    pub fn get_bool(&self, qt_key: &str, dflt: bool) -> bool {
        match self.raw(qt_key).map(String::as_str) {
            Some("true") | Some("1") => true,
            Some("false") | Some("0") => false,
            _ => dflt,
        }
    }

    pub fn get_uint(&self, qt_key: &str, dflt: u32) -> u32 {
        self.raw(qt_key)
            .and_then(|v| v.parse::<u32>().ok())
            .unwrap_or(dflt)
    }

    /// Raw string value, or dflt when absent. No unescaping yet: plain ASCII
    /// values (the only kind the current keys hold) round-trip directly,
    /// but phase 4 must add full QSettings INI unescaping before writing.
    pub fn get_string(&self, qt_key: &str, dflt: &str) -> String {
        self.raw(qt_key).cloned().unwrap_or_else(|| dflt.to_string())
    }
}

/* --------------------------- global store --------------------------- */

static STORE: RwLock<Option<Config>> = RwLock::new(None);

fn store_load(path: &Path) -> c_int {
    match Config::load_file(path) {
        Ok(config) => {
            *STORE.write().expect("shanti-config store poisoned") = Some(config);
            1
        }
        Err(_) => 0,
    }
}

fn current() -> Option<std::sync::RwLockReadGuard<'static, Option<Config>>> {
    STORE.read().ok()
}

/* ------------------------------ C ABI ------------------------------- */

fn owned(text: &str) -> *mut c_char {
    match CString::new(text) {
        Ok(s) => s.into_raw(),
        Err(_) => CString::new("").unwrap().into_raw(), // interior NULs impossible in our keyspace
    }
}

#[no_mangle]
pub extern "C" fn shanti_config_load() -> c_int {
    store_load(Path::new(&shanti_paths::settings_file()))
}

#[no_mangle]
pub extern "C" fn shanti_config_load_from(path: *const c_char) -> c_int {
    // SAFETY: callers pass a NUL-terminated C string or NULL (per header).
    match unsafe { from_cstr(path) } {
        Some(p) => store_load(Path::new(&p)),
        None => store_load(Path::new(&shanti_paths::settings_file())),
    }
}

/// # Safety
/// `ptr` must be a valid NUL-terminated C string or null.
unsafe fn from_cstr(ptr: *const c_char) -> Option<String> {
    if ptr.is_null() {
        None
    } else {
        Some(
            std::ffi::CStr::from_ptr(ptr)
                .to_string_lossy()
                .into_owned(),
        )
    }
}

#[no_mangle]
pub extern "C" fn shanti_config_get_bool(key: *const c_char, dflt: c_int) -> c_int {
    let guard = current();
    let config = guard.as_ref().and_then(|g| g.as_ref());
    // SAFETY: `key` is a valid NUL-terminated C string or null, per header.
    let parsed_key = unsafe { from_cstr(key) };
    match config.zip(parsed_key) {
        Some((config, key)) => config.get_bool(&key, dflt != 0) as c_int,
        None => (dflt != 0) as c_int,
    }
}

#[no_mangle]
pub extern "C" fn shanti_config_get_uint(key: *const c_char, dflt: u32) -> u32 {
    let guard = current();
    let config = guard.as_ref().and_then(|g| g.as_ref());
    // SAFETY: `key` is a valid NUL-terminated C string or null, per header.
    let parsed_key = unsafe { from_cstr(key) };
    match config.zip(parsed_key) {
        Some((config, key)) => config.get_uint(&key, dflt),
        None => dflt,
    }
}

#[no_mangle]
pub extern "C" fn shanti_config_get_string(
    key: *const c_char,
    dflt: *const c_char,
) -> *mut c_char {
    let guard = current();
    let config = guard.as_ref().and_then(|g| g.as_ref());
    // SAFETY: NUL-terminated C strings or null, per the header contract.
    let default_text = if dflt.is_null() {
        String::new()
    } else {
        unsafe { std::ffi::CStr::from_ptr(dflt).to_string_lossy().into_owned() }
    };
    let parsed_key = unsafe { from_cstr(key) };
    let value = match config.zip(parsed_key) {
        Some((config, key)) => config.get_string(&key, &default_text),
        None => default_text.clone(),
    };
    owned(&value)
}
