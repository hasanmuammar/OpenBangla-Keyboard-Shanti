/*  Shanti config — C ABI of the shanti-config crate
 *  (crates/shanti-config), a read-only reader for the OpenBangla Keyboard
 *  settings store shared with the Qt GUI.
 *
 *  This header is hand-written on purpose; keep it in sync with src/lib.rs.
 *
 *  Copyright (C) 2026 Shanti contributors
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#ifndef SHANTI_CONFIG_H
#define SHANTI_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Key naming mirrors src/shared/Settings.cpp: Qt group paths with '/':
 *   "settings/FixedLayout/ShowPrevWin"
 * In the INI store the first component becomes the section and the rest are
 * joined with '\' as the key name ([settings] + FixedLayout\ShowPrevWin),
 * matching what QSettings writes.

   All functions below are safe to call after shanti_config_load(). The
   store is re-read from disk on every shanti_config_load() call, mirroring
   the QSettings::sync() round-trips the old C++ code performed. */

/* Re-read the settings store. Normal behavior when the file is missing is
 * success with defaults (the GUI creates it on first run). Returns 1 when
 * the store is usable, 0 on a read error other than "file absent". */
int shanti_config_load(void);

/* Explicit-path variant for tests/tools: load returns the same values.
 * Passing NULL behaves like shanti_config_load(). Returns 1 on success. */
int shanti_config_load_from(const char *path);

/* Typed getters; every getter uses the same defaults as the corresponding
 * Settings::getX() method. `key` must not be null. */

int shanti_config_get_bool(const char *key, int dflt);

uint32_t shanti_config_get_uint(const char *key, uint32_t dflt);

/* Returns an owned NUL-terminated UTF-8 string the caller must free with
 * shanti_string_free(). Never returns null. */
char *shanti_config_get_string(const char *key, const char *dflt);

#ifdef __cplusplus
}
#endif

#endif /* end of include guard: SHANTI_CONFIG_H */
