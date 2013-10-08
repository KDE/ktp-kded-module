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
      m_enabledInConfig(false),
      m_playbackActive(false)
{
    //read settings and detect players if plugin is enabled
    onSettingsChanged();

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
    //if the plugin is disabled, no point in parsing the received signal
    if (!isEnabled()) {
        return;
    }

    // this is not the correct property interface, ignore
    if (interface != QLatin1String("org.mpris.MediaPlayer2.Player")) {
        return;
    }

    // PropertiesChanged and GetAll share the same signature, reuse it here
    setPlaybackStatus(changedProperties);

    // if some specific implementation may not notify the new value, request it manually
    if (invalidatedProperties.contains(QLatin1String("PlaybackStatus")) || invalidatedProperties.contains(QLatin1String("Metadata"))) {
        requestPlaybackStatus(message().service());
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

    unwatchAllPlayers();

    QStringList mprisServices = reply.value();

    Q_FOREACH (const QString &service, mprisServices) {
        if (!service.contains(QLatin1String("org.mpris.MediaPlayer2"))) {
            continue;
        }
        newMediaPlayer(service);
        m_knownPlayers.append(service);
    }

    if (m_knownPlayers.isEmpty()) {
        kDebug() << "Received empty players list while active, deactivating (player quit)";
        m_lastReceivedMetadata.clear();
        m_playbackActive = false;
        if (isActive()) {
            setActive(false);
        }
    }
}

void TelepathyMPRIS::onSettingsChanged()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    config.data()->reparseConfiguration();

    KConfigGroup kdedConfig = config->group("KDED");

    bool enabledInConfig = kdedConfig.readEntry("nowPlayingEnabled", false);

    m_nowPlayingText = kdedConfig.readEntry(QLatin1String("nowPlayingText"),
                                              i18nc("The default text displayed by now playing plugin. "
                                                    "track title: %1, artist: %2, album: %3",
                                                    "Now listening to %1 by %2 from album %3",
                                                    QLatin1String("%title"), QLatin1String("%artist"), QLatin1String("%album")));

    // we only change the enable state if the config is changed, this can prevent
    // re-enable/disable nowplaying if user just changed some other unrelated settings in kcm
    if (enabledInConfig != m_enabledInConfig) {
        m_enabledInConfig = enabledInConfig;
        activatePlugin(m_enabledInConfig);
    }
}

void TelepathyMPRIS::newMediaPlayer(const QString &service)
{
    kDebug() << "Found mpris service:" << service;
    requestPlaybackStatus(service);

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

void TelepathyMPRIS::requestPlaybackStatus(const QString& service)
{
    QDBusInterface mprisInterface(service,
                                  QLatin1String("/org/mpris/MediaPlayer2"),
                                  QLatin1String("org.freedesktop.DBus.Properties"));

    QDBusPendingCall call = mprisInterface.asyncCall(QLatin1String("GetAll"),
                                                     QLatin1String("org.mpris.MediaPlayer2.Player"));

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(onPlaybackStatusReceived(QDBusPendingCallWatcher*)));
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
    activatePlugin(true);
}

void TelepathyMPRIS::onDeactivateNowPlaying()
{
    kDebug() << "Plugin deactivated on contact list request";
    activatePlugin(false);
}

void TelepathyMPRIS::onPlaybackStatusReceived(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    if (reply.isError()) {
        kWarning() << "Received error reply from DBus" << reply.error();
    } else {
        QVariantMap replyData = reply.value();
        setPlaybackStatus(replyData);
    }

    watcher->deleteLater();
}

void TelepathyMPRIS::setPlaybackStatus(const QVariantMap& replyData)
{
    //lookup if the PlaybackStatus was changed
    if (replyData.keys().contains(QLatin1String("PlaybackStatus"))) {
        if (replyData.value(QLatin1String("PlaybackStatus")) == QLatin1String("Playing")) {
            m_playbackActive = true;
        } else {
            //if the player is stopped or paused, deactivate and return to normal presence
            m_playbackActive = false;
        }
    }

    //track data change
    if (replyData.keys().contains(QLatin1String("Metadata"))) {
        m_lastReceivedMetadata = qdbus_cast<QVariantMap>(replyData.value(QLatin1String("Metadata")));
    }

    setTrackToPresence();
}

void TelepathyMPRIS::setTrackToPresence()
{
    // not enabled, no need to parse
    if (!isEnabled()) {
        return;
    }

    // not playing or no metadata, set to old presence
    if (!m_playbackActive || m_lastReceivedMetadata.isEmpty()) {
        setActive(false);
        return;
    }

    QString artist = m_lastReceivedMetadata.value(QLatin1String("xesam:artist")).toString();
    QString title = m_lastReceivedMetadata.value(QLatin1String("xesam:title")).toString();
    QString album = m_lastReceivedMetadata.value(QLatin1String("xesam:album")).toString();
    QString trackNumber = m_lastReceivedMetadata.value(QLatin1String("xesam:trackNumber")).toString();

    //we replace track's info in custom nowPlayingText
    QString statusMessage = m_nowPlayingText;
    statusMessage.replace(QLatin1String("%title"), title, Qt::CaseInsensitive);
    statusMessage.replace(QLatin1String("%artist"), artist, Qt::CaseInsensitive);
    statusMessage.replace(QLatin1String("%album"), album, Qt::CaseInsensitive);
    statusMessage.replace(QLatin1String("%track"), trackNumber, Qt::CaseInsensitive);

    Tp::Presence requestedPresence = m_globalPresence->requestedPresence();
    Tp::SimplePresence presence;

    presence.type = requestedPresence.type();
    presence.status = requestedPresence.status();
    presence.statusMessage = statusMessage;

    setRequestedPresence(Tp::Presence(presence));
    setActive(true);
}

void TelepathyMPRIS::activatePlugin(bool enabled)
{
    if (enabled == isEnabled()) {
        return;
    }

    setEnabled(enabled);

    if (!enabled) {
        //unwatch for new mpris-enabled players
        disconnect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
                   this, SLOT(serviceOwnerChanged(QString,QString,QString)));
        unwatchAllPlayers();
        m_lastReceivedMetadata.clear();
        m_playbackActive = false;
    } else {
        //watch for new mpris-enabled players
        connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
                this, SLOT(serviceOwnerChanged(QString,QString,QString)));
        detectPlayers();
    }
}

void TelepathyMPRIS::unwatchAllPlayers()
{
    // disconnect all old player to avoid double connection
    Q_FOREACH (const QString &service, m_knownPlayers) {
        QDBusConnection::sessionBus().disconnect(service,
                                                 QLatin1String("/org/mpris/MediaPlayer2"),
                                                 QLatin1String("org.freedesktop.DBus.Properties"),
                                                 QLatin1String("PropertiesChanged"),
                                                 this,
                                                 SLOT(onPlayerSignalReceived(QString,QVariantMap,QStringList)) );
    }
    m_knownPlayers.clear();
}

