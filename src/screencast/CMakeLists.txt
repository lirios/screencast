# Translations
file(GLOB LiriScreencast_TRANSLATIONS "${CMAKE_CURRENT_SOURCE_DIR}/translations/*_*.ts")
qt5_add_translation(LiriScreencast_QM_FILES ${LiriScreencast_TRANSLATIONS})
install(FILES ${LiriScreencast_QM_FILES}
        DESTINATION "${INSTALL_DATADIR}/liri-screencast/translations")

find_package(GStreamer REQUIRED)

liri_add_executable(LiriScreencast
    OUTPUT_NAME
        "liri-screencast"
    SOURCES
        main.cpp
        screencast.cpp
        screencast.h
        ${LiriScreencast_QM_FILES}
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
        LIRISCREENCAST_VERSION="${PROJECT_VERSION}"
    LIBRARIES
        Qt5::Core
        Qt5::DBus
        PkgConfig::GStreamer
)
