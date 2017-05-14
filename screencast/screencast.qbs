import qbs 1.0

QtGuiApplication {
    name: "liri-screencast"
    targetName: "liri-screencast"

    Depends { name: "lirideployment" }
    Depends { name: "Qt"; submodules: ["core", "gui"] }
    Depends { name: "LiriWaylandClient" }
    Depends { name: "Qt5GStreamer.Utils" }

    condition: {
        if (!Qt5GStreamer.Utils.found) {
            console.error("Qt5GStreamer is required to build " + targetName);
            return false;
        }

        return true;
    }

    cpp.defines: [
        'LIRIWORKSPACE_VERSION="' + project.version + '"',
        "QT_NO_CAST_FROM_ASCII",
        "QT_NO_CAST_TO_ASCII"
    ]

    files: ["*.cpp", "*.h"]

    Group {
        name: "Translations"
        files: ["*_*.ts"]
        prefix: "translations/"
    }

    Group {
        qbs.install: true
        qbs.installDir: lirideployment.binDir
        fileTagsFilter: product.type
    }

    Group {
        qbs.install: true
        qbs.installDir: lirideployment.dataDir + "/liri-workspace/translations"
        fileTagsFilter: "qm"
    }
}
