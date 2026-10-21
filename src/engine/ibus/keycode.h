/*
 *  OpenBangla Keyboard
 *  Copyright (C) 2015-2020 Muhammad Mominul Huque <mominul2082@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEYCODE_H
#define KEYCODE_H

#include <stdint.h>
#include <ibus.h>
#include "riti.h"
#include "shanti-keysyms.h"

/* The keysym -> riti virtual-key table is single-sourced in the
 * shanti-keysyms crate (crates/shanti-keysyms), shared with the Fcitx5
 * engine; unknown keysyms resolve to VC_UNKNOWN over the C ABI. */
static const uint16_t VC_UNKNOWN = 0x0046;

static inline uint16_t ibus_keycode(guint k) {
  return shanti_keysyms_lookup(k);
}

#endif /* end of include guard: KEYCODE_H */
