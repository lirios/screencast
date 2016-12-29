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
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QScreen>

#include <Liri/WaylandClient/Output>

#include <Qt5GStreamer/QGlib/Connect>
#include <Qt5GStreamer/QGst/Buffer>
#include <Qt5GStreamer/QGst/Bus>
#include <Qt5GStreamer/QGst/Init>
#include <Qt5GStreamer/QGst/Message>
#include <Qt5GStreamer/QGst/Parse>
#include <Qt5GStreamer/QGst/Sample>

#include "screencast.h"

/*
 * StartupEvent
 */

static const QEvent::Type StartupEventType =
        static_cast<QEvent::Type>(QEvent::registerEventType());

StartupEvent::StartupEvent()
    : QEvent(StartupEventType)
{
}

AppSource::AppSource()
    : QGst::Utils::ApplicationSource()
    , m_stop(false)
{
}

bool AppSource::isStopped() const
{
    return m_stop;
}

void AppSource::needData(uint length)
{
    Q_UNUSED(length);

    m_stop = false;
}

void AppSource::enoughData()
{
    m_stop = true;
}

/*
 * Screencast
 */

Screencast::Screencast(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
    , m_inProgress(false)
    , m_thread(new QThread())
    , m_connection(WaylandClient::ClientConnection::fromQt())
    , m_registry(new WaylandClient::Registry(this))
    , m_shm(Q_NULLPTR)
    , m_screencaster(Q_NULLPTR)
    , m_size(QSize())
    , m_stride(0)
{
    // Wayland connection in a separate thread
    Q_ASSERT(m_connection);
    m_connection->moveToThread(m_thread);
    m_thread->start();
}

Screencast::~Screencast()
{
    if (!m_pipeline.isNull()) {
        m_pipeline->setState(QGst::StateNull);
        m_pipeline.clear();
    }

    delete m_screencaster;
    delete m_shm;
    delete m_registry;
    delete m_connection;

    m_thread->quit();
    m_thread->wait();
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

    // Interfaces
    connect(m_registry, &WaylandClient::Registry::interfacesAnnounced,
            this, &Screencast::interfacesAnnounced);
    connect(m_registry, &WaylandClient::Registry::interfaceAnnounced,
            this, &Screencast::interfaceAnnounced);

    // Setup registry
    m_registry->create(m_connection->display());
    m_registry->setup();

    m_initialized = true;
}

void Screencast::process()
{
    // Do not create the pipeline twice
    if (!m_pipeline.isNull())
        return;

    // Setup appsrc
    m_appSource.setSize(-1);
    m_appSource.setStreamType(QGst::AppStreamTypeStream);
    m_appSource.setFormat(QGst::FormatTime);
    m_appSource.enableBlock(false);
    m_appSource.setLatency(-1, -1);
    m_appSource.setMaxBytes(INT_MAX);

    // Set appsrc caps
    QString capsString = QStringLiteral("video/x-raw,format=RGB,width=%1,height=%2,depth=32,bpp=24,framerate=0/1")
            .arg(m_size.width()).arg(m_size.height());
    QGst::CapsPtr caps = QGst::Caps::fromString(qPrintable(capsString));
    m_appSource.setCaps(caps);

    // Create the pipeline
    QString pipelineDescr = QStringLiteral("appsrc name=\"screencap\" is-live=true ! " \
                                           "videoconvert ! queue ! theoraenc ! oggmux ! " \
                                           "filesink location=\"%1\"").arg(videoFileName());
    pipelineDescr = QStringLiteral("appsrc name=\"screencap\" is-live=true ! videoconvert ! vp9enc min_quantizer=13 max_quantizer=13 cpu-used=5 deadline=1000000 threads=2 ! queue ! webmmux ! filesink location=pippo");
    pipelineDescr = QStringLiteral("appsrc name=\"screencap\" ! videoconvert ! queue ! theoraenc ! oggmux ! filesink location=pippo");
    qInfo("GStreamer pipeline: %s", qPrintable(pipelineDescr));
    m_pipeline = QGst::Parse::launch(pipelineDescr).dynamicCast<QGst::Pipeline>();
    m_appSource.setElement(m_pipeline->getElementByName("screencap"));

    QGlib::connect(m_pipeline->bus(), "message::info", this, &Screencast::busMessage);
    QGlib::connect(m_pipeline->bus(), "message::warning", this, &Screencast::busMessage);
    QGlib::connect(m_pipeline->bus(), "message::error", this, &Screencast::busMessage);
    m_pipeline->bus()->addSignalWatch();

    m_pipeline->setState(QGst::StatePlaying);
}

void Screencast::start()
{
    if (m_inProgress) {
        qWarning("Cannot take another screenshot while a previous capture is in progress");
        return;
    }

    Q_FOREACH (QScreen *screen, QGuiApplication::screens()) {
        WaylandClient::Screencast *screencast = m_screencaster->capture(WaylandClient::Output::fromQt(screen, this));
        connect(screencast, &WaylandClient::Screencast::setupDone, this,
                [this](const QSize &size, qint32 stride) {
            m_size = size;
            m_stride = stride;
        });
        connect(screencast, &WaylandClient::Screencast::setupFailed, this, [this] {
            qCritical("Frame buffer setup failed, aborting...");
            QCoreApplication::quit();
        });
        connect(screencast, &WaylandClient::Screencast::frameRecorded, this,
                [this](WaylandClient::Buffer *buffer, quint32 time, WaylandClient::Screencast::Transform transform) {
            Q_UNUSED(time);

            process();

            QImage image = buffer->image();
            if (transform == WaylandClient::Screencast::TransformYInverted)
                image = image.mirrored(false, true);

            QGst::BufferPtr gstBuffer = QGst::Buffer::create(image.byteCount());
            QGst::BufferFlags flags(QGst::BufferFlagLive);
            gstBuffer->setFlags(flags);

            //GST_BUFFER_PTS(buffer) = now;

            QGst::MapInfo mapInfo;
            if (gstBuffer->map(mapInfo, QGst::MapWrite)) {
                memcpy(mapInfo.data(), image.bits(), mapInfo.size());
                gstBuffer->unmap(mapInfo);
            } else {
                qWarning("Failed to map frame buffer, skipping a frame");
                return;
            }

            if (!m_appSource.isStopped()) {
                QGst::FlowReturn ret = m_appSource.pushBuffer(gstBuffer);
                switch (ret) {
                case QGst::FlowEos:
                    qWarning("Failed to push buffer: end of stream");
                    break;
                case QGst::FlowNotLinked:
                    qWarning("Failed to push buffer: not linked");
                    break;
                case QGst::FlowFlushing:
                    qWarning("Failed to push buffer: flushing");
                    break;
                case QGst::FlowNotNegotiated:
                    qWarning("Failed to push buffer: not negotiated");
                    break;
                case QGst::FlowNotSupported:
                    qWarning("Failed to push buffer: not supported");
                    break;
                case QGst::FlowError:
                    qWarning("Failed to push buffer: generic error");
                    break;
                case QGst::FlowOk:
                    break;
                default:
                    qWarning("Failed to push buffer: unknown error");
                    break;
                }
            }
        });
    }

    m_inProgress = true;
}

void Screencast::interfacesAnnounced()
{
    if (!m_shm)
        qCritical("Unable to create shared memory buffers");

    if (!m_screencaster)
        qCritical("Wayland compositor doesn't have screenshooter capabilities");

    start();
}

void Screencast::interfaceAnnounced(const QByteArray &interface,
                                    quint32 name, quint32 version)
{
    if (interface == WaylandClient::Shm::interfaceName())
        m_shm = m_registry->createShm(name, version);
    else if (interface == WaylandClient::Screencaster::interfaceName())
        m_screencaster = m_registry->createScreencaster(m_shm, name, version);
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

#include "moc_screencast.cpp"
