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

#ifndef SCREENCAP_H
#define SCREENCAP_H

#include <QtCore/QObject>
#include <QtCore/QThread>

#include <GreenIsland/Client/ClientConnection>
#include <GreenIsland/Client/Registry>
#include <GreenIsland/Client/Screencaster>
#include <GreenIsland/Client/Shm>

#include <Qt5GStreamer/QGst/Pipeline>
#include <Qt5GStreamer/QGst/Utils/ApplicationSource>

using namespace GreenIsland;

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

class Screencap : public QObject
{
    Q_OBJECT
public:
    Screencap(QObject *parent = Q_NULLPTR);
    ~Screencap();

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;

private:
    bool m_initialized;
    bool m_inProgress;
    QThread *m_thread;
    Client::ClientConnection *m_connection;
    Client::Registry *m_registry;
    Client::Shm *m_shm;
    Client::Screencaster *m_screencaster;

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

#endif // SCREENCAP_H
