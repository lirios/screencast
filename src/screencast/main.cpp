/****************************************************************************
 * This file is part of Liri.
 *
 * Copyright (C) 2016 Pier Luigi Fiorini
 *
 * Author(s):
 *    Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * $BEGIN_LICENSE:GPL3+$
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QCoreApplication>
#include <QDBusConnection>

#include <Qt5GStreamer/QGst/Init>

#include "screencast.h"

#define TR(x) QT_TRANSLATE_NOOP("Command line parser", QLatin1String(x))

int main(int argc, char *argv[])
{
    // Setup application
    QCoreApplication app(argc, argv);
    app.setApplicationName(QLatin1String("ScreenCast"));
    app.setApplicationVersion(QLatin1String(LIRISCREENCAST_VERSION));
    app.setOrganizationDomain(QLatin1String("liri.io"));
    app.setOrganizationName(QLatin1String("Liri"));

    // Command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(QLatin1String("Simple screen capture program for Liri OS"));
    parser.addHelpOption();
    parser.addVersionOption();

    // Parse command line
    parser.process(app);

    // Check if the D-Bus session bus is available
    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.");
        return 1;
    }

    // Initialize QtGStreamer
    QGst::init(&argc, &argv);

    // Run the application
    Screencast *screencap = new Screencast();
    QCoreApplication::postEvent(screencap, new StartupEvent());
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     screencap, &Screencast::deleteLater);

    return app.exec();
}
