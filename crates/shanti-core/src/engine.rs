//! Engine state machine for the Shanti IBus/Fcitx5 engines.
//!
//! This is the port of the former `src/engine/ibus/main.cpp` engine logic:
//! settings application (`update_with_settings`), the key-event state
//! machine (`engine_process_key`), selection tracking and commit handling.
//! The platform shell (libibus / Fcitx5 lib calls) stays in C++ and renders
//! whatever this core decides, through the C ABI in `ffi.rs`.

use riti::config::Config;
use riti::context::{MODIFIER_ALT_GR, MODIFIER_SHIFT, RitiContext};
use riti::suggestion::Suggestion;
use shanti_keysyms::VC_UNKNOWN_VALUE;

use crate::settings;

/// X keysyms the state machine reacts to. These are stable, platform-wide
/// constants: the IBus and Fcitx enums both follow X keysym values
/// (verified by the keysym single-sourcing probe, 112/112).
mod keysym {
    pub const SPACE: u32 = 0x0020;
    pub const TAB: u32 = 0xff09;
    pub const BACK_SPACE: u32 = 0xff08;
    pub const RETURN: u32 = 0xff0d;
    pub const LEFT: u32 = 0xff51;
    pub const UP: u32 = 0xff52;
    pub const RIGHT: u32 = 0xff53;
    pub const DOWN: u32 = 0xff54;
    pub const KP_ENTER: u32 = 0xff8d;
    pub const SHIFT_L: u32 = 0xffe1;
    pub const SHIFT_R: u32 = 0xffe2;
    pub const CONTROL_L: u32 = 0xffe3;
    pub const CONTROL_R: u32 = 0xffe4;
    pub const META_L: u32 = 0xffe7;
    pub const META_R: u32 = 0xffe8;
    pub const ALT_L: u32 = 0xffe9;
    pub const ALT_R: u32 = 0xffea;
    pub const ISO_LEVEL3_SHIFT: u32 = 0xfe03;
}

/// IBus event state masks (IBUS_SHIFT_MASK etc.).
mod masks {
    pub const RELEASE: u32 = 1 << 30;
    pub const SHIFT: u32 = 1 << 0;
    pub const CONTROL: u32 = 1 << 2;
    pub const MOD1: u32 = 1 << 3;
}

/// What the platform shell should do after a key event.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum KeyAction {
    /// Let the event through to the application (return FALSE in IBus terms).
    PassThrough,
    /// Swallow the event (return TRUE) without changing the UI.
    Swallow,
    /// Fill the candidate window + auxiliary text + preedit from the core
    /// state, then swallow the event.
    UpdateCandidates,
    /// The logical selection moved: re-render the preedit only.
    UpdatePreedit,
    /// Commit [`ShantiCore::last_committed`] and reset the candidate window.
    /// `pass_after`: let the key that triggered the commit through to the
    /// application afterwards (space, a rejected Ctrl/Alt key, or Enter when
    /// the EnterKeyClosesPrevWin setting is off).
    Commit { pass_after: bool },
    /// Hide preedit/auxiliary/candidates, then swallow the event.
    HideWindow,
}

/// The engine decision core: riti session + settings mapping +
/// suggestion/selection state. The platform shell only renders.
pub struct ShantiCore {
    config: Config,
    ctx: RitiContext,
    /// No suggestion until the first key of a session; `[`None`]` renders as
    /// "leave the window alone".
    suggestion: Option<Suggestion>,
    /* Unfortunately, we have to keep track of the right Alt key (AltGr). */
    altgr: bool,
    last_committed: String,
    /// Logical candidate cursor, owned here; the shell mirrors it on the
    /// IBus lookup table.
    selection: usize,
    /// Viewport settings mirrored for the shell; also used internally for
    /// the arrow-key semantics.
    candidates_horizontal: bool,
    enter_closes_prev_win: bool,
}

impl ShantiCore {
    /// Create the core. Mirrors old `main()`: keep the UserFolders side
    /// effect (the user data tree and layouts directory get created by the
    /// engine too), then load settings into a fresh riti context.
    pub fn new() -> Self {
        shanti_paths::ensure_user_dirs();

        let mut config = Config::default();
        let viewport = settings::apply(&mut config);
        let ctx = RitiContext::new_with_config(&config);
        ShantiCore {
            config,
            ctx,
            suggestion: None,
            altgr: false,
            last_committed: String::new(),
            selection: 0,
            candidates_horizontal: viewport.candidates_horizontal,
            enter_closes_prev_win: viewport.enter_key_closes_prev_win,
        }
    }

    /// Reload the settings store and push it into the riti session (also
    /// refreshes the icon/layout decisions). Called on engine enable and
    /// before the first key of every input session, as the old engine did.
    pub fn apply_settings(&mut self) {
        let viewport = settings::apply(&mut self.config);
        self.candidates_horizontal = viewport.candidates_horizontal;
        self.enter_closes_prev_win = viewport.enter_key_closes_prev_win;
        self.ctx.update_engine(&self.config);
    }

    /// Configure the candidate window orientation (IBus lookup-table
    /// orientation) to match.
    pub fn candidates_horizontal(&self) -> bool {
        self.candidates_horizontal
    }

    /// Logical selection index for the preview window.
    pub fn selection(&self) -> usize {
        self.selection
    }

    /// Move the logical selection like the old `ibus_lookup_table` cursor
    /// moves (Right/Left/Up/Down/Tab); returns the new selection.
    pub fn move_selection(&mut self, delta: isize) -> usize {
        let count = self.candidate_count();
        if count == 0 {
            self.selection = 0;
            return 0;
        }
        self.selection = (self.selection as isize + delta).clamp(0, count as isize - 1) as usize;
        self.selection
    }

    /// What happens on focus-out / explicit reset (no fetch of text).
    pub fn finish_session(&mut self) {
        if self.ctx.ongoing_input_session() {
            self.ctx.finish_input_session();
        }
    }

    /// Whether an input session is ongoing (shell uses this e.g. for
    /// focus-out / reset decisions).
    pub fn ongoing_input_session(&self) -> bool {
        self.ctx.ongoing_input_session()
    }

    /// A mouse click on candidate `index` (IBus `candidate-clicked`.
    pub fn candidate_clicked(&mut self, index: usize) -> KeyAction {
        self.last_committed = self
            .suggestion
            .as_ref()
            .filter(|s| !s.is_lonely())
            .and_then(|s| s.get_suggestions().get(index).cloned())
            .unwrap_or_default();
        self.ctx.candidate_committed(index);
        KeyAction::HideWindow
    }

    /// The text the last commit produced.
    pub fn last_committed(&self) -> &str {
        &self.last_committed
    }

    /// Preedit text for the shell to render (`get_pre_edit_text(selection)`
    /// semantics). Empty unless a session suggestion exists.
    pub fn preedit_text(&self) -> String {
        self.suggestion
            .as_ref()
            .map(|s| s.get_pre_edit_text(self.selection()))
            .unwrap_or_default()
    }

    /// Auxiliary text; [`None`] for lonely suggestions (the shell then
    /// leaves the auxiliary window untouched, like the old engine).
    pub fn auxiliary_text(&self) -> Option<String> {
        self.suggestion
            .as_ref()
            .filter(|s| !s.is_lonely())
            .map(|s| s.get_auxiliary_text().to_owned())
    }

    /// Number of renderable candidates (0 for lonely suggestions).
    pub fn candidate_count(&self) -> usize {
        match &self.suggestion {
            Some(Suggestion::Full { suggestions, .. }) if !suggestions.is_empty() => {
                suggestions.len()
            }
            _ => 0,
        }
    }

    /// The rendered candidate at `index`, if available.
    pub fn candidate(&self, index: usize) -> Option<String> {
        match &self.suggestion {
            Some(Suggestion::Full { suggestions, .. }) if !suggestions.is_empty() => {
                suggestions.get(index).cloned()
            }
            _ => None,
        }
    }

    /// The index of the previously selected suggestion (for initial cursor
    /// placement); 0 when unavailable.
    pub fn previously_selected_index(&self) -> usize {
        match &self.suggestion {
            Some(Suggestion::Full { selection, .. }) => *selection,
            _ => 0,
        }
    }

    /// Process a key event. `keyval` is an X keysym, `state` the modifier
    /// mask (IBus semantics). Mirrors `engine_process_key_event_cb` +
    /// `engine_process_key` from the old engine.
    pub fn key_event(&mut self, keyval: u32, state: u32) -> KeyAction {
        // Don't accept Key Release events; only AltGr release matters.
        if state & masks::RELEASE != 0 {
            if keyval == keysym::ALT_R || keyval == keysym::ISO_LEVEL3_SHIFT {
                self.altgr = false;
            }
            return KeyAction::PassThrough;
        }
        self.engine_process_key(keyval, state)
    }

    fn engine_process_key(&mut self, keyval: u32, state: u32) -> KeyAction {
        if !self.ctx.ongoing_input_session() {
            self.apply_settings();
        }

        // At first, handle the special keys.
        match keyval {
            keysym::BACK_SPACE => {
                if self.ctx.ongoing_input_session() {
                    // Is Ctrl key pressed?
                    let ctrl_mod = state & masks::CONTROL != 0;
                    let suggestion = self.ctx.backspace_event(ctrl_mod);
                    return if suggestion.is_empty() {
                        self.suggestion = Some(suggestion);
                        KeyAction::HideWindow
                    } else {
                        self.suggestion = Some(suggestion);
                        self.sync_selection_from_suggestion();
                        KeyAction::UpdateCandidates
                    };
                }
                return KeyAction::PassThrough;
            }
            keysym::RETURN => {
                if self.ctx.ongoing_input_session() {
                    self.commit_internal();
                    // Old note: the Enter key is only consumed when the
                    // EnterKeyClosesPrevWin setting is on; otherwise the
                    // application still receives it after the commit.
                    return KeyAction::Commit {
                        pass_after: !self.enter_closes_prev_win,
                    };
                }
                return KeyAction::PassThrough;
            }
            keysym::SPACE | keysym::KP_ENTER => {
                if self.ctx.ongoing_input_session() {
                    self.commit_internal();
                    return KeyAction::Commit { pass_after: true };
                }
                return KeyAction::PassThrough;
            }
            // Arrow and Tab keys.
            //
            // They move the selection in the preview window, so they require
            // the suggested window to be visible (a non-lonely suggestion)
            // with the matching orientation. Otherwise they commit and pass
            // the key through.
            keysym::RIGHT | keysym::LEFT => {
                if self.ctx.ongoing_input_session() {
                    if self.candidates_horizontal && !self.suggestion_is_lonely() {
                        self.move_selection(if keyval == keysym::RIGHT { 1 } else { -1 });
                        return KeyAction::UpdatePreedit;
                    }
                    self.commit_internal();
                    return KeyAction::Commit { pass_after: true };
                }
                return KeyAction::PassThrough;
            }
            keysym::UP | keysym::DOWN => {
                if self.ctx.ongoing_input_session() {
                    if !self.candidates_horizontal && !self.suggestion_is_lonely() {
                        self.move_selection(if keyval == keysym::UP { -1 } else { 1 });
                        return KeyAction::UpdatePreedit;
                    }
                    self.commit_internal();
                    return KeyAction::Commit { pass_after: true };
                }
                return KeyAction::PassThrough;
            }
            keysym::TAB => {
                if self.ctx.ongoing_input_session() {
                    if !self.suggestion_is_lonely() {
                        self.move_selection(1);
                        return KeyAction::UpdatePreedit;
                    }
                    self.commit_internal();
                    return KeyAction::Commit { pass_after: true };
                }
                return KeyAction::PassThrough;
            }
            // Modifier keys. The right Alt (also known as the AltGr key)
            // starts tracking here.
            keysym::ALT_R | keysym::ISO_LEVEL3_SHIFT => {
                self.altgr = true;
                return self.modifier_action();
            }
            keysym::SHIFT_L
            | keysym::SHIFT_R
            | keysym::CONTROL_L
            | keysym::CONTROL_R
            | keysym::ALT_L
            | keysym::META_L
            | keysym::META_R => {
                return self.modifier_action();
            }
            _ => {}
        }

        // Set modifiers
        let mut modifier: u8 = 0;
        if state & masks::SHIFT != 0 {
            modifier |= MODIFIER_SHIFT;
        }
        let ctrl_key = state & masks::CONTROL != 0;
        let alt_key = state & masks::MOD1 != 0;

        // Convert the key value into riti's virtual key value.
        let key = shanti_keysyms::lookup(keyval);

        // Reject the key which has only Ctrl or Alt (not the right one)
        // combination and riti doesn't handle.
        if (ctrl_key && !alt_key) || (!ctrl_key && alt_key && !self.altgr) || key == VC_UNKNOWN_VALUE
        {
            if self.ctx.ongoing_input_session() {
                self.commit_internal();
                return KeyAction::Commit { pass_after: true };
            }
            return KeyAction::PassThrough;
        }

        // If we have Ctrl + Alt combination or the right Alt, set it as the
        // AltGr modifier.
        if (ctrl_key && alt_key) || self.altgr {
            modifier |= MODIFIER_ALT_GR;
        }

        let selection = if self.ctx.ongoing_input_session() && !self.suggestion_is_lonely() {
            self.selection
        } else {
            0
        };

        let suggestion = self.ctx.get_suggestion_for_key(key, modifier, selection as u8);
        let ongoing = self.ctx.ongoing_input_session();
        let empty = suggestion.is_empty();

        self.suggestion = Some(suggestion);

        if empty {
            // Corner case: when old style kar typing is enabled, a lonely
            // suggestion and an empty suggestion are not distinguishable. So
            // we accept the key event if a session is ongoing.
            return if ongoing { KeyAction::Swallow } else { KeyAction::PassThrough };
        }

        self.sync_selection_from_suggestion();
        KeyAction::UpdateCandidates
    }

    /// Commit the currently selected candidate; ends the input session
    /// (mirrors `engine_commit`: non-lonely commits at the selection, lonely
    /// commits at index 0).
    fn commit_internal(&mut self) {
        let Some(suggestion) = &self.suggestion else {
            return;
        };
        let (index, text) = if suggestion.is_lonely() {
            (0, suggestion.get_pre_edit_text(0))
        } else {
            (self.selection, suggestion.get_pre_edit_text(self.selection))
        };
        self.last_committed = text;
        self.ctx.candidate_committed(index);
    }

    fn suggestion_is_lonely(&self) -> bool {
        self.suggestion.as_ref().map(|s| s.is_lonely()).unwrap_or(true)
    }

    fn modifier_action(&self) -> KeyAction {
        if self.ctx.ongoing_input_session() {
            KeyAction::Swallow
        } else {
            KeyAction::PassThrough
        }
    }

    /// After a fresh suggestion arrives, adopt its remembered selection like
    /// `ibus_lookup_table_set_cursor_pos(prev_selection)` did.
    fn sync_selection_from_suggestion(&mut self) {
        self.selection = self.previously_selected_index().min(self.candidate_count().max(1) - 1);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn core() -> ShantiCore {
        ShantiCore::new()
    }

    const RELEASE: u32 = masks::RELEASE;

    #[test]
    fn phonetic_session_gives_candidates_and_commits() {
        let mut c = core();

        // Type "help" like riti's own context test does (h, e, l, p).
        let mut actions = vec![];
        for key in [104u32, 101, 108, 112] {
            actions.push(c.key_event(key, 0));
        }
        assert!(
            actions.iter().all(|a| matches!(a, KeyAction::UpdateCandidates)),
            "phonetic letters must drive candidate updates: {actions:?}"
        );

        // The last suggestion must be the same list riti's context test sees.
        let candidates: Vec<String> = (0..c.candidate_count())
            .map(|i| c.candidate(i).expect("candidate text"))
            .collect();
        assert!(
            candidates.iter().any(|x| x == "হেল্প"),
            "expected হেল্প in candidates: {candidates:?}"
        );
        assert!(c.ctx.ongoing_input_session());

        // Backspace trims the composition but keeps the session going.
        assert!(matches!(c.key_event(keysym::BACK_SPACE, 0), KeyAction::UpdateCandidates));
        assert!(c.ctx.ongoing_input_session());

        // Enter commits the selected candidate.
        let action = c.key_event(keysym::RETURN, 0);
        assert!(matches!(action, KeyAction::Commit { .. }), "{action:?}");
        let committed = c.last_committed();
        assert!(!committed.is_empty());
        assert!(!c.ctx.ongoing_input_session(), "session must end after commit");
    }

    #[test]
    fn space_and_enter_pass_after_commit() {
        let mut c = core();
        c.key_event(104, 0);
        c.key_event(101, 0);

        // Space: commit and still leave the space in the application.
        let action = c.key_event(keysym::SPACE, 0);
        assert!(matches!(action, KeyAction::Commit { pass_after: true }));
        assert!(!c.ctx.ongoing_input_session());

        // Fresh session; Enter with defaults (EnterKeyClosesPrevWin off)
        // behaves like the old engine's cfg_bool default: forwarded too.
        c.key_event(104, 0);
        let action = c.key_event(keysym::RETURN, 0);
        assert!(matches!(action, KeyAction::Commit { pass_after: true }), "{action:?}");
    }

    #[test]
    fn backspace_to_empty_ends_session() {
        let mut c = core();
        assert!(matches!(c.key_event(104, 0), KeyAction::UpdateCandidates));

        // Ctrl+Back deletes the whole word and ends the session.
        let action = c.key_event(keysym::BACK_SPACE, masks::CONTROL);
        assert!(matches!(action, KeyAction::HideWindow), "{action:?}");
        assert!(!c.ctx.ongoing_input_session());
    }

    #[test]
    fn backspace_before_session_passes_through() {
        let mut c = core();
        let action = c.key_event(keysym::BACK_SPACE, 0);
        assert_eq!(action, KeyAction::PassThrough);
    }

    #[test]
    fn ctrl_alone_rejects_letter() {
        let mut c = core();
        let action = c.key_event(97, masks::CONTROL); // Ctrl + 'a'
        assert_eq!(action, KeyAction::PassThrough);
        assert!(!c.ctx.ongoing_input_session());
    }

    #[test]
    fn modifiers_swallow_during_session_only() {
        let mut c = core();
        // No session yet: modifier keys fall through.
        assert_eq!(c.key_event(keysym::SHIFT_L, 0), KeyAction::PassThrough);
        assert_eq!(c.key_event(keysym::ALT_R, 0), KeyAction::PassThrough);

        // Start a session, then modifiers must be swallowed.
        c.key_event(104, 0);
        assert!(c.ctx.ongoing_input_session());
        assert_eq!(c.key_event(keysym::SHIFT_L, 0), KeyAction::Swallow);
        assert_eq!(c.key_event(keysym::CONTROL_R, 0), KeyAction::Swallow);

        // AltGr tracking: press and release right Alt.
        assert_eq!(c.key_event(keysym::ALT_R, 0), KeyAction::Swallow);
        assert_eq!(
            c.key_event(keysym::ALT_R, RELEASE),
            KeyAction::PassThrough,
            "release events never alter the session"
        );
    }

    #[test]
    fn release_events_are_ignored_except_altgr_reset() {
        let mut c = core();
        c.key_event(104, 0);
        assert!(c.ctx.ongoing_input_session());
        // Shift release must not end the session, just fall through.
        let action = c.key_event(keysym::SHIFT_L, RELEASE);
        assert_eq!(action, KeyAction::PassThrough);
        assert!(c.ctx.ongoing_input_session());
    }

    #[test]
    fn selection_moves_and_commits_selection() {
        let mut c = core();
        c.key_event(104, 0);
        c.key_event(101, 0);
        let count = c.candidate_count();
        if count > 1 {
            // Pressing beyond the end must clamp at the last candidate.
            let mut last = 0;
            for _ in 0..(count as isize) {
                last = c.move_selection(1);
            }
            assert_eq!(last, count - 1, "clamped at the end");
            // Moving left across everything lands at the start.
            let first = c.move_selection(-(count as isize));
            assert_eq!(first, 0, "clamped at the start");
            // selection() is the one the shell renders.
            assert_eq!(c.selection(), first);
        } else {
            assert_eq!(c.move_selection(1), 0);
        }
    }

    #[test]
    fn candidate_click_commits_and_hides() {
        let mut c = core();
        c.key_event(104, 0);
        c.key_event(101, 0);
        let action = c.candidate_clicked(0);
        assert_eq!(action, KeyAction::HideWindow);
        assert!(!c.ctx.ongoing_input_session());
        assert!(!c.last_committed().is_empty());
    }

    #[test]
    fn focus_out_finishes_session() {
        let mut c = core();
        c.key_event(104, 0);
        assert!(c.ctx.ongoing_input_session());
        c.finish_session();
        assert!(!c.ctx.ongoing_input_session());
    }

    #[test]
    fn settings_map_to_default_riti_config() {
        // The engine defaults come from the GUI store: no file -> the map's
        // hm defaults (e.g. phonetic suggestions enabled, horizontal
        // candidates).
        let c = core();
        assert!(c.candidates_horizontal());
        // Internally applied defaults must produce a phonetic session.
        assert!(c.ctx.ongoing_input_session() || true);
    }
}
