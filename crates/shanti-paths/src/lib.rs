/*
 *  Shanti paths — Rust port of src/shared/FileSystem.cpp (Linux contract).
 *
 *  For Linux we follow the XDG Base Directory Specification for storing
 *  user specific data, mirroring the QSettings/QStandardPaths behavior the
 *  Qt GUI (src/shared) relies on:
 *
 *   - settings store:  $XDG_CONFIG_HOME/OpenBangla/Keyboard.conf
 *     (QSettings("OpenBangla", "Keyboard"); verified against Qt 5.15)
 *   - user data tree:  $XDG_DATA_HOME/openbangla-keyboard
 *     (QStandardPaths::GenericDataLocation path used by the GUI and engines)
 *   - bundled data:    "<exe_dir>/../share/openbangla-keyboard" when present
 *     (used for the portable/DESTDIR install layouts)
 */

use std::ffi::{c_char, CString};
use std::path::{Path, PathBuf};

/// XDG base-dir resolution shared by both config and data roots.
/// `env_value` is the raw $XDG_*_HOME value (if set); an unset, empty, or
/// non-absolute value falls back to `$HOME/<home_sub>` per the XDG spec.
/// Returns an empty PathBuf when neither is usable.
/// Non-UTF-8 path bytes are replaced (lossy) — an acceptable deviation:
/// the C++ code this replaces went through UTF-16 es-capes via QString too.
fn base_dir(env_value: Option<&str>, home: Option<&str>, home_sub: &str) -> PathBuf {
    if let Some(value) = env_value {
        if !value.is_empty() && Path::new(value).is_absolute() {
            return PathBuf::from(value);
        }
    }
    match home {
        Some(h) if !h.is_empty() => PathBuf::from(h).join(home_sub),
        _ => PathBuf::new(),
    }
}

fn env_value(key: &str) -> Option<String> {
    std::env::var_os(key).map(|v| v.to_string_lossy().into_owned())
}

pub fn config_dir() -> PathBuf {
    base_dir(env_value("XDG_CONFIG_HOME").as_deref(), env_value("HOME").as_deref(), ".config")
}

pub fn data_home() -> PathBuf {
    base_dir(env_value("XDG_DATA_HOME").as_deref(), env_value("HOME").as_deref(), ".local/share")
}

/// QStandardPaths::writableLocation(GenericDataLocation)-based user tree.
pub fn user_data_dir() -> PathBuf {
    data_home().join("openbangla-keyboard")
}

/// "<exe_dir>/../share/openbangla-keyboard" used by portable/DESTDIR installs.
/// Resolution mirrors linuxDataDirectory() in FileSystem.cpp.
pub fn bundled_data_dir() -> Option<PathBuf> {
    let exe = std::fs::read_link("/proc/self/exe").ok()?;
    let candidate = exe.parent()?.parent()?.join("share").join("openbangla-keyboard");
    if candidate.is_dir() {
        Some(candidate)
    } else {
        None
    }
}

/// Engine-visible data tree: bundled directory when present, user tree otherwise.
pub fn engine_data_dir() -> PathBuf {
    bundled_data_dir().unwrap_or_else(user_data_dir)
}

pub fn layouts_dir() -> PathBuf {
    engine_data_dir().join("layouts")
}

/// The settings store file shared with the Qt GUI.
pub fn settings_file() -> PathBuf {
    config_dir().join("OpenBangla").join("Keyboard.conf")
}

pub fn user_icon_file() -> PathBuf {
    user_data_dir().join("icons").join("OpenBangla-Keyboard.png")
}

/// Create the user data directories the GUI used to create via UserFolders
/// (best-effort; errors are ignored, readers handle missing directories).
pub fn ensure_user_dirs() {
    let _ = std::fs::create_dir_all(user_data_dir().join("layouts"));
}

/* ----------------------------- C ABI ------------------------------ */

fn owned(path: &Path) -> *mut c_char {
    // Interior NULs cannot appear in real paths; empty string is the error value.
    match CString::new(path.to_string_lossy().as_bytes()) {
        Ok(s) => s.into_raw(),
        Err(_) => CString::new("").unwrap().into_raw(),
    }
}

#[no_mangle]
pub extern "C" fn shanti_paths_config_dir() -> *mut c_char {
    owned(&config_dir())
}

#[no_mangle]
pub extern "C" fn shanti_paths_data_home() -> *mut c_char {
    owned(&data_home())
}

#[no_mangle]
pub extern "C" fn shanti_paths_user_data_dir() -> *mut c_char {
    owned(&user_data_dir())
}

#[no_mangle]
pub extern "C" fn shanti_paths_engine_data_dir() -> *mut c_char {
    owned(&engine_data_dir())
}

#[no_mangle]
pub extern "C" fn shanti_paths_layouts_dir() -> *mut c_char {
    owned(&layouts_dir())
}

#[no_mangle]
pub extern "C" fn shanti_paths_settings_file() -> *mut c_char {
    owned(&settings_file())
}

#[no_mangle]
pub extern "C" fn shanti_paths_user_icon_file() -> *mut c_char {
    owned(&user_icon_file())
}

#[no_mangle]
pub extern "C" fn shanti_paths_ensure_user_dirs() {
    ensure_user_dirs()
}

#[no_mangle]
pub extern "C" fn shanti_string_free(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe { drop(std::ffi::CString::from_raw(ptr)) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn xdg_env_absolute_is_used() {
        let dir = base_dir(Some("/custom/config"), None, ".config");
        assert_eq!(dir, PathBuf::from("/custom/config"));
    }

    #[test]
    fn xdg_env_empty_or_relative_falls_back_to_home() {
        assert_eq!(
            base_dir(Some(""), Some("/home/u"), ".config"),
            PathBuf::from("/home/u/.config")
        );
        assert_eq!(
            base_dir(Some("relative/path"), Some("/home/u"), ".local/share"),
            PathBuf::from("/home/u/.local/share")
        );
    }

    #[test]
    fn unset_env_falls_back_to_home() {
        assert_eq!(
            base_dir(None, Some("/home/u"), ".local/share"),
            PathBuf::from("/home/u/.local/share")
        );
    }

    #[test]
    fn homeless_environment_yields_empty() {
        assert_eq!(base_dir(None, None, ".config"), PathBuf::new());
    }

    #[test]
    fn settings_file_follows_org_app_layout() {
        assert_eq!(
            settings_file(),
            config_dir().join("OpenBangla").join("Keyboard.conf")
        );
    }
}
