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

AutoConnect::AutoConnect(QObject *parent)
    : QObject(parent)
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    m_kdedConfig = config->group("KDED");
}

AutoConnect::~AutoConnect()
{
}

void AutoConnect::setAccountManager(const Tp::AccountManagerPtr &accountManager)
{
    m_accountManager = accountManager;
    onSettingsChanged();
}

void AutoConnect::setAutomaticPresence(const Tp::Presence &presence)
{
    QString autoConnectString = m_kdedConfig.readEntry(QLatin1String("autoConnect"), modeToString(AutoConnect::Manual));
    Mode autoConnectMode = stringToMode(autoConnectString);

    // Don't interfere if the user set it to manual.
    if (autoConnectMode != AutoConnect::Manual) {
        Q_FOREACH(Tp::AccountPtr account, m_accountManager->allAccounts()) {
            if ((autoConnectMode == AutoConnect::Enabled) && (account->automaticPresence() != presence)) {
                account->setAutomaticPresence(presence);
            } else if ((autoConnectMode == AutoConnect::Disabled) && (account->automaticPresence() != Tp::Presence::available())) {
                // The user disabled it, so reset the automatic presence to its default value (available).
                account->setAutomaticPresence(Tp::Presence::available());
            }
        }
    }
}

void AutoConnect::onSettingsChanged()
{
    if (m_accountManager) {
        QString autoConnect = m_kdedConfig.readEntry(QLatin1String("autoConnect"), modeToString(AutoConnect::Manual));

        // Don't interfere if the user set it to manual.
        if (autoConnect != modeToString(AutoConnect::Manual)) {
            Q_FOREACH(Tp::AccountPtr account, m_accountManager->allAccounts()) {
                if ((autoConnect == modeToString(AutoConnect::Enabled)) && (!account->connectsAutomatically())) {
                    account->setConnectsAutomatically(true);
                } else if ((autoConnect == modeToString(AutoConnect::Disabled)) && (account->connectsAutomatically())) {
                    account->setConnectsAutomatically(false);
                }
            }
        }
    }
}
