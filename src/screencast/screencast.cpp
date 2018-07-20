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

#include <QtCore/QDateTime>
#include <QtCore/QStandardPaths>

#include <Qt5GStreamer/QGlib/Connect>
#include <Qt5GStreamer/QGst/Buffer>
#include <Qt5GStreamer/QGst/Bus>
#include <Qt5GStreamer/QGst/Init>
#include <Qt5GStreamer/QGst/Message>
#include <Qt5GStreamer/QGst/Parse>
#include <Qt5GStreamer/QGst/Sample>

#include "outputs_interface.h"
#include "screencast.h"
#include "screencast_interface.h"

const QString dbusService = QLatin1String("io.liri.Session");
const QString outputsDBusPath = QLatin1String("/io/liri/Shell/Outputs");
const QString screenCastDBusPath = QLatin1String("/ScreenCast");

/*
 * StartupEvent
 */

static const QEvent::Type StartupEventType =
        static_cast<QEvent::Type>(QEvent::registerEventType());

StartupEvent::StartupEvent()
    : QEvent(StartupEventType)
{
}

/*
 * Screencast
 */

Screencast::Screencast(QObject *parent)
    : QObject(parent)
{
}

Screencast::~Screencast()
{
    if (m_outputs) {
        m_outputs->deleteLater();
        m_outputs = nullptr;
    }

    if (m_screenCast) {
        m_screenCast->deleteLater();
        m_screenCast = nullptr;
    }

    if (!m_pipeline.isNull()) {
        m_pipeline->setState(QGst::StateNull);
        m_pipeline.clear();
    }
}

bool Screencast::event(QEvent *event)
{
    if (event->type() == StartupEventType) {
        initialize();
        return true;
    }

    return QObject::event(event);
}

QString Screencast::videoFileName() const
{
    return QStringLiteral("%1/%2.ogv").arg(
                QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
                tr("Screencast from %1").arg(QDateTime::currentDateTime().toString(QLatin1String("yyyy-MM-dd hh:mm:ss"))));
}

void Screencast::initialize()
{
    if (m_initialized)
        return;

    m_initialized = true;

    m_outputs = new IoLiriShellOutputsInterface(
                dbusService, outputsDBusPath, QDBusConnection::sessionBus());
    if (!m_outputs->isValid()) {
        qWarning("Cannot find D-Bus service, please run this program under Liri Shell.");
        m_outputs->deleteLater();
        m_outputs = nullptr;
        QCoreApplication::exit(1);
        return;
    }

    m_screenCast = new IoLiriShellScreenCastInterface(
                dbusService, screenCastDBusPath, QDBusConnection::sessionBus());
    if (!m_screenCast->isValid()) {
        qWarning("Cannot find D-Bus service, please run this program under Liri Shell.");
        m_outputs->deleteLater();
        m_outputs = nullptr;
        m_screenCast->deleteLater();
        m_screenCast = nullptr;
        QCoreApplication::exit(1);
        return;
    }
    connect(m_screenCast, &IoLiriShellScreenCastInterface::streamReady,
            this, &Screencast::handleStreamReady);
    connect(m_screenCast, &IoLiriShellScreenCastInterface::startStreaming,
            this, &Screencast::handleStartStreaming);
    connect(m_screenCast, &IoLiriShellScreenCastInterface::stopStreaming,
            this, &Screencast::handleStopStreaming);
    m_screenCast->captureScreen(m_outputs->primaryOutput());
}

void Screencast::busMessage(const QGst::MessagePtr &message)
{
    switch (message->type()) {
    case QGst::MessageEos:
        m_pipeline->setState(QGst::StateNull);
        QCoreApplication::quit();
        break;
    case QGst::MessageInfo:
        qInfo("%s", qPrintable(message.staticCast<QGst::InfoMessage>()->error().message()));
        break;
    case QGst::MessageWarning:
        qWarning("%s", qPrintable(message.staticCast<QGst::WarningMessage>()->error().message()));
        break;
    case QGst::MessageError:
        m_pipeline->setState(QGst::StateNull);
        qCritical("%s", qPrintable(message.staticCast<QGst::ErrorMessage>()->error().message()));
        break;
    default:
        break;
    }
}

void Screencast::handleStreamReady(uint nodeId)
{
    m_nodeId = nodeId;

    // Create the pipeline
    QString pipelineDescr = QStringLiteral(
                "pipewiresrc path=%1 ! " \
                "videoconvert ! queue ! theoraenc ! oggmux ! " \
                "filesink location=\"%2\"").arg(QString::number(m_nodeId)).arg(videoFileName());
    qInfo("GStreamer pipeline: %s", qPrintable(pipelineDescr));
    m_pipeline = QGst::Parse::launch(pipelineDescr).dynamicCast<QGst::Pipeline>();

    QGlib::connect(m_pipeline->bus(), "message::info", this, &Screencast::busMessage);
    QGlib::connect(m_pipeline->bus(), "message::warning", this, &Screencast::busMessage);
    QGlib::connect(m_pipeline->bus(), "message::error", this, &Screencast::busMessage);
    m_pipeline->bus()->addSignalWatch();

    m_pipeline->setState(QGst::StatePaused);

    handleStartStreaming();
}

void Screencast::handleStartStreaming()
{
    Q_ASSERT(m_nodeId >= 0);

    m_pipeline->setState(QGst::StatePlaying);
}

void Screencast::handleStopStreaming()
{
    if (m_pipeline.isNull())
        return;

    m_pipeline->setState(QGst::StateNull);
}

#include "moc_screencast.cpp"
