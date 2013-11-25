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


#ifndef TELEPATHY_MPRIS_H
#define TELEPATHY_MPRIS_H

#include "telepathy-kded-module-plugin.h"

class TelepathyMPRIS : public TelepathyKDEDModulePlugin, protected QDBusContext
{
    Q_OBJECT

public:
    explicit TelepathyMPRIS(KTp::GlobalPresence *globalPresence, QObject *parent = 0);
    virtual ~TelepathyMPRIS();

    QString pluginName() const;

public Q_SLOTS:
    void reloadConfig();
    void onActivateNowPlaying();
    void onDeactivateNowPlaying();

Q_SIGNALS:
    void togglePlaybackActive(bool);

private Q_SLOTS:
    void serviceNameFetchFinished(QDBusPendingCallWatcher *callWatcher);
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void onPlaybackStatusReceived(QDBusPendingCallWatcher *watcher);
    void onPlayerSignalReceived(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    void detectPlayers();
    void watchPlayer(const QString &player);
    void requestPlaybackStatus(const QString& service);
    void setPlaybackStatus(const QVariantMap &reply);
    void setTrackToPresence();
    void activatePlugin(bool enabled);
    void unwatchAllPlayers();

    bool m_enabledInConfig;
    QStringList m_watchedPlayers;
    QString m_nowPlayingText;

    QVariantMap m_lastReceivedMetadata;
    bool m_playbackActive;
};

#endif // TELEPATHY_MPRIS_H
