include("${FCITX_INSTALL_CMAKECONFIG_DIR}/Fcitx5Utils/Fcitx5CompilerSettings.cmake")

find_package(Pthread REQUIRED)
find_package(DL REQUIRED)

add_library(openbangla MODULE openbangla.cpp)
# Since rewrite phase 2 the addon's engine state machine is the shanti-core
# crate (same core the IBus shell uses). The corrosion target name is
# `shanti_core` (underscores; see crates/CMakeLists.txt).
target_link_libraries(openbangla Fcitx5::Core shanti_core Pthread::Pthread DL::DL)
target_include_directories(openbangla PRIVATE
    "${CMAKE_SOURCE_DIR}/crates/shanti-core/include")
set_target_properties(openbangla PROPERTIES PREFIX "")
install(TARGETS openbangla DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/openbangla.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/openbangla-addon.conf" RENAME openbangla.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
