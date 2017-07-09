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

#include "telepathy-mpris.h"
#include "ktp_kded_debug.h"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QTimer>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>

static const QLatin1String dbusInterfaceProperties("org.freedesktop.DBus.Properties");

static const QLatin1String mprisPath("/org/mpris/MediaPlayer2");
static const QLatin1String mprisServicePrefix("org.mpris.MediaPlayer2");
static const QLatin1String mprisInterfaceName("org.mpris.MediaPlayer2.Player");

TelepathyMPRIS::TelepathyMPRIS(QObject* parent)
    : QObject(parent),
    m_activationTimer(new QTimer()),
    m_activePlayer(new Player())
{
    connect(this, &TelepathyMPRIS::playerChange, &m_initLoop, &QEventLoop::quit);

    m_activationTimer->setSingleShot(true);
    m_activationTimer->setInterval(400);
}

TelepathyMPRIS::~TelepathyMPRIS()
{
}

void TelepathyMPRIS::enable(bool enable)
{
    if (enable && !m_timerConnection) {
        //player changed
        m_timerConnection = QObject::connect(m_activationTimer, &QTimer::timeout, m_activationTimer, [=] {
            auto getPlayers = [this] (Service state) {
                QList<Player*> players;
                for (Player *watchedPlayer : m_players.values()) {
                    if (watchedPlayer->playState == state) {
                        players << watchedPlayer;
                    }
                }

                return players;
            };

            if (m_activePlayer->playState < Paused) {
                QList<Player*> players = QList<Player*>() << getPlayers(Playing) << getPlayers(Paused);

                //if the players list is empty create a new default active player
                if (players.isEmpty()) {
                    m_activePlayer = new Player();
                } else {
                    m_activePlayer = players.at(0);
                }

                qCDebug(KTP_KDED_MODULE) << "Active player changed:" << m_players.key(m_activePlayer);
            }

            if (!m_initLoop.isRunning()) {
                Q_EMIT playerChange();
            } else {
                m_initLoop.quit();
            }
        });

        //get registered service names asynchronously
        QDBusPendingCall async = QDBusConnection::sessionBus().interface()->asyncCall(QLatin1String("ListNames"));
        QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
        connect(callWatcher, &QDBusPendingCallWatcher::finished, callWatcher, [=] {
            bool foundPlayers = false;
            QDBusPendingReply<QStringList> reply = *callWatcher;
            if (reply.isError()) {
                qCDebug(KTP_KDED_MODULE) << reply.error();
                return;
            }

            for (const QString &serviceName : reply.value()) {
                if (serviceName.startsWith(mprisServicePrefix)) {
                    requestPlaybackStatus(serviceName, QDBusConnection::sessionBus().interface()->serviceOwner(serviceName));
                    foundPlayers = true;
                }
            }

            if (!foundPlayers) {
                m_initLoop.quit();
            }

            callWatcher->deleteLater();
        });

        //watch for mpris2-enabled players appearing and disappearing
        connect(QDBusConnection::sessionBus().interface(),
                    &QDBusConnectionInterface::serviceOwnerChanged,
                    this,
                    &TelepathyMPRIS::serviceOwnerChanged);

        m_initLoop.exec();
    } else if (!enable) {
        disconnect(m_timerConnection);
        disconnect(QDBusConnection::sessionBus().interface(),
                    &QDBusConnectionInterface::serviceOwnerChanged,
                    this,
                    &TelepathyMPRIS::serviceOwnerChanged);

        QHash<QString, QString> serviceNameByOwner = m_serviceNameByOwner;
        for (const QString &serviceName : serviceNameByOwner) {
            serviceOwnerChanged(serviceName, serviceNameByOwner.key(serviceName), QString());
        }
    }
}

void TelepathyMPRIS::onPlayerSignalReceived(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties)

    //this is not the correct property interface, ignore
    if (interface != mprisInterfaceName) {
        return;
    }

    const QString &serviceName = m_serviceNameByOwner[QDBusConnection::sessionBus().interface()->serviceOwner(message().service())];
    sortPlayerReply(changedProperties, serviceName);
}

void TelepathyMPRIS::requestPlaybackStatus(const QString &serviceName, const QString &owner)
{
    auto playerConnected = [=] () {
        if (m_players.keys().contains(serviceName)) {
            return true;
        }

        bool connected = QDBusConnection::sessionBus().connect(serviceName,
                                      mprisPath,
                                      dbusInterfaceProperties,
                                      QLatin1String("PropertiesChanged"),
                                      this,
                                      SLOT(onPlayerSignalReceived(QString,QVariantMap,QStringList)));

        if (connected) {
            qCDebug(KTP_KDED_MODULE) << "Found player" << serviceName;
            Player *player = new Player();
            m_players.insert(serviceName, player);
            m_serviceNameByOwner.insert(owner, serviceName);
        }

        return connected;
    };

    QDBusMessage mprisMsg = QDBusMessage::createMethodCall(serviceName, mprisPath, dbusInterfaceProperties, QLatin1String("GetAll"));
    mprisMsg.setArguments(QList<QVariant>() << mprisInterfaceName);

    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(mprisMsg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [=] {
        QDBusPendingReply<QVariantMap> reply = *watcher;

        if (reply.isError()) {
            qCWarning(KTP_KDED_MODULE) << "Received error reply from DBus" << reply.error() << "service" << serviceName;
        } else if (playerConnected()) {
            sortPlayerReply(reply.value(), serviceName);
        }

        watcher->deleteLater();
    });
}

void TelepathyMPRIS::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    auto playerVanished = [=] (const QString &serviceName, const QString &oldOwner) {
        if (m_players.keys().contains(serviceName)) {
            QDBusConnection::sessionBus().disconnect(serviceName,
                                                     mprisPath,
                                                     dbusInterfaceProperties,
                                                     QLatin1String("PropertiesChanged"),
                                                     this,
                                                     SLOT(onPlayerSignalReceived(QString,QVariantMap,QStringList)));

            m_players[serviceName]->playState = Service::Unknown;
            if (m_players[serviceName] == m_activePlayer) {
                m_activationTimer->start();
            }
            m_players.remove(serviceName);
            m_serviceNameByOwner.remove(oldOwner);
            qCDebug(KTP_KDED_MODULE) << "Player" << serviceName << "is no longer available";
        }
    };

    if (serviceName.startsWith(mprisServicePrefix)) {
        qCDebug(KTP_KDED_MODULE) << "DBus service name change:" << serviceName << "once owned by" << oldOwner << "is now owned by" << newOwner;
        if (oldOwner.isEmpty()) {
            //if we have no oldOwner, we have new player registered at dbus
            requestPlaybackStatus(serviceName, newOwner);
        } else if (newOwner.isEmpty()) {
            //if there's no newOwner, the player quit
            playerVanished(serviceName, oldOwner);
        } else if (!oldOwner.isEmpty() && !newOwner.isEmpty()) {
            //otherwise the service name owner changed
            m_serviceNameByOwner.remove(oldOwner);
            m_serviceNameByOwner.insert(newOwner, serviceName);
            requestPlaybackStatus(serviceName, newOwner);
        }
    }
}

void TelepathyMPRIS::sortPlayerReply(const QVariantMap &serviceInfo, const QString &serviceName)
{
    bool playerChanged = false;

    //sort and store incoming metadata
    if (serviceInfo.keys().contains(QLatin1String("Metadata"))) {
        QVariantMap metadata = qdbus_cast<QVariantMap>(serviceInfo.value(QLatin1String("Metadata")));
        if (m_players[serviceName]->metadata != metadata) {
            m_players[serviceName]->metadata = metadata;
            playerChanged = true;
        }
    }

    //sort and store incoming playback information
    if (serviceInfo.keys().contains(QLatin1String("PlaybackStatus"))) {
        QVariant playbackStatus = serviceInfo.value(QLatin1String("PlaybackStatus"));
        auto playState = [] (const QVariant &playbackStatus) {
            TelepathyMPRIS::Service playState;
            if (playbackStatus == QLatin1String("Playing")) {
                playState = Service::Playing;
            } else if (playbackStatus == QLatin1String("Paused")) {
                playState = Service::Paused;
            } else if (playbackStatus == QLatin1String("Stopped")) {
                playState = Service::Stopped;
            } else {
                playState = Service::Unknown;
            }

            return playState;
        };

        if (m_players[serviceName]->playState != playState(playbackStatus)) {
            if ((m_players[serviceName]->playState == Unknown) && (playState(playbackStatus) < Playing)) {
                playerChanged = false;
            } else {
                playerChanged = true;
            }

            m_players[serviceName]->playState = playState(serviceInfo.value(QLatin1String("PlaybackStatus")));
        }
    }

    qCDebug(KTP_KDED_MODULE) << "Player" << serviceName << m_players[serviceName]->playState;

    if (playerChanged || m_initLoop.isRunning()) {
        m_activationTimer->start();
    }
}
