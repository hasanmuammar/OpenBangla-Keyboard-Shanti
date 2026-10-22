# Portable user-local build

This directory contains the fork-specific Linux source-build, runtime-bundling and user-local installation implementation. The normal installer downloads prebuilt releases and does not use this build environment.

- `common.sh` defines user-local paths and cache locations.
- `environment.sh` selects IBus or Fcitx5 and chooses Toolbx, Distrobox, Podman, or Docker.
- `build.sh` installs build dependencies inside the selected environment, configures CMake, builds the selected backend, stages the installation, and prepares bundled Qt/runtime libraries.
- `bundle-runtime.sh` handles the Linux shared-library closure and runtime modules such as the libproxy backend.
- `install.sh` copies the staged tree into the user-local prefix and registers the selected input method.
- `launch-bundled.sh` supplies the bundled library directory to the installed GUI and IBus engine.

## Main integrations

The portable build uses [CMake](https://github.com/Kitware/CMake), [Qt5](https://github.com/qt/qtbase), [Corrosion](https://github.com/corrosion-rs/corrosion) and [Zstandard](https://github.com/facebook/zstd), with Toolbx, Distrobox, Podman or Docker providing the isolated build environment.

## Build environment selection

The build prefers an existing Toolbx or Distrobox environment, then Podman or Docker. If no suitable builder exists, an interactive source build may offer to install Podman through a detected host package manager on supported systems. Non-interactive builds do not install host packages automatically. On Arch-based systems, the script deliberately refuses to run `pacman -Sy`; install Podman through the normal system-update workflow instead.

The Podman builder is persistent and named `shanti-builder`. It uses Debian 13 slim, with development dependencies cached inside that container. Existing incompatible builder containers are not removed without explicit confirmation, and automatic cleanup is restricted to a builder created by the current run.
