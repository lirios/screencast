include_directories(
    ${CMAKE_SOURCE_DIR}/headers
    ${CMAKE_BINARY_DIR}/headers
)

set(SOURCES
    main.cpp
    screencap.cpp
)

add_executable(hawaii-screencap ${SOURCES})
set_target_properties(hawaii-screencap PROPERTIES
    COMPILE_FLAGS "-fexceptions")
target_link_libraries(hawaii-screencap
                      Qt5::Core
                      Qt5::Gui
                      GreenIsland::Client
                      Qt5GStreamerUtils)

install(TARGETS hawaii-screencap
        DESTINATION ${CMAKE_INSTALL_BINDIR})
