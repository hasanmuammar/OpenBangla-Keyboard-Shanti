# keyboard.conf — generated fixture

This file is **captured output from real Qt 5.15** (`QSettings("OpenBangla", "Keyboard")`,
NativeFormat, captured pre-rebrand — today the GUI writes `QSettings("Shanti", "Shanti")`).
It is the ground truth for
`shanti-config`'s parser: run `cargo test` in `crates/shanti-config` to verify
the parser reads it with Qt parity.

## Regenerating

From any machine with Qt5 dev packages (e.g. `tools/build.sh`'s toolbox):
`qmake`-less environments need only `qtbase` dev + a C++ compiler.

```bash
cat > /tmp/qt-dump.cpp <<'EOF'
#include <QSettings>
#include <QPoint>
#include <cstdio>
int main() {
  QSettings s("Shanti", "Shanti");  // pre-rebrand: ("OpenBangla", "Keyboard")
  // Mirrors exactly what src/frontend writes via src/shared/Settings.cpp setters.
  s.setValue("window/positions/TopBar", QPoint(120, 240));
  s.setValue("window/visibility/TopBar", true);
  s.setValue("settings/TrayInfoCount", (uint)3);
  s.setValue("window/positions/LayoutViewer", QPoint(50, 60));
  s.setValue("layout/name", QString::fromUtf8("Avro Phonetic"));
  s.setValue("layout/path", "avro_phonetic");
  s.setValue("settings/EnterKeyClosesPrevWin", false);
  s.setValue("settings/FixedLayout/ShowPrevWin", true);
  s.setValue("settings/FixedLayout/AutoVowelForm", false);
  s.setValue("settings/FixedLayout/AutoChandraPos", true);
  s.setValue("settings/FixedLayout/TraditionalKar", true);
  s.setValue("settings/FixedLayout/NumberPad", true);
  s.setValue("settings/FixedLayout/OldReph", false);
  s.setValue("settings/FixedLayout/OldKarOrder", true);
  s.setValue("settings/CandidateWin/Horizontal", true);
  s.setValue("settings/CandidateWin/Phonetic", false);
  s.setValue("settings/PreviewWin/IncludeEnglish", true);
  s.setValue("settings/ANSI", false);
  s.setValue("settings/SmartQuoting", true);
  s.setValue("settings/PreviousUserDataRemains", true);
  s.sync();
  printf("FILE=%s\n", s.fileName().toUtf8().constData());
  return 0;
}
EOF
g++ -fPIC $(pkg-config --cflags Qt5Core) /tmp/qt-dump.cpp $(pkg-config --libs Qt5Core) -o /tmp/qt-dump
export XDG_CONFIG_HOME=PATH_TO_THIS_DIRECTORY/stdout=/dev/null-tmpdir   # choose an empty temp dir
mkdir -p "$XDG_CONFIG_HOME" && /tmp/qt-dump
cp "$XDG_CONFIG_HOME/Shanti/Shanti.conf" tests/fixtures/keyboard.conf
```

Notes:
- Qt writes `$XDG_CONFIG_HOME/Shanti/Shanti.conf` (org/app subdirectory; before
  the rebrand: `$XDG_CONFIG_HOME/OpenBangla/Keyboard.conf`),
  NOT a `.plist` despite what older code/comments may suggest on macOS.
- The remaining group path components are folded into the key with `\`
  separators inside each section (`[settings]` + `FixedLayout\ShowPrevWin`).
- `QPoint` values serialize as `@Point(x y)`; bools as bare `true`/`false`;
  uints as bare integers; strings unquoted.

Keep the deliberately-flipped boolean values in place — they verify the
parser distinguishes written values from `Settings.cpp` defaults.
