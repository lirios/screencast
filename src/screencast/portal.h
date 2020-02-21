/****************************************************************************
 * SPDX-FileCopyrightText: 2020 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/

#ifndef PORTAL_H
#define PORTAL_H

#include <QDBusObjectPath>
#include <QObject>
#include <QVariantMap>

class Portal : public QObject
{
    Q_OBJECT
public:
    enum AvailableSourceTypes {
        Monitor = 1,
        Window = 2
    };
    Q_ENUM(AvailableSourceTypes)

    enum AvailableCursorModes {
        Hidden = 1,
        Embedded = 2,
        Metadata = 4
    };
    Q_ENUM(AvailableCursorModes)

    typedef struct {
        uint nodeId;
        QVariantMap map;
    } Stream;
    typedef QList<Stream> Streams;

    explicit Portal(QObject *parent = nullptr);

    void createSession();

signals:
    void streamReady(int fd, uint nodeId, const QVariantMap &map);
    void sessionClosed(const QVariantMap &map);

private:
    QDBusObjectPath m_sessionHandle;

    QString newRequestToken() const;
    QString newSessionToken() const;

private slots:
    void createSessionHandler(uint response, const QVariantMap &results);
    void selectSourcesHandler(uint response, const QVariantMap &results);
    void startHandler(uint response, const QVariantMap &results);
};

#endif // PORTAL_H
