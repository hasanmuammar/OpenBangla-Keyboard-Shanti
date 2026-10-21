//! Engine core for the Shanti rewrite (see `docs/REWRITE-PLAN.md` phase 2):
//! settings application, the key-event state machine, suggestion handling
//! and the riti session, extracted from the IBus engine's `main.cpp` so the
//! platform shells reduce to libibus/Fcitx5 plumbing.
//!
//! The C ABI that the C++ shells consume lives in `ffi.rs`
//! (`include/shanti-core.h`); the Rust-side state machine in `engine.rs`;
//! the GUI settings-store mapping in `settings.rs`.

mod engine;
mod ffi;
mod settings;

pub use engine::{KeyAction, ShantiCore};
pub use settings::Viewport;
