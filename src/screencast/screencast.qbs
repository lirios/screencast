import qbs 1.0

QtGuiApplication {
    name: "liri-screencast"
    targetName: "liri-screencast"

    Depends { name: "lirideployment" }
    Depends { name: "Qt"; submodules: ["core", "dbus"]; versionAtLeast: project.minimumQtVersion }
    Depends { name: "Qt5GStreamer.Utils" }

    condition: {
        if (!Qt5GStreamer.Utils.found) {
            console.error("Qt5GStreamer is required to build " + targetName);
            return false;
        }

        return true;
    }

    cpp.defines: [
        'LIRISCREENCAST_VERSION="' + project.version + '"',
        "QT_NO_CAST_FROM_ASCII",
        "QT_NO_CAST_TO_ASCII"
    ]

    files: [
        "main.cpp",
        "screencast.cpp",
        "screencast.h",
    ]

    Group {
        name: "Translations"
        files: ["*_*.ts"]
        prefix: "translations/"
    }

    Group {
        name: "D-Bus Interfaces"
        files: [
            "io.liri.Shell.Outputs.xml",
            "io.liri.Shell.ScreenCast.xml",
        ]
        fileTags: ["qt.dbus.interface"]
    }

    Group {
        qbs.install: true
        qbs.installDir: lirideployment.binDir
        fileTagsFilter: "application"
    }

    Group {
        qbs.install: true
        qbs.installDir: lirideployment.dataDir + "/liri-screencast/translations"
        fileTagsFilter: "qm"
    }
}
