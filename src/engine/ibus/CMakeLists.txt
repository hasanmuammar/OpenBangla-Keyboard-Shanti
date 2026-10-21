include_directories(../../shared ${IBUS_INCLUDE_DIRS}
    "${CMAKE_SOURCE_DIR}/crates/shanti-paths/include"
    "${CMAKE_SOURCE_DIR}/crates/shanti-core/include")
add_executable(ibus-engine-openbangla main.cpp)
# Qt-free since phase 1; and since phase 2 the engine state machine (settings
# application, key events, suggestion handling) is the shanti-core crate and
# the shell renders its decisions with libibus calls. The corrosion target
# name is `shanti_core` (underscores; see crates/CMakeLists.txt).
target_link_libraries(ibus-engine-openbangla shanti_core shanti_paths ${IBUS_LIBRARIES} dl pthread)

configure_file(${CMAKE_SOURCE_DIR}/data/openbangla.xml.in ${CMAKE_BINARY_DIR}/data/openbangla.xml)

install(FILES ${CMAKE_BINARY_DIR}/data/openbangla.xml
        DESTINATION "${DATADIR}/ibus/component")

install(TARGETS ibus-engine-openbangla
        RUNTIME DESTINATION ${LIBEXECDIR})
