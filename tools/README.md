# Fork tooling

The fork-specific installation workflow is under `tools/portable/`.

The prebuilt installer requires Python 3 for safe tar archive validation, plus GNU tar, `sha256sum`, `realpath`, `find`, `mktemp`, and either `curl` or `wget`. It does not need build tools or development packages.

`tools/install.sh` is the normal user-facing installer. It detects desktop environment and CPU architecture, downloads the matching prebuilt Linux release, strictly validates the single-record SHA-256 checksum, checks archive paths and symbolic-link containment before modifying the installed program, and verifies that the staged payload includes the files required for the selected backend.

The checksum is fetched next to the archive, so it detects accidental corruption but is not, by itself, an independent publisher-authentication mechanism. New release builds also publish GitHub/Sigstore provenance attestations. Users with the GitHub CLI can require signed provenance verification with:

```bash
bash tools/install.sh --version TAG --verify-provenance
```

This requires a published, attested release tag and fails closed if the attestation does not match this repository's release workflow and the selected tag. It remains optional so the ordinary install flow does not require the GitHub CLI.

`tools/uninstall.sh` removes the user-local installation and backend registration. User data and caches are preserved unless separately requested with `--purge-data` or `--purge-cache`; removals are constrained to explicit Shanti targets.

Release archives bundle `install.sh`, `uninstall.sh`, their `portable/` helpers and `release-tag.txt` under `share/shanti-keyboard/`, so a downloaded archive can install, repair or uninstall without a source checkout. Installing relocates that directory into the user data directory (`~/.local/share/shanti-keyboard/` by default).

The portable installer is the primary installation path for this fork.

## External build and desktop projects

The installer integrates with [Toolbx](https://github.com/containers/toolbox), [Distrobox](https://github.com/89luca89/distrobox), [Podman](https://github.com/containers/podman) or [Docker](https://github.com/moby/moby) for isolated builds, and with [IBus](https://github.com/ibus/ibus) or [Fcitx5](https://github.com/fcitx/fcitx5) for Linux input-method integration.

## Source builds

`tools/build.sh` is the developer-facing source-build entry point. It can use Toolbx, Distrobox, Podman or Docker to isolate build dependencies and can also create release archives.
