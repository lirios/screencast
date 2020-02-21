/****************************************************************************
 * SPDX-FileCopyrightText: 2020 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/

#ifndef SCREENCAST_H
#define SCREENCAST_H

#include <QEvent>
#include <QObject>

class Screencast : public QObject
{
    Q_OBJECT
public:
    explicit Screencast(QObject *parent = nullptr);
    ~Screencast();

protected:
    bool event(QEvent *event) override;

private:
    bool m_initialized = false;

    QString videoFileName() const;

    void initialize();

private Q_SLOTS:
    void handleStreamReady(uint nodeId);
};

class StartupEvent : public QEvent
{
public:
    StartupEvent();
};

#endif // SCREENCAST_H
