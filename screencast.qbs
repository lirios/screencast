import qbs 1.0

Project {
    name: "Screencast"

    readonly property string version: "0.9.0"

    readonly property string minimumQtVersion: "5.10.0"

    property bool useStaticAnalyzer: false

    condition: qbs.targetOS.contains("linux") && !qbs.targetOS.contains("android")

    minimumQbsVersion: "1.9.0"

    references: [
        "src/screencast/screencast.qbs",
    ]
}
