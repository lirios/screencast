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

#ifndef SCREENCAST_H
#define SCREENCAST_H

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <Liri/WaylandClient/ClientConnection>
#include <Liri/WaylandClient/Registry>
#include <Liri/WaylandClient/Screencaster>
#include <Liri/WaylandClient/Shm>

#include <Qt5GStreamer/QGst/Pipeline>
#include <Qt5GStreamer/QGst/Utils/ApplicationSource>

using namespace Liri;

class AppSource : public QGst::Utils::ApplicationSource
{
public:
    AppSource();

    bool isStopped() const;

protected:
    void needData(uint length) Q_DECL_OVERRIDE;
    void enoughData() Q_DECL_OVERRIDE;

private:
    bool m_stop;
};

class Screencast : public QObject
{
    Q_OBJECT
public:
    Screencast(QObject *parent = Q_NULLPTR);
    ~Screencast();

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;

private:
    bool m_initialized;
    bool m_inProgress;
    QThread *m_thread;
    WaylandClient::ClientConnection *m_connection;
    WaylandClient::Registry *m_registry;
    WaylandClient::Shm *m_shm;
    WaylandClient::Screencaster *m_screencaster;

    struct {
        bool initialized;
        quint32 name;
        quint32 version;
    } m_deferredScreencaster;

    QSize m_size;
    qint32 m_stride;

    QGst::PipelinePtr m_pipeline;
    AppSource m_appSource;

    QString videoFileName() const;

    void initialize();
    void process();
    void start();

private Q_SLOTS:
    void interfacesAnnounced();
    void interfaceAnnounced(const QByteArray &interface, quint32 name, quint32 version);
    void busMessage(const QGst::MessagePtr &message);
};

class StartupEvent : public QEvent
{
public:
    StartupEvent();
};

#endif // SCREENCAST_H
