/****************************************************************************
 * SPDX-FileCopyrightText: 2020 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>

#include "portal.h"
#include "screencast.h"

Q_DECLARE_METATYPE(Portal::Stream)
Q_DECLARE_METATYPE(Portal::Streams)

const QDBusArgument &operator >> (const QDBusArgument &arg, Portal::Stream &stream)
{
    arg.beginStructure();
    arg >> stream.nodeId;

    arg.beginMap();
    while (!arg.atEnd()) {
        QString key;
        QVariant map;
        arg.beginMapEntry();
        arg >> key >> map;
        arg.endMapEntry();
        stream.map.insert(key, map);
    }
    arg.endMap();
    arg.endStructure();

    return arg;
}

Portal::Portal(QObject *parent)
    : QObject(parent)
{
}

void Portal::createSession()
{
    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.portal.ScreenCast"),
                                              QStringLiteral("CreateSession"));

    msg << QVariantMap { { QStringLiteral("handle_token"), newRequestToken() },
                         { QStringLiteral("session_handle_token"), newSessionToken() } };

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(
        watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QDBusObjectPath> reply = *self;
            self->deleteLater();

            if (!reply.isValid()) {
                qCWarning(lcScreencast, "Failed to create session: %s",
                          qPrintable(reply.error().message()));
                return;
            }

            QDBusConnection::sessionBus().connect(
                QString(), reply.value().path(), QStringLiteral("org.freedesktop.portal.Request"),
                QStringLiteral("Response"), this, SLOT(createSessionHandler(uint, QVariantMap)));
        });
}

QString Portal::newRequestToken() const
{
    static quint32 count = 0;
    return QStringLiteral("u") + QString::number(++count);
}

QString Portal::newSessionToken() const
{
    static quint32 count = 0;
    return QStringLiteral("u") + QString::number(++count);
}

void Portal::createSessionHandler(uint response, const QVariantMap &results)
{
    if (response != 0) {
        qCWarning(lcScreencast, "Create session failed with code %d", response);
        return;
    }

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.portal.ScreenCast"),
                                              QStringLiteral("SelectSources"));
    m_sessionHandle = QDBusObjectPath(results.value(QStringLiteral("session_handle")).toString());

    QDBusConnection::sessionBus().connect(
        QString(), m_sessionHandle.path(), QStringLiteral("org.freedesktop.portal.Session"),
        QStringLiteral("Closed"), this, SIGNAL(sessionClosed(QVariantMap)));

    msg << QVariant::fromValue(m_sessionHandle)
        << QVariantMap { { QStringLiteral("multiple"), false },
                         { QStringLiteral("types"), uint(Monitor) },
                         { QStringLiteral("handle_token"), newRequestToken() } };

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(
        watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QDBusObjectPath> reply = *self;
            self->deleteLater();

            if (!reply.isValid()) {
                qCWarning(lcScreencast, "Failed to select sources: %s",
                          qPrintable(reply.error().message()));
                return;
            }

            QDBusConnection::sessionBus().connect(
                QString(), reply.value().path(), QStringLiteral("org.freedesktop.portal.Request"),
                QStringLiteral("Response"), this, SLOT(selectSourcesHandler(uint, QVariantMap)));
        });
}

void Portal::selectSourcesHandler(uint response, const QVariantMap &results)
{
    if (response != 0) {
        qCWarning(lcScreencast, "Select sources failed with code %d", response);
        return;
    }

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                              QStringLiteral("/org/freedesktop/portal/desktop"),
                                              QStringLiteral("org.freedesktop.portal.ScreenCast"),
                                              QStringLiteral("Start"));

    msg << QVariant::fromValue(m_sessionHandle) << QString() // parent_window
        << QVariantMap { { QStringLiteral("handle_token"), newRequestToken() } };

    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
    connect(
        watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
            QDBusPendingReply<QDBusObjectPath> reply = *self;
            self->deleteLater();

            if (!reply.isValid()) {
                qCWarning(lcScreencast, "Failed to start: %s", qPrintable(reply.error().message()));
                return;
            }

            QDBusConnection::sessionBus().connect(
                QString(), reply.value().path(), QStringLiteral("org.freedesktop.portal.Request"),
                QStringLiteral("Response"), this, SLOT(startHandler(uint, QVariantMap)));
        });
}

void Portal::startHandler(uint response, const QVariantMap &results)
{
    if (response != 0) {
        qCWarning(lcScreencast, "Start failed with code %d", response);
        return;
    }

    Streams streams = qdbus_cast<Streams>(results.value(QStringLiteral("streams")));
    for (auto stream : qAsConst(streams)) {
        auto msg =
            QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.portal.Desktop"),
                                           QStringLiteral("/org/freedesktop/portal/desktop"),
                                           QStringLiteral("org.freedesktop.portal.ScreenCast"),
                                           QStringLiteral("OpenPipeWireRemote"));
        msg << QVariant::fromValue(m_sessionHandle) << QVariantMap();

        QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(msg);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingCall);
        connect(watcher, &QDBusPendingCallWatcher::finished, this,
                [this, stream](QDBusPendingCallWatcher *self) {
                    QDBusPendingReply<QDBusUnixFileDescriptor> reply = *self;
                    self->deleteLater();

                    if (!reply.isValid()) {
                        qCWarning(lcScreencast, "Failed to open pipewire remote: %s",
                                  qPrintable(reply.error().message()));
                        return;
                    }

                    emit streamReady(reply.value().fileDescriptor(), stream.nodeId, stream.map);
                });
    }
}
