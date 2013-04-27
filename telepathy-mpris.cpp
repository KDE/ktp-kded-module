/*
    Now playing... presence plugin
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

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariant>

#include <KDebug>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

#include <TelepathyQt/AccountSet>

#include <KTp/global-presence.h>

TelepathyMPRIS::TelepathyMPRIS(KTp::GlobalPresence* globalPresence, QObject* parent)
    : TelepathyKDEDModulePlugin(globalPresence, parent),
      m_presenceActivated(false)
{
    //read settings and detect players if plugin is enabled
    onSettingsChanged();

    //watch for new mpris-enabled players
    connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));

    QDBusConnection::sessionBus().connect(QString(), QLatin1String("/Telepathy"), QLatin1String("org.kde.Telepathy"),
                                          QLatin1String("activateNowPlaying"), this, SLOT(onActivateNowPlaying()) );

    QDBusConnection::sessionBus().connect(QString(), QLatin1String("/Telepathy"), QLatin1String("org.kde.Telepathy"),
                                          QLatin1String("deactivateNowPlaying"), this, SLOT(onDeactivateNowPlaying()) );
}

TelepathyMPRIS::~TelepathyMPRIS()
{
}

QString TelepathyMPRIS::pluginName() const
{
    return QString::fromLatin1("telepathy-mpris");
}

void TelepathyMPRIS::onPlayerSignalReceived(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(interface)
    Q_UNUSED(invalidatedProperties)

    //if the plugin is disabled, no point in parsing the received signal
    if (!isEnabled()) {
        return;
    }

    //lookup if the PlaybackStatus was changed
    if (changedProperties.keys().contains(QLatin1String("PlaybackStatus"))) {
        if (changedProperties.value(QLatin1String("PlaybackStatus")) == QLatin1String("Playing")) {
            m_playbackActive = true;
            setActive(true);
            setTrackToPresence(m_lastReceivedMetadata);
        } else {
            //if the player is stopped or paused, deactivate and return to normal presence
            m_playbackActive = false;
            m_lastReceivedMetadata.clear();
            setActive(false);
            return;
        }
    }

    //track data change
    if (changedProperties.keys().contains(QLatin1String("Metadata"))) {
        m_lastReceivedMetadata = qdbus_cast<QMap<QString, QVariant> >(changedProperties.value(QLatin1String("Metadata")));
        if (m_playbackActive) {
            setTrackToPresence(m_lastReceivedMetadata);
        }
    }
}

void TelepathyMPRIS::detectPlayers()
{
    //get registered service names asynchronously
    QDBusPendingCall async = QDBusConnection::sessionBus().interface()->asyncCall(QLatin1String("ListNames"));
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(serviceNameFetchFinished(QDBusPendingCallWatcher*)));
}

void TelepathyMPRIS::serviceNameFetchFinished(QDBusPendingCallWatcher *callWatcher)
{
    QDBusPendingReply<QStringList> reply = *callWatcher;
    if (reply.isError()) {
        kDebug() << reply.error();
        return;
    }

    callWatcher->deleteLater();

    QStringList mprisServices = reply.value();
    QStringList players;

    Q_FOREACH (const QString &service, mprisServices) {
        if (!service.contains(QLatin1String("org.mpris.MediaPlayer2"))) {
            continue;
        }
        newMediaPlayer(service);
        players.append(service);
    }

    //this gets rid of removed services and stores only those currently present
    m_knownPlayers = players;

    if (m_knownPlayers.isEmpty() && isActive()) {
        kDebug() << "Received empty players list while active, deactivating (player quit)";
        setActive(false);
    }
}

void TelepathyMPRIS::onSettingsChanged()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    config.data()->reparseConfiguration();

    KConfigGroup kdedConfig = config->group("KDED");

    bool pluginEnabled = kdedConfig.readEntry("nowPlayingEnabled", false);

    //if the plugin was enabled and is now disabled
    if (isEnabled() && !pluginEnabled) {
        setEnabled(false);
        return;
    }

    //if the plugin was disabled and is now enabled
    if (!isEnabled() && pluginEnabled) {
        setEnabled(true);
        m_nowPlayingText = kdedConfig.readEntry(QLatin1String("nowPlayingText"),
                                                  i18nc("The default text displayed by now playing plugin. "
                                                        "track title: %1, artist: %2, album: %3",
                                                        "Now listening to %1 by %2 from album %3",
                                                        QLatin1String("%title"), QLatin1String("%artist"), QLatin1String("%album")));
        detectPlayers();
    }
}

void TelepathyMPRIS::newMediaPlayer(const QString &service)
{
    kDebug() << "Found mpris service:" << service;
    QDBusInterface mprisInterface(service,
                                  QLatin1String("/org/mpris/MediaPlayer2"),
                                  QLatin1String("org.freedesktop.DBus.Properties"));

    QDBusPendingCall call = mprisInterface.asyncCall(QLatin1String("GetAll"),
                                                     QLatin1String("org.mpris.MediaPlayer2.Player"));

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(onPlaybackStatusReceived(QDBusPendingCallWatcher*)));

    //check if we are already watching this service
    if (!m_knownPlayers.contains(service)) {
        QDBusConnection::sessionBus().connect(service,
                                              QLatin1String("/org/mpris/MediaPlayer2"),
                                              QLatin1String("org.freedesktop.DBus.Properties"),
                                              QLatin1String("PropertiesChanged"),
                                              this,
                                              SLOT(onPlayerSignalReceived(QString,QVariantMap,QStringList)) );
    }
}

void TelepathyMPRIS::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner)

    if (serviceName.contains(QLatin1String("org.mpris.MediaPlayer2"))) {
        if (!newOwner.isEmpty()) {
            //if we have newOwner, we have new player registered at dbus
            kDebug() << "New player appeared on dbus, connecting...";
            newMediaPlayer(serviceName);
        } else if (newOwner.isEmpty()) {
            //if there's no owner, the player quit, look if there are any other players
            kDebug() << "Player disappeared from dbus, looking for other players...";
            detectPlayers();
        }
    }
}

void TelepathyMPRIS::onActivateNowPlaying()
{
    kDebug() << "Plugin activated";
    m_presenceActivated = true;
    setEnabled(true);
    if( !m_knownPlayers.isEmpty() )
        detectPlayers();
}

void TelepathyMPRIS::onDeactivateNowPlaying()
{
    kDebug() << "Plugin deactivated on contact list request";

    if (m_presenceActivated) {
        m_presenceActivated = false;
        setActive(false);
        setEnabled(false);
        KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
        KConfigGroup kdedConfig = config->group("KDED");
        kdedConfig.writeEntry("nowPlayingEnabled", false);
        kdedConfig.sync();
    }
}

void TelepathyMPRIS::onPlaybackStatusReceived(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        kWarning() << "Received error reply from DBus" << reply.error();
    } else {
        QVariantMap replyData = reply.value();
        if (replyData.value(QLatin1String("PlaybackStatus")).toString() == QLatin1String("Playing")) {
            setTrackToPresence(qdbus_cast<QMap<QString, QVariant> >(replyData.value(QLatin1String("Metadata")).value<QDBusArgument>()));
        }
    }

    watcher->deleteLater();
}

void TelepathyMPRIS::setTrackToPresence(const QMap<QString, QVariant> &trackData)
{
    if (trackData.isEmpty()) {
        return;
    }

    QString artist = trackData.value(QLatin1String("xesam:artist")).toString();
    QString title = trackData.value(QLatin1String("xesam:title")).toString();
    QString album = trackData.value(QLatin1String("xesam:album")).toString();
    QString trackNumber = trackData.value(QLatin1String("xesam:trackNumber")).toString();

    //we replace track's info in custom nowPlayingText
    QString statusMessage = m_nowPlayingText;
    statusMessage.replace(QLatin1String("%title"), title, Qt::CaseInsensitive);
    statusMessage.replace(QLatin1String("%artist"), artist, Qt::CaseInsensitive);
    statusMessage.replace(QLatin1String("%album"), album, Qt::CaseInsensitive);
    statusMessage.replace(QLatin1String("%track"), trackNumber, Qt::CaseInsensitive);

    Tp::Presence currentPresence = m_globalPresence->currentPresence();
    Tp::SimplePresence presence;

    presence.type = currentPresence.type();
    presence.status = currentPresence.status();
    presence.statusMessage = statusMessage;

    setRequestedPresence(Tp::Presence(presence));
    if (m_presenceActivated) {
        setActive(true);
    }
}
