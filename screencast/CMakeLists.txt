include_directories(
    ${CMAKE_SOURCE_DIR}/headers
    ${CMAKE_BINARY_DIR}/headers
)

set(SOURCES
    main.cpp
    screencast.cpp
)

vibe_translate_ts(QM_FILES
                  SOURCES ${SOURCES}
                  TEMPLATE liri-screencast
                  INSTALL_DIR ${DATA_INSTALL_DIR}/liri-workspace/translations
                  COMPONENT "Runtime")
vibe_translation_loader(SOURCES
                        TRANSLATIONS_DIR liri-workspace/translations
                        CATALOG_NAME liri-screencast)

add_executable(liri-screencast ${SOURCES})
set_target_properties(liri-screencast PROPERTIES
    COMPILE_FLAGS "-fexceptions")
target_link_libraries(liri-screencast
                      Qt5::Core
                      Qt5::Gui
                      GreenIsland::Client
                      Qt5GStreamerUtils)

install(TARGETS liri-screencast
        DESTINATION ${CMAKE_INSTALL_BINDIR})
