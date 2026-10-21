# Rust crates shared by the engines (and, later, by the GTK GUI).
#
# Corrosion imported targets: the crate name with dashes replaced by
# underscores — see CorrosionGenerator.cmake ("Explicitly set library names
# have always been forbidden from using dashes ... Corrosion will thus
# replace dashes with underscores"). The link targets are therefore
# `shanti_config` and `shanti_paths` (underscore), even though the Cargo
# packages are `shanti-config` / `shanti-paths` (likewise `shanti_keysyms`
# for `shanti-keysyms`).
#
# Consumers must also add the include/ directory of each crate for its
# hand-written C ABI header.

corrosion_import_crate(
    MANIFEST_PATH shanti-config/Cargo.toml
    CRATE_TYPES staticlib
)

corrosion_import_crate(
    MANIFEST_PATH shanti-paths/Cargo.toml
    CRATE_TYPES staticlib
)

corrosion_import_crate(
    MANIFEST_PATH shanti-keysyms/Cargo.toml
    CRATE_TYPES staticlib
)
