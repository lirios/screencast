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
