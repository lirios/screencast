/****************************************************************************
 * This file is part of Hawaii.
 *
 * Copyright (C) 2016 Pier Luigi Fiorini
 *
 * Author(s):
 *    Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * $BEGIN_LICENSE:GPL2+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $END_LICENSE$
 ***************************************************************************/

#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <QtGui/QGuiApplication>

#include <Qt5GStreamer/QGst/Init>

#include "config.h"
#include "screencap.h"

#define TR(x) QT_TRANSLATE_NOOP("Command line parser", QStringLiteral(x))

int main(int argc, char *argv[])
{
    // Force using the wayland QPA plugin
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("wayland"));

    // Setup application
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Screen Capture"));
    app.setApplicationVersion(QStringLiteral(HAWAII_WORKSPACE_VERSION));
    app.setOrganizationDomain(QStringLiteral("hawaiios.org"));
    app.setOrganizationName(QStringLiteral("Hawaii"));

    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Simple screen capture program for Hawaii"));
    parser.addHelpOption();
    parser.addVersionOption();

    // Parse command line
    parser.process(app);

    // Need to be running with wayland QPA
    if (!QGuiApplication::platformName().startsWith(QStringLiteral("wayland"))) {
        qCritical("This application requires a Wayland session");
        return 1;
    }

    // Initialize QtGStreamer
    QGst::init(&argc, &argv);

    // Run the application
    Screencap *screencap = new Screencap();
    QGuiApplication::postEvent(screencap, new StartupEvent());
    QObject::connect(&app, &QGuiApplication::aboutToQuit,
                     screencap, &Screencap::deleteLater);

    return app.exec();
}
