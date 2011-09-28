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
#include <TelepathyQt4/AccountSet>

TelepathyMPRIS::TelepathyMPRIS(const Tp::AccountManagerPtr am, QObject* parent) : QObject(parent)
{
    m_accountManager = am;

    if (am->onlineAccounts()->accounts().isEmpty()) {
        return;
    }

    m_originalPresence = am->onlineAccounts()->accounts().first()->currentPresence().statusMessage();

    QDBusConnectionInterface *i = QDBusConnection::sessionBus().interface();
    QStringList mprisServices = i->registeredServiceNames().value().filter("org.mpris.MediaPlayer2");

    QString artist;
    QString title;
    QString album;

    foreach (const QString &service, mprisServices) {
        QDBusInterface mprisInterface(service, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player");
        if (mprisInterface.property("PlaybackStatus") == QLatin1String("Playing")) {
            QMap<QString, QVariant> metadata = mprisInterface.property("Metadata").toMap();
            artist = metadata.value("xesam:artist").toString();
            title = metadata.value("xesam:title").toString();
            album = metadata.value("xesam:album").toString();

            QDBusConnection::sessionBus().connect(
                "org.mpris.MediaPlayer2.clementine",
                "/org/mpris/MediaPlayer2",
                "org.freedesktop.DBus.Properties",
                "PropertiesChanged",
                this,
                SLOT(onPlayerSignalReceived(const QString& ,
                                            const QVariantMap& ,
                                            const QStringList& )) );

            break;
        }
    }

    if (!am->onlineAccounts()->accounts().isEmpty()) {
        Tp::Presence currentPresence = am->onlineAccounts()->accounts().first()->currentPresence();

        Tp::SimplePresence presence;
        presence.type = currentPresence.type();
        presence.status = currentPresence.status();
        presence.statusMessage = QString("Now listening to %1 by %2 from album %3").arg(title, artist, album);

        kDebug() << "Setting presence message to" << presence.statusMessage;

        emit setPresence(presence);
    }
}

TelepathyMPRIS::~TelepathyMPRIS()
{

}

void TelepathyMPRIS::onPlayerSignalReceived(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(interface)
    Q_UNUSED(invalidatedProperties)

    if (m_accountManager->onlineAccounts()->accounts().isEmpty()) {
        return;
    }
    //FIXME We can do less lame parsing
    foreach (const QVariant &property, changedProperties.values()) {
        if (property.canConvert<QDBusArgument>()) {
            QString artist;
            QString title;
            QString album;

            QDBusArgument g = property.value<QDBusArgument>();
            QMap<QString, QVariant> k = qdbus_cast<QMap<QString, QVariant> >(g);
            title = k.value("xesam:title").toString();
            artist = k.value("xesam:artist").toString();
            album = k.value("xesam:album").toString();

            Tp::Presence currentPresence = m_accountManager->onlineAccounts()->accounts().first()->currentPresence();

            Tp::SimplePresence presence;
            presence.type = currentPresence.type();
            presence.status = currentPresence.status();
            presence.statusMessage = QString("Now listening to %1 by %2 from album %3").arg(title, artist, album);

            emit setPresence(presence);
        }

        if (property.canConvert<QString>()) {
            if (property.toString() == QLatin1String("Paused")) {
                Tp::Presence currentPresence = m_accountManager->onlineAccounts()->accounts().first()->currentPresence();

                Tp::SimplePresence presence;
                presence.type = currentPresence.type();
                presence.status = currentPresence.status();
                presence.statusMessage = m_originalPresence;
            }
        }
    }

}

void TelepathyMPRIS::onSettingsChanged()
{

}
