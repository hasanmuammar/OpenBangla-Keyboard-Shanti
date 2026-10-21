include_directories(../riti/include ../../shared ${IBUS_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/crates/shanti-config/include"
    "${CMAKE_SOURCE_DIR}/crates/shanti-paths/include")
add_executable(ibus-engine-openbangla main.cpp)
# Qt-free since rewrite phase 1: settings/paths come from the shanti-config
# and shanti-paths crates instead of libShared (which is QSettings-based).
# Corrosion target names have underscores (dashes are replaced when the
# crate is imported; see crates/CMakeLists.txt).
target_link_libraries(ibus-engine-openbangla riti shanti_config shanti_paths ${IBUS_LIBRARIES} dl pthread)

configure_file(${CMAKE_SOURCE_DIR}/data/openbangla.xml.in ${CMAKE_BINARY_DIR}/data/openbangla.xml)

install(FILES ${CMAKE_BINARY_DIR}/data/openbangla.xml
        DESTINATION "${DATADIR}/ibus/component")

install(TARGETS ibus-engine-openbangla
        RUNTIME DESTINATION ${LIBEXECDIR})
