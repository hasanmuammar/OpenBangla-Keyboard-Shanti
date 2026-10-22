# This CMake module handles the installation of files

if(OS_LINUX)
        configure_file(${CMAKE_SOURCE_DIR}/data/shanti-keyboard.desktop.in ${CMAKE_BINARY_DIR}/data/shanti-keyboard.desktop)

        install(FILES ${CMAKE_SOURCE_DIR}/data/32.png
                DESTINATION ${PROJECT_DATADIR}/icons
                RENAME Shanti-Keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/1024.png
                DESTINATION ${DATADIR}/pixmaps/
                RENAME shanti-keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/16.png
                DESTINATION ${DATADIR}/icons/hicolor/16x16/apps/
                RENAME shanti-keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/24.png
                DESTINATION ${DATADIR}/icons/hicolor/24x24/apps/
                RENAME shanti-keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/32.png
                DESTINATION ${DATADIR}/icons/hicolor/32x32/apps/
                RENAME shanti-keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/48.png
                DESTINATION ${DATADIR}/icons/hicolor/48x48/apps/
                RENAME shanti-keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/64.png
                DESTINATION ${DATADIR}/icons/hicolor/64x64/apps/
                RENAME shanti-keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/128.png
                DESTINATION ${DATADIR}/icons/hicolor/128x128/apps/
                RENAME shanti-keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/512.png
                DESTINATION ${DATADIR}/icons/hicolor/512x512/apps/
                RENAME shanti-keyboard.png)

        install(FILES ${CMAKE_SOURCE_DIR}/data/1024.png
                DESTINATION ${DATADIR}/icons/hicolor/1024x1024/apps/
                RENAME shanti-keyboard.png)
        
        install(FILES ${CMAKE_BINARY_DIR}/data/shanti-keyboard.desktop
                DESTINATION "${DATADIR}/applications")
        
        install(FILES ${CMAKE_SOURCE_DIR}/data/org.shanti.ShantiKeyboard.metainfo.xml
                DESTINATION ${DATADIR}/metainfo)
endif()

if(OS_LINUX)
        install(FILES ${CMAKE_SOURCE_DIR}/data/avrophonetic.json
                DESTINATION ${PROJECT_DATADIR}/layouts)

        install(FILES ${CMAKE_SOURCE_DIR}/data/khipro.json
                DESTINATION ${PROJECT_DATADIR}/layouts)

        install(FILES ${CMAKE_SOURCE_DIR}/data/Borno.json
                DESTINATION ${PROJECT_DATADIR}/layouts)

        install(FILES ${CMAKE_SOURCE_DIR}/data/Avro_Easy.json
                DESTINATION ${PROJECT_DATADIR}/layouts)

        install(FILES ${CMAKE_SOURCE_DIR}/data/Munir_Optima.json
                DESTINATION ${PROJECT_DATADIR}/layouts)

        install(FILES ${CMAKE_SOURCE_DIR}/data/National_Jatiya.json
                DESTINATION ${PROJECT_DATADIR}/layouts)

        install(FILES ${CMAKE_SOURCE_DIR}/data/Probhat.json
                DESTINATION ${PROJECT_DATADIR}/layouts)

        install(FILES ${CMAKE_SOURCE_DIR}/src/engine/riti/data/autocorrect.json
                DESTINATION ${PROJECT_DATADIR}/data)

        install(FILES ${CMAKE_SOURCE_DIR}/data/dictionary.json
                DESTINATION ${PROJECT_DATADIR}/data)

        install(FILES ${CMAKE_SOURCE_DIR}/data/suffix.json
                DESTINATION ${PROJECT_DATADIR}/data)

        install(FILES ${CMAKE_SOURCE_DIR}/data/regex.json
                DESTINATION ${PROJECT_DATADIR}/data)
endif() # OS_LINUX