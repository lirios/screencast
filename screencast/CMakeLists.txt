include_directories(
    ${CMAKE_SOURCE_DIR}/headers
    ${CMAKE_BINARY_DIR}/headers
)

set(SOURCES
    main.cpp
    screencast.cpp
)

add_executable(hawaii-screencast ${SOURCES})
set_target_properties(hawaii-screencast PROPERTIES
    COMPILE_FLAGS "-fexceptions")
target_link_libraries(hawaii-screencast
                      Qt5::Core
                      Qt5::Gui
                      GreenIsland::Client
                      Qt5GStreamerUtils)

install(TARGETS hawaii-screencast
        DESTINATION ${CMAKE_INSTALL_BINDIR})
