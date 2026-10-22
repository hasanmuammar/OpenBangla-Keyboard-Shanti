/*
 *  Shanti Keyboard
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

/*  IBus engine shell for Shanti Keyboard.
 *
 *  Pure libibus plumbing since rewrite phase 2 (docs/REWRITE-PLAN.md): the
 *  engine state machine (settings application, key filtering, suggestion
 *  handling, riti session) lives in the shanti-core crate and decides what
 *  happens; this shell renders those decisions with libibus calls.
 *
 *  Qt-free since phase 1 — configuration flows through shanti-core, which
 *  reads the same `$XDG_CONFIG_HOME/Shanti/Shanti.conf` store the Qt
 *  GUI writes. */

#include <ibus.h>
#include <cstring>
#include <filesystem>
#include <string>
#include "shanti-core.h"
#include "shanti-paths.h"
#include "Log.h"

static ShantiCore *core = nullptr;
static IBusBus *bus = nullptr;
static IBusEngine *engine = nullptr;
static IBusLookupTable *table = nullptr;
static gint id = 0;

/************************************* Rendering *************************************/

void engine_update_preedit() {
  char *txt = shanti_core_preedit_text(core);
  IBusText *text = ibus_text_new_from_string(txt);
  shanti_core_string_free(txt);

  ibus_engine_update_preedit_text_with_mode(engine, text, ibus_text_get_length(text), TRUE, IBUS_ENGINE_PREEDIT_COMMIT);
}

void engine_update_lookup_table() {
  if (shanti_core_has_auxiliary(core)) {
    char *aux = shanti_core_auxiliary_text(core);
    IBusText *auxiliary = ibus_text_new_from_string(aux);
    shanti_core_string_free(aux);

    ibus_lookup_table_clear(table);
    ibus_engine_update_auxiliary_text(engine, auxiliary, TRUE);
  }

  int len = shanti_core_candidate_count(core);
  for (int i = 0; i < len; i++) {
    char *txt = shanti_core_candidate(core, i);
    IBusText *text = ibus_text_new_from_string(txt);
    shanti_core_string_free(txt);
    ibus_lookup_table_append_candidate(table, text);
  }

  ibus_lookup_table_set_orientation(
      table, shanti_core_candidates_horizontal(core) ? IBUS_ORIENTATION_HORIZONTAL
                                                     : IBUS_ORIENTATION_VERTICAL);
  ibus_lookup_table_set_cursor_pos(table, shanti_core_selection(core));

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

/* Commit `text` (owned-crate string) and render the reset. */
void engine_commit(char *text) {
  engine_commit_text(ibus_text_new_from_string(text));
  shanti_core_string_free(text);
}

/************************************* Callbacks *************************************/

gboolean engine_process_key_event_cb(IBusEngine *engine,
                                     guint keyval,
                                     guint keycode,
                                     guint state) {
  switch (shanti_core_key_event(core, keyval, state)) {
    case SHANTI_KEY_PASS_THROUGH:
      return FALSE;
    case SHANTI_KEY_SWALLOW:
      return TRUE;
    case SHANTI_KEY_UPDATE_CANDIDATES:
      engine_update_lookup_table();
      return TRUE;
    case SHANTI_KEY_UPDATE_PREEDIT:
      ibus_lookup_table_set_cursor_pos(table, shanti_core_selection(core));
      engine_update_preedit();
      return TRUE;
    case SHANTI_KEY_COMMIT:
      engine_commit(shanti_core_last_committed_text(core));
      return TRUE;
    case SHANTI_KEY_COMMIT_PASS:
      engine_commit(shanti_core_last_committed_text(core));
      return FALSE;
    case SHANTI_KEY_HIDE_WINDOW:
      engine_reset();
      return TRUE;
    default:
      LOG_DEBUG("[IM:iBus]: Unrecognised engine action\n");
      return FALSE;
  }
}

void engine_enable_cb(IBusEngine *engine) {
  LOG_INFO("[IM:iBus]: IM enabled\n");
  shanti_core_apply_settings(core);
}

void engine_disable_cb(IBusEngine *engine) {
  LOG_INFO("[IM:iBus]: IM disabled\n");
}

void engine_focus_out_cb(IBusEngine *engine) {
  LOG_DEBUG("[IM:iBus]: IM Focus out\n");

  if (shanti_core_ongoing_input_session(core)) {
    shanti_core_finish_session(core);
    engine_reset();
  }
}

void engine_reset_cb(IBusEngine *engine) {
  LOG_DEBUG("[IM:iBus]: IM Reset\n");

  if (shanti_core_ongoing_input_session(core)) {
    shanti_core_finish_session(core);
    engine_reset();
  }
}

void engine_candidate_clicked_cb(IBusEngine *engine, guint index, guint button, guint state) {
  shanti_core_candidate_clicked(core, index);
  engine_commit(shanti_core_last_committed_text(core));
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
    ibus_bus_request_name(bus, "org.freedesktop.IBus.Shanti", 0);
  } else {
    std::error_code executableError;
    const auto executablePath =
        std::filesystem::read_symlink("/proc/self/exe", executableError);
    const std::string engineCommand =
        "\"" + (executableError ? std::string("ibus-engine-shanti")
                                : executablePath.string()) +
        "\" --ibus";

    char *iconC = shanti_paths_user_icon_file();
    const std::string iconPath = iconC ? std::string(iconC) : std::string();
    shanti_string_free(iconC);

    component = ibus_component_new("org.freedesktop.IBus.Shanti",
                                   "Shanti Keyboard",
                                   PROJECT_VERSION,
                                   "GPL 3",
                                   "See AboutBox",
                                   "https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti",
                                   engineCommand.c_str(),
                                   "shanti-keyboard"
    );

    ibus_component_add_engine(component,
                              ibus_engine_desc_new("Shanti",
                                                   "Shanti Keyboard",
                                                   "Shanti Keyboard IME for iBus",
                                                   "bn",
                                                   "GPL 3",
                                                   "See About Dialog",
                                                   iconPath.c_str(),
                                                   "default"
                              ));
    ibus_bus_register_component(bus, component);

    ibus_bus_set_global_engine_async(bus, "Shanti", -1, nullptr, nullptr, nullptr);
  }
  ibus_main();
}

int main(int argc, char *argv[]) {
  /* The core keeps the UserFolders side effect: the user data tree (and its
   * layouts directory) is created by shanti_core_new(), exactly as before. */
  core = shanti_core_new();

  start_setup(argc > 1 && strcmp(argv[1], "--ibus") == 0);

  shanti_core_free(core);
  return 0;
}
