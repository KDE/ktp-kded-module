/*
    Now playing
    Copyright (C) 2017  James D. Smith <smithjd15@gmail.com>
    Copyright (C) 2011  Martin Klapetek <martin.klapetek@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef TELEPATHY_MPRIS_H
#define TELEPATHY_MPRIS_H

#include <KTp/types.h>

#include <QTimer>

class TelepathyMPRIS : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    TelepathyMPRIS(QObject *parent = 0);
    ~TelepathyMPRIS();

    enum Service {
        Unknown,
        Stopped,
        Paused,
        Playing
    };
    Q_ENUM(Service)

    struct Player {
        Service playState = TelepathyMPRIS::Unknown;
        QVariantMap metadata;
    };

    void enable(bool enable);

    Player* player() {return m_activePlayer;}

Q_SIGNALS:
    void playerChange();

private Q_SLOTS:
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void onPlayerSignalReceived(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    void requestPlaybackStatus(const QString &serviceName, const QString &owner);
    void sortPlayerReply(const QVariantMap &serviceInfo, const QString &serviceName);

    QMetaObject::Connection m_timerConnection;
    QTimer *m_activationTimer;
    QEventLoop m_initLoop;
    QHash<QString, Player*> m_players;
    QHash<QString, QString> m_serviceNameByOwner;
    Player* m_activePlayer;
};

#endif // TELEPATHY_MPRIS_H
