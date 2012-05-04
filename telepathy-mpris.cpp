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

    bool trackInfoFound = false;

    QString artist;
    QString title;
    QString album;
    QString trackNumber;

    //FIXME We can do less lame parsing...maybe.
    Q_FOREACH (const QVariant &property, changedProperties.values()) {  //krazy:exclude=foreach
        if (property.canConvert<QString>()) {
            if (property.toString() == QLatin1String("Paused") || property.toString() == QLatin1String("Stopped")) {
                setActive(false);
                return;
            }

            if (property.toString() == QLatin1String("Playing")) {
                QStringList mprisServices = QDBusConnection::sessionBus().interface()->registeredServiceNames().value().filter(QLatin1String("org.mpris.MediaPlayer2"));

                Q_FOREACH (const QString &service, mprisServices) {
                    QDBusInterface mprisInterface(service, QLatin1String("/org/mpris/MediaPlayer2"), QLatin1String("org.mpris.MediaPlayer2.Player"));
                    if (mprisInterface.property("PlaybackStatus") == QLatin1String("Playing")) {
                        QMap<QString, QVariant> metadata = mprisInterface.property("Metadata").toMap();
                        if (metadata.isEmpty()) {
                            break;
                        }

                        artist = metadata.value(QLatin1String("xesam:artist")).toString();
                        title = metadata.value(QLatin1String("xesam:title")).toString();
                        album = metadata.value(QLatin1String("xesam:album")).toString();
                        trackNumber = metadata.value(QLatin1String("xesam:trackNumber")).toString();
                        trackInfoFound = true;
                        break;
                    }

                }
            }
        }
    }

    if (trackInfoFound) {
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
}

void TelepathyMPRIS::detectPlayers()
{
    QDBusConnectionInterface *i = QDBusConnection::sessionBus().interface();
    QStringList mprisServices = i->registeredServiceNames().value().filter(QLatin1String("org.mpris.MediaPlayer2"));
    QStringList players;

    Q_FOREACH (const QString &service, mprisServices) {
        kDebug() << "Found mpris service:" << service;
        QDBusInterface mprisInterface(service, QLatin1String("/org/mpris/MediaPlayer2"), QLatin1String("org.mpris.MediaPlayer2.Player"));
        if (mprisInterface.property("PlaybackStatus") == QLatin1String("Playing")) {
            QVariantMap m;
            m.insert(QLatin1String("PlaybackStatus"), QVariant(QLatin1String("Playing")));
            onPlayerSignalReceived(QString(), m, QStringList());
        }

        //check if we are already watching this service
        if (!m_knownPlayers.contains(service)) {
            QDBusConnection::sessionBus().connect(
                service,
                QLatin1String("/org/mpris/MediaPlayer2"),
                QLatin1String("org.freedesktop.DBus.Properties"),
                QLatin1String("PropertiesChanged"),
                this,
                SLOT(onPlayerSignalReceived(QString,QVariantMap,QStringList)) );
        }

        players.append(service);
    }

    //this gets rid of removed services and stores only those currently present
    m_knownPlayers = players;
}

void TelepathyMPRIS::onSettingsChanged()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
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

void TelepathyMPRIS::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner)
    Q_UNUSED(newOwner)
    if (serviceName.contains(QLatin1String("org.mpris.MediaPlayer2"))) {
        kDebug() << "Found new mpris interface, running detection...";
        detectPlayers();
    }
}

void TelepathyMPRIS::onActivateNowPlaying()
{
    kDebug() << "Plugin activated";
    m_presenceActivated = true;
    detectPlayers();
}

void TelepathyMPRIS::onDeactivateNowPlaying()
{
    kDebug() << "Plugin deactivated on CL request";

    if (m_presenceActivated) {
        m_presenceActivated = false;
        setActive(false);
    }
}
