/****************************************************************************
 * SPDX-FileCopyrightText: 2020 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/

#ifndef SCREENCAST_H
#define SCREENCAST_H

#include <QEvent>
#include <QLoggingCategory>
#include <QObject>

#include <gst/gstelement.h>

Q_DECLARE_LOGGING_CATEGORY(lcScreencast)

class Portal;
class Stream;

class Screencast : public QObject
{
    Q_OBJECT
public:
    explicit Screencast(QObject *parent = nullptr);
    ~Screencast();

protected:
    bool event(QEvent *event) override;

private:
    friend class Stream;

    bool m_initialized = false;
    Portal *m_portal = nullptr;
    QVector<Stream *> m_streams;

    QString videoFileName() const;

    void initialize();

private Q_SLOTS:
    void handleStreamReady(int fd, uint nodeId, const QVariantMap &map);
    void handleSessionClosed(const QVariantMap &map);
    void shutdown();
};

class Stream
{
public:
    Stream() = default;
    ~Stream();

    Screencast *screencast = nullptr;
    GstElement *pipeline = nullptr;
};

class StartupEvent : public QEvent
{
public:
    StartupEvent();
};

#endif // SCREENCAST_H
