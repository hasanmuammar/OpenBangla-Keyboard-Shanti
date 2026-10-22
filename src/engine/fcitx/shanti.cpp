/*
 *  Shanti Keyboard
 *  Copyright (C) 2012-2012 Yichao Yu
 *  Copyright (C) 2020-2025 CSSlayer <wengxt@gmail.com>
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
/*  Fcitx5 addon shell for Shanti.
 *
 *  Pure Fcitx5 plumbing since rewrite phase 2 (docs/REWRITE-PLAN.md): the
 *  engine state machine (settings application, key filtering, suggestion
 *  handling, riti session) lives in the shanti-core crate and decides what
 *  happens; this shell renders those decisions with Fcitx5 calls.
 *
 *  Fcitx5's addon API is a C++ ABI interface, so this shell stays a C++
 *  class by design (documented hard limit in the rewrite plan). */

#include "shanti.h"
#include "shanti-core.h"
#include <fcitx-utils/capabilityflags.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/log.h>
#include <fcitx/addoninstance.h>
#include <fcitx/candidatelist.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/inputpanel.h>
#include <fcitx/text.h>
#include <fcitx/userinterface.h>
#include <memory>
#include <string>

/* The core expects IBus semantics for the state bits: release is IBus's
 * bit 1<<30; Fcitx reports release separately on the KeyEvent. */
#define SHANTI_IBUS_RELEASE_MASK (1u << 30)

FCITX_DEFINE_LOG_CATEGORY(shanti, "shanti");
#define FCITX_SHANTI_DEBUG() FCITX_LOGC(shanti, Debug)

namespace fcitx {

class ShantiState : public InputContextProperty {
public:
  ShantiState(ShantiEngine *engine, InputContext &ic)
      : engine_(engine), ic_(&ic) {}

  ~ShantiState() {}

  class ShantiCandidate : public CandidateWord {
  public:
    ShantiCandidate(ShantiEngine *engine, std::string text, int index)
        : CandidateWord(Text(std::move(text))), engine_(engine), index_(index) {
    }

    void select(InputContext *inputContext) const override {
      auto *state = inputContext->propertyFor(engine_->factory());
      state->selectCandidate(text().toString(), index_);
    }

  private:
    ShantiEngine *engine_;
    int index_;
  };

  class ShantiCandidateList : public CommonCandidateList {
  public:
    void setGlobalCursorIndex(int index) {
      int items = this->pageSize();
      int page = index / items;
      setPage(page);
      CommonCandidateList::setGlobalCursorIndex(index);
    }
  };

  void selectCandidate(const std::string & /*text*/, int index) {
    engine_->syncInputContext(ic_);
    shanti_core_candidate_clicked(engine_->core(), index);
    char *txt = shanti_core_last_committed_text(engine_->core());
    ic_->commitString(std::string(txt));
    shanti_core_string_free(txt);
    reset();
  }

  void updatePreedit() {
    char *txt = shanti_core_preedit_text(engine_->core());
    std::string text(txt);
    shanti_core_string_free(txt);

    /* Mirror the logical selection onto the widget so the cursor stays in
     * sync with the core. */
    auto candidateList = std::dynamic_pointer_cast<CommonCandidateList>(
        ic_->inputPanel().candidateList());
    if (candidateList) {
      candidateList->setGlobalCursorIndex(
          shanti_core_selection(engine_->core()));
    }

    Text preedit(std::move(text));
    preedit.setCursor(preedit.textLength());
    if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
      ic_->inputPanel().setClientPreedit(preedit);
      ic_->updatePreedit();
    }
    // we don't use input panel preedit here, because the aux text is actually
    // enough.
  }

  /* Render the candidate window + auxiliary text + preedit from the core
   * state (the old updateUI). */
  void updateUI() {
    ic_->inputPanel().reset();
    do {
      if (shanti_core_has_auxiliary(engine_->core())) {
        char *aux = shanti_core_auxiliary_text(engine_->core());
        if (aux && aux[0]) {
          ic_->inputPanel().setAuxUp(Text(std::string(aux)));
        }
        shanti_core_string_free(aux);

        auto candidateList = std::make_unique<ShantiCandidateList>();
        auto len = shanti_core_candidate_count(engine_->core());
        for (int i = 0; i < len; i++) {
          char *text = shanti_core_candidate(engine_->core(), i);
          candidateList->append<ShantiCandidate>(engine_, text, i);
          shanti_core_string_free(text);
        }

        candidateList->setLayoutHint(shanti_core_candidates_horizontal(engine_->core())
                                         ? CandidateLayoutHint::Horizontal
                                         : CandidateLayoutHint::Vertical);
        candidateList->setGlobalCursorIndex(shanti_core_selection(engine_->core()));
        ic_->inputPanel().setCandidateList(std::move(candidateList));
      }
      updatePreedit();
    } while (0);
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
  }

  void reset() {
    shanti_core_reset_state(engine_->core());
    ic_->inputPanel().reset();
    ic_->updatePreedit();
    ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
  }

  void keyEvent(KeyEvent &keyEvent) {
    auto key = keyEvent.rawKey();

    /* Release events only matter for AltGr tracking inside the core; like
     * the old engine, they pass through untouched and never switch
     * contexts. */
    if (keyEvent.isRelease()) {
      shanti_core_key_event(engine_->core(), key.sym(), SHANTI_IBUS_RELEASE_MASK);
      return;
    }

    engine_->syncInputContext(ic_);

    /* Old engine behaviour kept: BackSpace before any session just clears
     * our leftover panel and is forwarded to the application. */
    if (key.sym() == FcitxKey_BackSpace &&
        !shanti_core_ongoing_input_session(engine_->core())) {
      /* covers the old reloadConfig() call made when no session is ongoing */
      shanti_core_apply_settings(engine_->core());
      reset();
      return;
    }

    switch (shanti_core_key_event(engine_->core(), key.sym(), key.states())) {
    case SHANTI_KEY_PASS_THROUGH:
      // Let Fcitx forward the key; no UI change.
      return;
    case SHANTI_KEY_SWALLOW:
      keyEvent.filterAndAccept();
      return;
    case SHANTI_KEY_UPDATE_CANDIDATES:
      updateUI();
      keyEvent.filterAndAccept();
      return;
    case SHANTI_KEY_UPDATE_PREEDIT:
      updatePreedit();
      ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
      keyEvent.filterAndAccept();
      return;
    case SHANTI_KEY_COMMIT:
      commit();
      keyEvent.filterAndAccept();
      return;
    case SHANTI_KEY_COMMIT_PASS:
      commit();
      return;
    case SHANTI_KEY_HIDE_WINDOW:
      // Old engine: the BackSpace that emptied the composition is still
      // swallowed (see engine_process_key: the key never reaches the app).
      keyEvent.filterAndAccept();
      reset();
      return;
    default:
      return;
    }
  }

  /* Commit the text the core just prepared and clear the panels. */
  void commit() {
    char *txt = shanti_core_last_committed_text(engine_->core());
    ic_->commitString(std::string(txt));
    shanti_core_string_free(txt);
    reset();
  }

private:
  ShantiEngine *engine_;
  InputContext *ic_;
};

ShantiEngine::ShantiEngine(Instance *instance)
    : instance_(instance), factory_(
          [this](InputContext &ic) { return new ShantiState(this, ic); }) {
  /* shanti_core_new() keeps the UserFolders side effect: it creates the
   * user data tree (and its layouts directory) exactly as before. */
  core_ = shanti_core_new();
  instance->inputContextManager().registerProperty("shantiState",
                                                   &factory_);
}

ShantiEngine::~ShantiEngine() {
  if (core_) {
    shanti_core_free(core_);
    core_ = nullptr;
  }
}

void ShantiEngine::activate(const InputMethodEntry & /*entry*/,
                                InputContextEvent & /*event*/) {
  shanti_core_apply_settings(core_);
}

void ShantiEngine::reloadConfig() {
  /* Fcitx5 config-dialog hook: re-apply the settings store into the core. */
  shanti_core_apply_settings(core_);
}

void ShantiEngine::keyEvent(const InputMethodEntry & /*entry*/,
                                KeyEvent &keyEvent) {
  auto *ic = keyEvent.inputContext();
  auto *state = ic->propertyFor(&factory_);
  state->keyEvent(keyEvent);
}

void ShantiEngine::reset(const InputMethodEntry & /*entry*/,
                             InputContextEvent &event) {
  auto *state = event.inputContext()->propertyFor(&factory_);
  state->reset();
}

/* Check if the last input context is the current one; since the core is
 * shared, reset its state when the context switched. (The old engine's
 * context() was structurally the same but never seeded its tracking, so
 * the switch path was dormant — here it actually works.) */
void ShantiEngine::syncInputContext(InputContext *ic) {
  if (!currentIC_.isValid()) {
    currentIC_ = ic->watch();
    return;
  }
  if (ic != currentIC_.get()) {
    shanti_core_reset_state(core_);
    auto *oldIC = currentIC_.get();
    oldIC->inputPanel().reset();
    oldIC->updatePreedit();
    oldIC->updateUserInterface(UserInterfaceComponent::InputPanel);
    currentIC_ = ic->watch();
  }
}

FCITX_ADDON_FACTORY(ShantiFactory);
} // namespace fcitx
