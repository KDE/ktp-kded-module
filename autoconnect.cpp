/*
    Manage auto-connecting and restoring of the last presence
    Copyright (C) 2012  Dominik Cermak <d.cermak@arcor.de>

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

#include "autoconnect.h"

#include <KConfig>

#include <KTp/presence.h>

AutoConnect::AutoConnect(QObject *parent)
    : QObject(parent)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    m_kdedConfig = config->group("KDED");
    m_presenceConfig = config->group("LastPresence");
}

AutoConnect::~AutoConnect()
{
}

void AutoConnect::setAccountManager(const Tp::AccountManagerPtr &accountManager)
{
    m_accountManager = accountManager;

    uint presenceType = m_presenceConfig.readEntry<uint>(QLatin1String("PresenceType"), (uint)Tp::ConnectionPresenceTypeOffline);
    QString presenceStatus = m_presenceConfig.readEntry(QLatin1String("PresenceStatus"), QString());
    QString presenceMessage = m_presenceConfig.readEntry(QLatin1String("PresenceMessage"), QString());

    QString autoConnectString = m_kdedConfig.readEntry(QLatin1String("autoConnect"), modeToString(AutoConnect::Manual));
    Mode autoConnectMode = stringToMode(autoConnectString);

    if (autoConnectMode == AutoConnect::Enabled) {
        Q_FOREACH(Tp::AccountPtr account, m_accountManager->allAccounts()) {
            account->setRequestedPresence(Tp::Presence((Tp::ConnectionPresenceType)presenceType, presenceStatus,               presenceMessage));
        }
    }
}

void AutoConnect::savePresence(const KTp::Presence &presence)
{
    m_presenceConfig.writeEntry(QLatin1String("PresenceType"), (uint)presence.type());
    m_presenceConfig.writeEntry(QLatin1String("PresenceStatus"), presence.status());
    m_presenceConfig.writeEntry(QLatin1String("PresenceMessage"), presence.statusMessage());

    m_presenceConfig.sync();
}
