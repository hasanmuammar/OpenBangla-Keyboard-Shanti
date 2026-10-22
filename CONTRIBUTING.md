# Contributing to this fork

This repository is an experimental Linux-focused fork of OpenBangla Keyboard, rebranded as Shanti Keyboard.

## Scope

Changes should be relevant to the fork itself, especially:

- the portable user-local installer and uninstaller;
- Linux IBus and Fcitx5 integration;
- isolated build environments;
- bundled runtime handling;
- Linux CMake configuration;
- fork-specific documentation.

Windows and macOS support are not part of this fork.

## Development branch

Use the `develop` branch for fork development.

## Testing

For installer changes, test installation and removal with:

```bash
bash tools/install.sh
bash tools/uninstall.sh
```

For IBus changes, verify the engine with:

```bash
ibus list-engine | grep -i -A3 -B2 shanti
```

For a manual CMake check, configure exactly one backend:

```bash
cmake -S . -B build -G Ninja \
  -DENABLE_IBUS=ON \
  -DENABLE_FCITX=OFF

cmake --build build --parallel 2
```

For Fcitx5, switch the two backend options accordingly.

## Documentation

Fork-specific documentation belongs in `docs/wiki/`. Do not direct users to the upstream project's installation or configuration documentation for this fork.

## Commit messages

Keep commit messages concise and describe the actual change made in this fork.
