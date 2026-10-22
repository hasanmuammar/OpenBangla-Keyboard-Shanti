include_directories(../shared ../platform/include 3rdParty)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SRC_GUI
    TopBar.cpp
    AboutDialog.cpp
    AboutFile.cpp
    LayoutViewer.cpp
    KeyboardWidget.cpp
    SettingsDialog.cpp
    ToggleSwitch.cpp
    FlatpakIbusIntegration.cpp
    FcitxIntegration.cpp
    AutoCorrectDialog.cpp
    SingleInstance.cpp
    SingleInstance.h
    images.qrc
)

set(SRC_MAIN
    main.cpp
    Layout.cpp
    LayoutConverter.cpp
    LayoutConverter.h
    AvroPhonetic.cpp
    PlatformConfig.cpp
)

add_executable(shanti-gui ${SRC_MAIN} ${SRC_GUI})
target_link_libraries(
    shanti-gui
    libShared
    Qt5::Widgets
    Qt5::Network
    Qt5::Svg
    Qt5::DBus
    platform
    ${ZSTD_LIBRARIES}
)

install(TARGETS shanti-gui
    RUNTIME DESTINATION ${BIN_DIR}
)
