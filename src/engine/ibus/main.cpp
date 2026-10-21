/*
 *  OpenBangla Keyboard
 *  Copyright (C) 2015-2018 Muhammad Mominul Huque <mominul2082@gmail.com>
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

/*  IBus engine for OpenBangla Keyboard.
 *
 *  Qt-free since rewrite phase 1 (docs/REWRITE-PLAN.md): configuration is
 *  read through the shanti-config crate (a read-only QSettings-store reader)
 *  and path discovery through shanti-paths, mirroring the contracts of the
 *  former src/shared Settings/UserFolders code the Qt GUI still uses. */

#include <ibus.h>
#include <cstring>
#include <filesystem>
#include <string>
#include "riti.h"
#include "keycode.h"
#include "shanti-config.h"
#include "shanti-paths.h"
#include "Log.h"

static RitiContext *ctx = nullptr;
static Config *config = nullptr;
static IBusBus *bus = nullptr;
static IBusEngine *engine = nullptr;
static IBusLookupTable *table = nullptr;
static gint id = 0;
static Suggestion *suggestion = nullptr;
/* Unfortunately, we have to keep track of the right Alt Key. */
static bool altGr = false;

/* shanti-config boolean getter with Settings.cpp default semantics. */
static bool cfg_bool(const char *key, bool dflt) {
  return shanti_config_get_bool(key, dflt ? 1 : 0) != 0;
}

/* The host GUI can save an absolute path to its layout file. When the
 * engine runs in Flatpak, that host path may not exist inside the sandbox.
 * Resolve bundled layouts by filename against the engine's own data tree. */
static std::string resolve_layout_path(const std::string &configuredPath) {
  namespace fs = std::filesystem;
  try {
    if (!fs::exists(configuredPath) && fs::path(configuredPath).is_absolute()) {
      char *dataDirC = shanti_paths_engine_data_dir();
      fs::path dataDir = std::string(dataDirC ? dataDirC : "");
      shanti_string_free(dataDirC);
      fs::path bundled = dataDir / "layouts" / fs::path(configuredPath).filename();
      if (fs::is_regular_file(bundled)) {
        LOG_INFO("[IM:iBus]: Resolving inaccessible layout path to bundled file: %s\n",
                 bundled.string().c_str());
        return bundled.string();
      }
    }
  } catch (...) {
    // Keep the configured path on any filesystem error.
  }
  return configuredPath;
}

std::string engine_data_dir() {
  char *dataDirC = shanti_paths_engine_data_dir();
  std::string dataDir(dataDirC ? dataDirC : "");
  shanti_string_free(dataDirC);
  return dataDir;
}

void update_with_settings() {
  shanti_config_load();

  char *layoutC = shanti_config_get_string("layout/path", "avro_phonetic");
  std::string layoutPath = resolve_layout_path(layoutC ? layoutC : "");
  shanti_string_free(layoutC);

  if (!riti_config_set_layout_file(config, layoutPath.c_str())) {
    LOG_ERROR("[IM:iBus]: Failed to set layout file: %s\n", layoutPath.c_str());
  }

  riti_config_set_suggestion_include_english(config, cfg_bool("settings/PreviewWin/IncludeEnglish", true));
  riti_config_set_phonetic_suggestion(config, cfg_bool("settings/CandidateWin/Phonetic", true));

  const std::string databaseDir = engine_data_dir() + "/data";
  if (!riti_config_set_database_dir(config, databaseDir.c_str())) {
    LOG_ERROR("[IM:iBus]: Failed to set database directory: %s\n", databaseDir.c_str());
  }

  riti_config_set_fixed_suggestion(config, cfg_bool("settings/FixedLayout/ShowPrevWin", true));
  riti_config_set_fixed_auto_vowel(config, cfg_bool("settings/FixedLayout/AutoVowelForm", true));
  riti_config_set_fixed_auto_chandra(config, cfg_bool("settings/FixedLayout/AutoChandraPos", true));
  riti_config_set_fixed_traditional_kar(config, cfg_bool("settings/FixedLayout/TraditionalKar", false));
  riti_config_set_fixed_old_kar_order(config, cfg_bool("settings/FixedLayout/OldKarOrder", false));
  riti_config_set_fixed_old_reph(config, cfg_bool("settings/FixedLayout/OldReph", true));
  riti_config_set_fixed_numpad(config, cfg_bool("settings/FixedLayout/NumberPad", true));
  riti_config_set_ansi_encoding(config, cfg_bool("settings/ANSI", false));
  riti_config_set_smart_quote(config, cfg_bool("settings/SmartQuoting", true));

  if (table != nullptr) {
    ibus_lookup_table_set_orientation(
        table,
        cfg_bool("settings/CandidateWin/Horizontal", true) ? IBUS_ORIENTATION_HORIZONTAL
                                                           : IBUS_ORIENTATION_VERTICAL);
  }
}

void engine_update_preedit() {
  char *txt = nullptr;

  if (!riti_suggestion_is_lonely(suggestion)) {
    ibus_engine_update_lookup_table_fast(engine, table, TRUE);
    guint index = ibus_lookup_table_get_cursor_pos(table);
    txt = riti_suggestion_get_pre_edit_text(suggestion, index);
  } else {
    txt = riti_suggestion_get_pre_edit_text(suggestion, 0);
  }

  IBusText *text = ibus_text_new_from_string(txt);
  riti_string_free(txt);

  ibus_engine_update_preedit_text_with_mode(engine, text, ibus_text_get_length(text), TRUE, IBUS_ENGINE_PREEDIT_COMMIT);
}

void engine_update_lookup_table() {
  if (!riti_suggestion_is_lonely(suggestion)) {
    char *aux = riti_suggestion_get_auxiliary_text(suggestion);
    IBusText *auxiliary = ibus_text_new_from_string(aux);
    riti_string_free(aux);

    ibus_lookup_table_clear(table);
    ibus_engine_update_auxiliary_text(engine, auxiliary, TRUE);

    uintptr_t len = riti_suggestion_get_length(suggestion);

    for (uintptr_t i = 0; i < len; i++) {
      char *txt = riti_suggestion_get_suggestion(suggestion, i);
      IBusText *text = ibus_text_new_from_string(txt);
      ibus_lookup_table_append_candidate(table, text);
      riti_string_free(txt);
    }

    uintptr_t prev_selection = riti_suggestion_previously_selected_index(suggestion);

    ibus_lookup_table_set_cursor_pos(table, prev_selection);
  }

  engine_update_preedit();
}

void engine_reset() {
  ibus_lookup_table_clear(table);
  ibus_engine_hide_preedit_text(engine);
  ibus_engine_hide_auxiliary_text(engine);
  ibus_engine_hide_lookup_table(engine);
}

void engine_commit_text(IBusText *text) {
  ibus_engine_commit_text(engine, text);
  engine_reset();
}

void engine_commit() {
  uintptr_t index = 0;
  char *text = nullptr;

  if (!riti_suggestion_is_lonely(suggestion)) {
    index = ibus_lookup_table_get_cursor_pos(table);
    text = riti_suggestion_get_pre_edit_text(suggestion, index);
  } else {
    index = 0;
    text = riti_suggestion_get_pre_edit_text(suggestion, 0);
  }

  engine_commit_text(ibus_text_new_from_string(text));
  riti_string_free(text);
  riti_context_candidate_committed(ctx, index);
}

gboolean engine_process_key(guint keyval, guint keycode, guint state) {
  u_int8_t modifier = 0;
  bool ctrl_key = false;
  bool alt_key = false;

  if (!riti_context_ongoing_input_session(ctx)) {
    update_with_settings();
    riti_context_update_engine(ctx, config);
  }

  // At first, handle the special keys.
  switch (keyval) {
    case IBUS_KEY_BackSpace:
      if (riti_context_ongoing_input_session(ctx)) {
        // Is Ctrl key is pressed?
        bool ctrl_mod = state & IBUS_CONTROL_MASK;
        suggestion = riti_context_backspace_event(ctx, ctrl_mod);

        if (!riti_suggestion_is_empty(suggestion)) {
          engine_update_lookup_table();
        } else {
          engine_reset();
        }

        return TRUE;
      } else {
        return FALSE;
      }
    case IBUS_KEY_Return:
      if (riti_context_ongoing_input_session(ctx)) {
        engine_commit();
        return (gboolean)cfg_bool("settings/EnterKeyClosesPrevWin", false);
      } else {
        return FALSE;
      }
    case IBUS_KEY_space:
    case IBUS_KEY_KP_Enter:
      if (riti_context_ongoing_input_session(ctx)) {
        engine_commit();
      }
      return FALSE;
    /** Arrow and Tab keys.
     * We use the arrow keys and the tab key to move the selection
     * in the preview window. So we have to ensure the preview
     * window is shown by checking if the current suggestion is
     * not a lonely one. Otherwise we don't handle it.
     **/
    case IBUS_KEY_Right:
    case IBUS_KEY_Left:
      if (riti_context_ongoing_input_session(ctx)) {
        if (cfg_bool("settings/CandidateWin/Horizontal", true) && !riti_suggestion_is_lonely(suggestion)) {
          if (keyval == IBUS_KEY_Right) {
            ibus_lookup_table_cursor_down(table);
          } else {
            ibus_lookup_table_cursor_up(table);
          }
          engine_update_preedit();
          return TRUE;
        } else {
          engine_commit();
          return FALSE;
        }
      } else {
        return FALSE;
      }
    case IBUS_KEY_Up:
    case IBUS_KEY_Down:
      if (riti_context_ongoing_input_session(ctx)) {
        if (!cfg_bool("settings/CandidateWin/Horizontal", true) && !riti_suggestion_is_lonely(suggestion)) {
          if (keyval == IBUS_KEY_Up) {
            ibus_lookup_table_cursor_up(table);
          } else {
            ibus_lookup_table_cursor_down(table);
          }
          engine_update_preedit();
          return TRUE;
        } else {
          engine_commit();
          return FALSE;
        }
      } else {
        return FALSE;
      }
    case IBUS_KEY_Tab:
      if (riti_context_ongoing_input_session(ctx)) {
        if (!riti_suggestion_is_lonely(suggestion)) {
          ibus_lookup_table_cursor_down(table);
          engine_update_preedit();
          return TRUE;
        } else {
          engine_commit();
          return FALSE;
        }
      } else {
        return FALSE;
      }
    /** Modifier keys **/
    case IBUS_KEY_Alt_R:
    case IBUS_KEY_ISO_Level3_Shift:
    altGr = true; // Keep track of the right Alt key (also known as the AltGr key)
    case IBUS_KEY_Shift_L:
    case IBUS_KEY_Shift_R:
    case IBUS_KEY_Control_L:
    case IBUS_KEY_Control_R:
    case IBUS_KEY_Alt_L:
    case IBUS_KEY_Meta_L:
    case IBUS_KEY_Meta_R:
      return (gboolean)riti_context_ongoing_input_session(ctx);
    default:
      break;
  }

  // Set modifiers
  if (state & IBUS_SHIFT_MASK) {
    modifier |= MODIFIER_SHIFT;
  }

  if (state & IBUS_CONTROL_MASK) {
    ctrl_key = true;
  }

  if (state & IBUS_MOD1_MASK) {
    alt_key = true;
  }

  // Convert the key value into riti's key value.
  uint16_t key = ibus_keycode(keyval);

  // Reject the key which has only Ctrl or Alt (not the right one) combination and riti doesn't handle.
  if ((ctrl_key && !alt_key) || (!ctrl_key && alt_key && !altGr) || key == VC_UNKNOWN) {
    if (riti_context_ongoing_input_session(ctx)) {
      engine_commit();
    }
    return FALSE;
  }

  // If we have Ctrl and Alt combination or the right Alt, set it as the AltGr modifier.
  if ((ctrl_key && alt_key) || altGr) {
    modifier |= MODIFIER_ALT_GR;
  }

  int index = 0;

  if (riti_context_ongoing_input_session(ctx) && !riti_suggestion_is_lonely(suggestion)) {
    index = ibus_lookup_table_get_cursor_pos(table);
  }

  suggestion = riti_get_suggestion_for_key(ctx, key, modifier, index);

  if (!riti_suggestion_is_empty(suggestion)) {
    engine_update_lookup_table();
  } else {
    // Corner case: When old style kar typing is enabled, a lonely suggestion and an empty
    // suggestion is not distinguishable. So we accept the key event if a input session
    // is ongoing.
    return riti_context_ongoing_input_session(ctx);
  }

  return TRUE;
}

/************************************* Callbacks *************************************/

gboolean engine_process_key_event_cb(IBusEngine *engine,
                                     guint keyval,
                                     guint keycode,
                                     guint state) {
  // Don't accept Key Release event
  if (state & IBUS_RELEASE_MASK) {
    if (keyval == IBUS_KEY_Alt_R || keyval == IBUS_KEY_ISO_Level3_Shift) {
      altGr = false;
    }
    return FALSE;
  }

  gboolean ret = engine_process_key(keyval, keycode, state);
  LOG_DEBUG("[IM:iBus]: Key event was %s\n", ret ? "accepted" : "rejected");
  return ret;
}

void engine_enable_cb(IBusEngine *engine) {
  LOG_INFO("[IM:iBus]: IM enabled\n");
  update_with_settings();
  riti_context_update_engine(ctx, config);
}

void engine_disable_cb(IBusEngine *engine) {
  LOG_INFO("[IM:iBus]: IM disabled\n");
}

void engine_focus_out_cb(IBusEngine *engine) {
  LOG_DEBUG("[IM:iBus]: IM Focus out\n");

  if (riti_context_ongoing_input_session(ctx)) {
    riti_context_finish_input_session(ctx);
    engine_reset();
  }
}

void engine_reset_cb(IBusEngine *engine) {
  LOG_DEBUG("[IM:iBus]: IM Reset\n");

  if (riti_context_ongoing_input_session(ctx)) {
    riti_context_finish_input_session(ctx);
    engine_reset();
  }
}

void engine_candidate_clicked_cb(IBusEngine *engine, guint index, guint button, guint state) {
  IBusText *text = ibus_lookup_table_get_candidate(table, index);
  engine_commit_text(text);
  riti_context_candidate_committed(ctx, index);
}

IBusEngine *create_engine_cb(IBusFactory *factory,
                             gchar *engine_name,
                             gpointer user_data) {
  id += 1;
  gchar *path = g_strdup_printf("/org/freedesktop/IBus/Engine/%i", id);
  engine = ibus_engine_new(engine_name,
                           path,
                           ibus_bus_get_connection(bus));

  // Setup Lookup table
  table = ibus_lookup_table_new(10, 0, TRUE, TRUE);
  g_object_ref_sink(table);

  LOG_INFO("[IM:iBus]: Creating IM Engine\n");
  LOG_DEBUG("[IM:iBus]: Creating IM Engine with name:%s and id:%d\n", engine_name, id);

  g_signal_connect(engine, "process-key-event", G_CALLBACK(engine_process_key_event_cb), NULL);
  g_signal_connect(engine, "enable", G_CALLBACK(engine_enable_cb), NULL);
  g_signal_connect(engine, "disable", G_CALLBACK(engine_disable_cb), NULL);
  g_signal_connect(engine, "focus-out", G_CALLBACK(engine_focus_out_cb), NULL);
  g_signal_connect(engine, "reset", G_CALLBACK(engine_reset_cb), NULL);
  g_signal_connect(engine, "candidate-clicked", G_CALLBACK(engine_candidate_clicked_cb), NULL);

  return engine;
}

void engine_disconnected_cb(IBusBus *bus, gpointer user_data) {
  ibus_quit();
  LOG_INFO("[IM:iBus]: Bus disconnected!\n");
}

void start_setup(bool ibus) {
  IBusComponent *component;

  LOG_DEBUG("[IM:iBus]: Started IM facilities.\n");

  ibus_init();

  bus = ibus_bus_new();
  g_signal_connect(bus, "disconnected", G_CALLBACK(engine_disconnected_cb), NULL);

  IBusFactory *factory = ibus_factory_new(ibus_bus_get_connection(bus));
  g_signal_connect(factory, "create-engine", G_CALLBACK(create_engine_cb), NULL);

  if (ibus) {
    ibus_bus_request_name(bus, "org.freedesktop.IBus.OpenBangla", 0);
  } else {
    std::error_code executableError;
    const auto executablePath =
        std::filesystem::read_symlink("/proc/self/exe", executableError);
    const std::string engineCommand =
        "\"" + (executableError ? std::string("ibus-engine-openbangla")
                                : executablePath.string()) +
        "\" --ibus";

    char *iconC = shanti_paths_user_icon_file();
    const std::string iconPath = iconC ? std::string(iconC) : std::string();
    shanti_string_free(iconC);

    component = ibus_component_new("org.freedesktop.IBus.OpenBangla",
                                   "OpenBangla Keyboard",
                                   PROJECT_VERSION,
                                   "GPL 3",
                                   "See AboutBox",
                                   "http://openbangla.github.io/",
                                   engineCommand.c_str(),
                                   "openbangla-keyboard"
    );

    ibus_component_add_engine(component,
                              ibus_engine_desc_new("OpenBangla",
                                                   "OpenBangla Keyboard",
                                                   "OpenBangla Keyboard IME for iBus",
                                                   "bn",
                                                   "GPL 3",
                                                   "See About Dialog",
                                                   iconPath.c_str(),
                                                   "default"
                              ));
    ibus_bus_register_component(bus, component);

    ibus_bus_set_global_engine_async(bus, "OpenBangla", -1, nullptr, nullptr, nullptr);
  }
  ibus_main();
}

int main(int argc, char *argv[]) {
  // Keep the UserFolders side effect: the user data tree (and its layouts
  // directory) is created by the engine too, exactly as before.
  shanti_paths_ensure_user_dirs();

  shanti_config_load();
  config = riti_config_new();
  update_with_settings();
  ctx = riti_context_new_with_config(config);

  start_setup(argc > 1 && strcmp(argv[1], "--ibus") == 0);

  riti_context_free(ctx);
  return 0;
}
