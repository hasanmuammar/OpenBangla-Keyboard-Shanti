//! C ABI consumed by the C++ IM platform shells (libibus / Fcitx5). Follows
//! the shanti-config convention: owned `char*` results freed with
//! [`shanti_core_string_free`], opaque handles, hand-written header
//! (`include/shanti-core.h`).

use std::ffi::{CString, c_char, c_int};

use crate::engine::{KeyAction, ShantiCore};

const ACTION_PASS_THROUGH: c_int = 0;
const ACTION_SWALLOW: c_int = 1;
const ACTION_UPDATE_CANDIDATES: c_int = 2;
const ACTION_UPDATE_PREEDIT: c_int = 3;
const ACTION_COMMIT: c_int = 4;
const ACTION_HIDE_WINDOW: c_int = 5;
/// Commit action with the triggering key forwarded afterwards (space, or
/// Enter when the EnterKeyClosesPrevWin setting is off).
const ACTION_COMMIT_PASS: c_int = 6;

fn encode_action(action: KeyAction) -> c_int {
    match action {
        KeyAction::PassThrough => ACTION_PASS_THROUGH,
        KeyAction::Swallow => ACTION_SWALLOW,
        KeyAction::UpdateCandidates => ACTION_UPDATE_CANDIDATES,
        KeyAction::UpdatePreedit => ACTION_UPDATE_PREEDIT,
        KeyAction::Commit { pass_after } => {
            if pass_after {
                ACTION_COMMIT_PASS
            } else {
                ACTION_COMMIT
            }
        }
        KeyAction::HideWindow => ACTION_HIDE_WINDOW,
    }
}

fn owned(text: &str) -> *mut c_char {
    match CString::new(text) {
        Ok(s) => s.into_raw(),
        // Interior NULs cannot occur in our text; fall back to empty.
        Err(_) => CString::default().into_raw(),
    }
}

/// # Safety
/// `ptr` must be null or a valid [`shanti_core_new`] pointer.
unsafe fn borrow<'a>(ptr: *mut ShantiCore) -> &'a mut ShantiCore {
    assert!(!ptr.is_null());
    &mut *ptr
}

#[no_mangle]
pub extern "C" fn shanti_core_new() -> *mut ShantiCore {
    Box::into_raw(Box::new(ShantiCore::new()))
}

/// # Safety
/// `core` must be a pointer returned by [`shanti_core_new`], not yet freed,
/// and must not be used after this call.
#[no_mangle]
pub unsafe extern "C" fn shanti_core_free(core: *mut ShantiCore) {
    if !core.is_null() {
        drop(Box::from_raw(core));
    }
}

#[no_mangle]
pub unsafe extern "C" fn shanti_core_apply_settings(core: *mut ShantiCore) {
    borrow(core).apply_settings();
}

#[no_mangle]
pub unsafe extern "C" fn shanti_core_candidates_horizontal(core: *mut ShantiCore) -> c_int {
    borrow(core).candidates_horizontal() as c_int
}

#[no_mangle]
pub unsafe extern "C" fn shanti_core_ongoing_input_session(core: *mut ShantiCore) -> c_int {
    borrow(core).ongoing_input_session() as c_int
}

#[no_mangle]
pub unsafe extern "C" fn shanti_core_finish_session(core: *mut ShantiCore) {
    borrow(core).finish_session();
}

/// Process a key event; returns one of the `SHANTI_KEY_*` action codes.
#[no_mangle]
pub unsafe extern "C" fn shanti_core_key_event(core: *mut ShantiCore, keyval: u32, state: u32) -> c_int {
    encode_action(borrow(core).key_event(keyval, state))
}

#[no_mangle]
pub unsafe extern "C" fn shanti_core_selection(core: *mut ShantiCore) -> c_int {
    borrow(core).selection() as c_int
}

/// Move the logical selection (`delta` = +1/-1); returns the new selection.
#[no_mangle]
pub unsafe extern "C" fn shanti_core_move_selection(core: *mut ShantiCore, delta: c_int) -> c_int {
    borrow(core).move_selection(delta as isize) as c_int
}

#[no_mangle]
pub unsafe extern "C" fn shanti_core_candidate_count(core: *mut ShantiCore) -> c_int {
    borrow(core).candidate_count() as c_int
}

/// # Safety
/// Result must be freed with [`shanti_core_string_free`].
#[no_mangle]
pub unsafe extern "C" fn shanti_core_candidate(core: *mut ShantiCore, index: c_int) -> *mut c_char {
    match borrow(core).candidate(index.max(0) as usize) {
        Some(text) => owned(&text),
        None => owned(""),
    }
}

/// The index of the previously-selected candidate (cursor restoration).
#[no_mangle]
pub unsafe extern "C" fn shanti_core_previously_selected_index(core: *mut ShantiCore) -> c_int {
    borrow(core).previously_selected_index() as c_int
}

/// Non-zero when an auxiliary text is present; then
/// [`shanti_core_auxiliary_text`] is meaningful.
#[no_mangle]
pub unsafe extern "C" fn shanti_core_has_auxiliary(core: *mut ShantiCore) -> c_int {
    borrow(core).auxiliary_text().is_some() as c_int
}

/// # Safety
/// Result must be freed with [`shanti_core_string_free`]; empty string when
/// the auxiliary window should stay untouched.
#[no_mangle]
pub unsafe extern "C" fn shanti_core_auxiliary_text(core: *mut ShantiCore) -> *mut c_char {
    owned(&borrow(core).auxiliary_text().unwrap_or_default())
}

/// # Safety
/// Result must be freed with [`shanti_core_string_free`].
#[no_mangle]
pub unsafe extern "C" fn shanti_core_preedit_text(core: *mut ShantiCore) -> *mut c_char {
    owned(&borrow(core).preedit_text())
}

/// # Safety
/// Result must be freed with [`shanti_core_string_free`].
#[no_mangle]
pub unsafe extern "C" fn shanti_core_last_committed_text(core: *mut ShantiCore) -> *mut c_char {
    owned(borrow(core).last_committed())
}

/// A candidate was clicked in the preview window; returns the action code
/// (hide window + swallow). The committed text is readable through
/// [`shanti_core_last_committed_text`].
#[no_mangle]
pub unsafe extern "C" fn shanti_core_candidate_clicked(core: *mut ShantiCore, index: c_int) -> c_int {
    encode_action(borrow(core).candidate_clicked(index.max(0) as usize))
}

/// # Safety
/// `text` must have been returned from this crate.
#[no_mangle]
pub unsafe extern "C" fn shanti_core_string_free(text: *mut c_char) {
    if !text.is_null() {
        drop(CString::from_raw(text));
    }
}
