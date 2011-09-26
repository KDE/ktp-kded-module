/*
    KDE integration module for Telepathy
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

#include "telepathy-module.h"

#include <KPluginFactory>

#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include "telepathy-mpris.h"
#include "autoaway.h"
#include <KDebug>
#include <TelepathyQt4/Debug>

K_PLUGIN_FACTORY(TelepathyModuleFactory, registerPlugin<TelepathyModule>(); )
K_EXPORT_PLUGIN(TelepathyModuleFactory("telepathy_module"))

TelepathyModule::TelepathyModule(QObject* parent, const QList<QVariant>& args)
    : KDEDModule(parent)
{
    Tp::registerTypes();
    Tp::enableDebug(false);
    Tp::enableWarnings(false);

    // Start setting up the Telepathy AccountManager.
    Tp::AccountFactoryPtr  accountFactory = Tp::AccountFactory::create(QDBusConnection::sessionBus(),
                                                                       Tp::Features() << Tp::Account::FeatureCore);

    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(QDBusConnection::sessionBus(),
                                                                               Tp::Features() << Tp::Connection::FeatureCore);

    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());

    m_accountManager = Tp::AccountManager::create(QDBusConnection::sessionBus(),
                                                  accountFactory,
                                                  connectionFactory,
                                                  channelFactory);


    connect(m_accountManager->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountManagerReady(Tp::PendingOperation*)));
}

TelepathyModule::~TelepathyModule()
{
}

void TelepathyModule::onAccountManagerReady(Tp::PendingOperation* op)
{
    if (op->isError()) {
        return;
    }

    m_autoAway = new AutoAway(m_accountManager, this);
    connect(m_autoAway, SIGNAL(setPresence(Tp::Presence)),
            this, SLOT(setPresence(Tp::Presence)));

    m_mpris = new TelepathyMPRIS(m_accountManager, this);
    connect(m_mpris, SIGNAL(setPresence(Tp::Presence)),
            this, SLOT(setPresence(Tp::Presence)));
}

void TelepathyModule::setPresence(const Tp::Presence &presence)
{
    kDebug() << "Setting presence to" << presence.status() << presence.statusMessage();
    foreach (const Tp::AccountPtr &account, m_accountManager->allAccounts()) {
        if (account->isEnabled() && account->isValid() && account->isOnline()) {
            account->setRequestedPresence(presence);
        }
    }
}

#include "telepathy-module.moc"
