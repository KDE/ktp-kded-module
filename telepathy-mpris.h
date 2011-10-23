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
#include <TelepathyQt4/Presence>
#include <TelepathyQt4/AccountManager>

class TelepathyMPRIS : public TelepathyKDEDModulePlugin
{
    Q_OBJECT

public:
    TelepathyMPRIS(GlobalPresence *globalPresence, QObject *parent = 0);
    virtual ~TelepathyMPRIS();

public Q_SLOTS:
    void onPlayerSignalReceived(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void onSettingsChanged();
    void detectPlayers();
    void serviceOwnerChanged(const QString &a, const QString &b, const QString &c);
    void onActivateNowPlaying();
    void onDeactivateNowPlaying();

Q_SIGNALS:
    void togglePlaybackActive(bool);

private:
    QStringList m_knownPlayers;
    bool m_presenceActivated;
};

#endif // TELEPATHY_MPRIS_H
