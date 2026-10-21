/*  Shanti paths — Rust port of src/shared/FileSystem.cpp user-directory
 *  discovery, exposed across the C ABI for the engine plugins.
 *
 *  This header is the C ABI of the shanti-paths crate (crates/shanti-paths).
 *  It is hand-written on purpose; keep it in sync with src/lib.rs.
 *
 *  Copyright (C) 2026 Shanti contributors
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#ifndef SHANTI_PATHS_H
#define SHANTI_PATHS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* All path-returning functions return an owned NUL-terminated UTF-8 string
 * that the caller must free with shanti_string_free(). They may return an
 * empty string "" when no usable base directory can be determined (mirroring
 * QStandardPaths behavior on a HOME-less session); callers must treat "" as
 * an error condition. */

/* $XDG_CONFIG_HOME (absolute) or $HOME/.config */
char *shanti_paths_config_dir(void);

/* $XDG_DATA_HOME (absolute) or $HOME/.local/share */
char *shanti_paths_data_home(void);

/* data_home + "/openbangla-keyboard" — always writable user data tree.
 * Does not create it; use shanti_paths_ensure_user_dirs() for that. */
char *shanti_paths_user_data_dir(void);

/* Engine-visible data tree: the bundled "<exe_dir>/../share/openbangla-keyboard"
 * directory when it exists, else user_data_dir(). Mirrors linuxDataDirectory(). */
char *shanti_paths_engine_data_dir(void);

/* engine_data_dir() + "/layouts" */
char *shanti_paths_layouts_dir(void);

/* config_dir + "/OpenBangla/Keyboard.conf" — the settings store shared
 * with the Qt GUI (QSettings("OpenBangla", "Keyboard")). */
char *shanti_paths_settings_file(void);

/* user_data_dir + "/icons/OpenBangla-Keyboard.png" */
char *shanti_paths_user_icon_file(void);

/* Create user_data_dir and user_data_dir/layouts if missing (best effort). */
void shanti_paths_ensure_user_dirs(void);

/* Free a string returned by any shanti_paths_* / shanti_config_* function. */
void shanti_string_free(char *path);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: SHANTI_PATHS_H */
