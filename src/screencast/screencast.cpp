/****************************************************************************
 * SPDX-FileCopyrightText: 2020 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/

#include <QCoreApplication>
#include <QDateTime>
#include <QDBusArgument>
#include <QPoint>
#include <QSize>
#include <QStandardPaths>

#include "portal.h"
#include "screencast.h"
#include "sigwatch.h"

#include <gst/gst.h>

Q_LOGGING_CATEGORY(lcScreencast, "liri.screencast")

static gboolean bus_watch_cb(GstBus *bus, GstMessage *msg, gpointer user_data)
{
    Stream *stream = static_cast<Stream *>(user_data);
    Q_ASSERT(stream);

    g_autoptr (GError) err = nullptr;
    gchar *debug_info = nullptr;

    switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_INFO:
        gst_message_parse_error(msg, &err, &debug_info);
        qCInfo(lcScreencast, "Message received from element %s: %s", GST_OBJECT_NAME(msg->src), err->message);
        g_clear_error(&err);
        break;
    case GST_MESSAGE_WARNING:
        gst_message_parse_error(msg, &err, &debug_info);
        qCWarning(lcScreencast, "Warning received from element %s: %s", GST_OBJECT_NAME(msg->src), err->message);
        g_clear_error(&err);
        break;
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(msg, &err, &debug_info);
        qCWarning(lcScreencast, "Error received from element %s: %s", GST_OBJECT_NAME(msg->src), err->message);
        if (debug_info)
            qCWarning(lcScreencast, "Debugging information: %s", debug_info);
        g_clear_error(&err);
        g_free(debug_info);
        gst_bus_remove_watch(bus);
        delete stream;
        break;
    case GST_MESSAGE_EOS:
        qCInfo(lcScreencast, "End of stream reached");
        gst_bus_remove_watch(bus);
        delete stream;
        break;
    case GST_MESSAGE_STATE_CHANGED:
        // We are only interested in state-changed messages from the pipeline
        if (GST_MESSAGE_SRC(msg) == GST_OBJECT(stream->pipeline)) {
            GstState oldState, newState, pendingState;
            gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
            qCInfo(lcScreencast,
                   "Pipeline state changed from %s to %s",
                  gst_element_state_get_name(oldState),
                  gst_element_state_get_name(newState));
        }
        break;
    default:
        break;
    }

    return true;
}

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
    , m_portal(new Portal(this))
{
    connect(m_portal, &Portal::streamReady, this, &Screencast::handleStreamReady);
    connect(m_portal, &Portal::sessionClosed, this, &Screencast::handleSessionClosed);

    auto *sigwatch = new UnixSignalWatcher(this);
    sigwatch->watchForSignal(SIGINT);
    sigwatch->watchForSignal(SIGTERM);
    connect(sigwatch, &UnixSignalWatcher::unixSignal, this, &Screencast::shutdown);
}

Screencast::~Screencast()
{
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

    m_portal->createSession();
}

void Screencast::handleStreamReady(int fd, uint nodeId, const QVariantMap &map)
{
    // Position
    int x = 0, y = 0;
    QDBusArgument dbusPosition = map[QStringLiteral("position")].value<QDBusArgument>();
    dbusPosition.beginArray();
    dbusPosition >> x >> y;
    dbusPosition.endArray();

    // Size
    int w = 0, h = 0;
    QDBusArgument dbusSize = map[QStringLiteral("size")].value<QDBusArgument>();
    dbusSize.beginArray();
    dbusSize >> w >> h;
    dbusSize.endArray();

    // Format
    QString format = QStringLiteral("bgrx");
    if (map.contains(QStringLiteral("format"))) {
        QDBusArgument dbusFormat = map[QStringLiteral("format")].value<QDBusArgument>();
        dbusFormat >> format;
    }

    qCInfo(lcScreencast, "Stream %d", nodeId);
    qCInfo(lcScreencast, "Position %d, %d", x, y);
    qCInfo(lcScreencast, "Size %dx%d", w, h);

    // Create the pipeline
    QString launch = QStringLiteral("pipewiresrc fd=%1 path=%2 ! " \
                                    "rawvideoparse width=%3 height=%4 format=%5 framerate=1 ! " \
                                    "autovideoconvert ! queue ! theoraenc ! oggmux ! " \
                                    "filesink location=\"%6\"").arg(fd).arg(nodeId).arg(w).arg(h).arg(format).arg(videoFileName());
    GstElement *pipeline = gst_parse_launch(launch.toUtf8(), nullptr);
    GstBus *bus = gst_element_get_bus(pipeline);

    Stream *stream = new Stream();
    stream->screencast = this;
    stream->pipeline = pipeline;
    m_streams.append(stream);
    gst_bus_add_watch(bus, bus_watch_cb, stream);

    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qCWarning(lcScreencast, "Unable to set the pipeline to the playing state");
        gst_bus_remove_watch(bus);
        delete stream;
    }
}

void Screencast::handleSessionClosed(const QVariantMap &map)
{
    Q_UNUSED(map)

    // TODO: Not clear what map contains, anyway we should kill the stream,
    // but which one?
}

void Screencast::shutdown()
{
    qDeleteAll(m_streams);
    QCoreApplication::quit();
}

Stream::~Stream()
{
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    if (screencast) {
        screencast->m_streams.removeOne(this);
        screencast = nullptr;
    }
}
