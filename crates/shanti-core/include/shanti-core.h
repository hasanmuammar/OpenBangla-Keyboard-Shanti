/*  Shanti engine core — C ABI of the shanti-core crate
 *  (crates/shanti-core): the engine state machine shared by the IBus and
 *  Fcitx5 engine shells. This header is hand-written on purpose; keep it in
 *  sync with src/ffi.rs.
 *
 *  Copyright (C) 2026 Shanti contributors
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#ifndef SHANTI_CORE_H
#define SHANTI_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque core handle. All functions below take a `ShantiCore *` returned by
 * shanti_core_new(); they must be called from the IM platform's main loop
 * thread (single-threaded usage). */

typedef struct ShantiCore ShantiCore;

/* Key-event action codes — what the platform shell should do so the
 * rendering matches the former engine logic (engine_process_key). */
#define SHANTI_KEY_PASS_THROUGH    0 /* let the key reach the application */
#define SHANTI_KEY_SWALLOW         1 /* accept the key, no UI change */
#define SHANTI_KEY_UPDATE_CANDIDATES 2 /* (re)fill candidates + aux + preedit */
#define SHANTI_KEY_UPDATE_PREEDIT  3 /* selection moved: preedit only */
#define SHANTI_KEY_COMMIT          4 /* commit & reset, swallow the key */
#define SHANTI_KEY_HIDE_WINDOW     5 /* hide preedit/aux/candidates */
#define SHANTI_KEY_COMMIT_PASS     6 /* commit & reset, forward the key */

/* Lifecycle */
ShantiCore *shanti_core_new(void);
void shanti_core_free(ShantiCore *core);

/* Apply the settings store into the riti session (old
 * update_with_settings() + riti_context_update_engine()). */
void shanti_core_apply_settings(ShantiCore *core);

/* Non-zero when the candidate window should be horizontal. */
int shanti_core_candidates_horizontal(ShantiCore *core);

/* Key event handling. `keyval` is an X keysym, `state` the IBus modifier
 * mask (SHIFT/CONTROL/MOD1, release mask). Returns one of the
 * SHANTI_KEY_* codes. Handles AltGr tracking, release filtering, the
 * On-going-session bookkeeping and the commit decisions like the old
 * engine did. */
int shanti_core_key_event(ShantiCore *core, uint32_t keyval, uint32_t state);

/* One-shot: after SHANTI_KEY_COMMIT, non-zero if the key that triggered
 * the commit should still reach the application (space, or Enter when the
 * EnterKeyClosesPrevWin setting is off). Read right after the action;
 * the next key event resets it. */
int shanti_core_commit_pass_after(void);

/* Non-zero while a riti input session is ongoing. */
int shanti_core_ongoing_input_session(ShantiCore *core);

/* Focus-out / reset helper: finish an ongoing session if any. Deciding
 * whether to reset the rendering is the shell's job (see old
 * engine_focus_out_cb / engine_reset_cb). */
void shanti_core_finish_session(ShantiCore *core);

/* Logical selection for the preview window (the old engine read it from
 * the IBus lookup-table cursor; the shell mirrors this value onto the
 * table). */
int shanti_core_selection(ShantiCore *core);
/* Move the logical selection (+1 / -1) like the old table cursor did;
 * returns the new selection. */
int shanti_core_move_selection(ShantiCore *core, int delta);

/* Preview-window state queries; render with the engine types the shell
 * knows (IBusText etc.). */
int shanti_core_candidate_count(ShantiCore *core);
/* Non-zero when an auxiliary text is available (not a lonely suggestion);
 * then shanti_core_auxiliary_text() is meaningful and the shell should
 * update the auxiliary window. */
int shanti_core_has_auxiliary(ShantiCore *core);
/* Previously-selected candidate index, for context; also reflected in
 * shanti_core_selection() right after a fresh suggestion. */
int shanti_core_previously_selected_index(ShantiCore *core);
/* Result is owned; free with shanti_core_string_free(). Empty when absent. */
char *shanti_core_candidate(ShantiCore *core, int index);
char *shanti_core_auxiliary_text(ShantiCore *core); /* owned; empty if none */
char *shanti_core_preedit_text(ShantiCore *core);   /* owned */
char *shanti_core_last_committed_text(ShantiCore *core); /* owned */

/* A candidate was clicked in the window. Returns the action code
 * (SHANTI_KEY_HIDE_WINDOW) and leaves the committed text in
 * shanti_core_last_committed_text(). */
int shanti_core_candidate_clicked(ShantiCore *core, int index);

/* Free strings returned by this crate. */
void shanti_core_string_free(char *text);

#ifdef __cplusplus
}
#endif

#endif /* SHANTI_CORE_H */
