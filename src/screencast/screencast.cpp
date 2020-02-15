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

#include "screencast.h"

#include <gst/gst.h>

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
}

void Screencast::handleStreamReady(uint nodeId)
{
    // Create the pipeline and the elements
    GstElement *source = gst_element_factory_make("pipewiresrc", "source");
    GstElement *convert = gst_element_factory_make("videoconvert", "convert");
    GstElement *queue = gst_element_factory_make("queue", "queue");
    GstElement *encode = gst_element_factory_make("theoraenc", "encode");
    GstElement *mux = gst_element_factory_make("oggmux", "mux");
    GstElement *sink = gst_element_factory_make("filesink", "sink");
    GstElement *pipeline = gst_pipeline_new("record");
    if (!pipeline || !source || !convert || !queue || !encode || !mux || !sink) {
        qWarning("Not all elements could be created");
        return;
    }

    // Build the pipeline (the source will be linked later)
    gst_bin_add_many(GST_BIN(pipeline), source, convert, queue, encode, mux, sink, nullptr);
    if (!gst_element_link_many(convert, queue, encode, mux, sink, nullptr)) {
        qWarning("Elements could not be linked");
        gst_object_unref(pipeline);
        return;
    }

    // Set node ID and sink location
    g_object_set(source, "path", nodeId, nullptr);
    g_object_set(sink, "location", qPrintable(videoFileName()), nullptr);

    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qWarning("Unable to set the pipeline to the playing state");
        gst_object_unref(pipeline);
        return;
    }

    // Listen to the bus
    bool terminate = false;
    GstBus *bus = gst_element_get_bus(pipeline);
    do {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                                     GstMessageType(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if (!msg)
            continue;

        GError *err = nullptr;
        gchar *debug_info = nullptr;

        switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &err, &debug_info);
            qWarning("Error received from element %s: %s", GST_OBJECT_NAME(msg->src), err->message);
            qWarning("Debugging information: %s", debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            terminate = true;
            break;
        case GST_MESSAGE_EOS:
            qInfo("End of stream reached");
            terminate = true;
            break;
        case GST_MESSAGE_STATE_CHANGED:
            // We are only interested in state-changed messages from the pipeline
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pipeline)) {
                GstState oldState, newState, pendingState;
                gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
                qInfo("Pipeline state changed from %s to %s",
                      gst_element_state_get_name(oldState),
                      gst_element_state_get_name(newState));
            }
            break;
        default:
            qCritical("Unexpected message received");
            break;
        }

        gst_message_unref(msg);
    } while (!terminate);

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}
