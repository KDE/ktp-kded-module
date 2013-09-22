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
#include <KDebug>

#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Debug>

#include <KTp/contact-factory.h>
#include <KTp/global-presence.h>

#include "telepathy-mpris.h"
#include "autoaway.h"
#include "autoconnect.h"
#include "error-handler.h"
#include "telepathy-kded-module-plugin.h"
#include "contactnotify.h"
#include "screensaveraway.h"

#include <KConfigGroup>
#include "contact-request-handler.h"

K_PLUGIN_FACTORY(TelepathyModuleFactory, registerPlugin<TelepathyModule>(); )
K_EXPORT_PLUGIN(TelepathyModuleFactory("ktp_integration_module", "kded_ktp_integration_module"))

TelepathyModule::TelepathyModule(QObject* parent, const QList<QVariant>& args)
    : KDEDModule(parent)
    , m_autoAway( 0 )
    , m_mpris( 0 )
    , m_autoConnect( 0 )
    , m_errorHandler( 0 )
    , m_globalPresence( 0 )
    , m_contactHandler( 0 )
    , m_contactNotify( 0 )
    , m_screenSaverAway( 0 )
{
    Q_UNUSED(args)

    Tp::registerTypes();
    Tp::enableDebug(false);
    Tp::enableWarnings(false);

    // Start setting up the Telepathy AccountManager.
    Tp::AccountFactoryPtr  accountFactory = Tp::AccountFactory::create(QDBusConnection::sessionBus(),
                                                                       Tp::Features() << Tp::Account::FeatureCore);

    Tp::ConnectionFactoryPtr connectionFactory = Tp::ConnectionFactory::create(QDBusConnection::sessionBus(),
                                                                               Tp::Features() << Tp::Connection::FeatureCore
                                                                                              << Tp::Connection::FeatureRoster);

    Tp::ContactFactoryPtr contactFactory = KTp::ContactFactory::create(Tp::Features() << Tp::Contact::FeatureAlias
                                                                                      << Tp::Contact::FeatureSimplePresence
                                                                                      << Tp::Contact::FeatureAvatarToken
                                                                                      << Tp::Contact::FeatureCapabilities);

    Tp::ChannelFactoryPtr channelFactory = Tp::ChannelFactory::create(QDBusConnection::sessionBus());

    m_accountManager = Tp::AccountManager::create(QDBusConnection::sessionBus(),
                                                  accountFactory,
                                                  connectionFactory,
                                                  channelFactory,
                                                  contactFactory);


    connect(m_accountManager->becomeReady(),
            SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountManagerReady(Tp::PendingOperation*)));

    QDBusConnection::sessionBus().connect(QString(), QLatin1String("/Telepathy"), QLatin1String("org.kde.Telepathy"),
                                          QLatin1String("settingsChange"), this, SIGNAL(settingsChanged()));

}

TelepathyModule::~TelepathyModule()
{
}

void TelepathyModule::onAccountManagerReady(Tp::PendingOperation* op)
{
    if (op->isError()) {
        return;
    }

    m_globalPresence = new KTp::GlobalPresence(this);
    m_globalPresence->setAccountManager(m_accountManager);
    connect(m_globalPresence, SIGNAL(requestedPresenceChanged(KTp::Presence)),
            this, SLOT(onRequestedPresenceChanged(KTp::Presence)));

    m_autoAway = new AutoAway(m_globalPresence, this);
    connect(m_autoAway, SIGNAL(activate(bool)),
            this, SLOT(onPluginActivated(bool)));

    connect(this, SIGNAL(settingsChanged()),
            m_autoAway, SLOT(onSettingsChanged()));

    m_screenSaverAway = new ScreenSaverAway(m_globalPresence, this);
    connect(m_screenSaverAway, SIGNAL(activate(bool)),
            this, SLOT(onPluginActivated(bool)));

    connect(this, SIGNAL(settingsChanged()),
            m_screenSaverAway, SLOT(onSettingsChanged()));

    m_mpris = new TelepathyMPRIS(m_globalPresence, this);
    connect(m_mpris, SIGNAL(activate(bool)),
            this, SLOT(onPluginActivated(bool)));

    connect(this, SIGNAL(settingsChanged()),
            m_mpris, SLOT(onSettingsChanged()));

    m_autoConnect = new AutoConnect(this);
    m_autoConnect->setAccountManager(m_accountManager);

    //earlier in list = higher priority
    m_pluginStack << m_autoAway << m_screenSaverAway << m_mpris;

    m_errorHandler = new ErrorHandler(m_accountManager, this);
    m_contactHandler = new ContactRequestHandler(m_accountManager, this);
    m_contactNotify = new ContactNotify(m_accountManager, this);

    m_lastUserPresence = m_globalPresence->requestedPresence();
}

void TelepathyModule::onRequestedPresenceChanged(const KTp::Presence &presence)
{
    // the difference between user requested offline and network related offline is the connectionStatus is connected or not
    // offline caused by network offline shold not be recorded as user requested.
    if (presence.type() == Tp::ConnectionPresenceTypeOffline
     && m_globalPresence->connectionStatus() != Tp::ConnectionStatusConnected) {
        return;
    }

    //if it's changed to what we set it to. Ignore it.
    if (presence == currentPluginPresence()) {
        return;
    }

    //user is manually setting the presence.
    m_lastUserPresence = presence;

    //save presence (needed for autoconnect)
    m_autoConnect->savePresence(presence);
}

void TelepathyModule::onPluginActivated(bool active)
{
    Q_UNUSED(active);
    //a plugin has changed state, set presence to whatever a plugin thinks it should be (or restore users setting)
    setPresence(currentPluginPresence());
}

void TelepathyModule::setPresence(const KTp::Presence &presence)
{
    Q_FOREACH(const Tp::AccountPtr &account, m_accountManager->allAccounts()) {
        if (account->isEnabled() &&
            (account->connectionStatusReason() == Tp::ConnectionStatusReasonNoneSpecified ||
             account->connectionStatusReason() == Tp::ConnectionStatusReasonRequested)) {
            account->setRequestedPresence(presence);
        }
    }
}

KTp::Presence TelepathyModule::currentPluginPresence() const
{
    //search plugins in priority order. If a plugin is active, return the state it thinks it should be in.
    Q_FOREACH(TelepathyKDEDModulePlugin* plugin, m_pluginStack) {
        if (plugin->isActive() && plugin->isEnabled()) {
            return plugin->requestedPresence();
        }
    }
    //no plugins active, return the last presence the user set.
    return m_lastUserPresence;
}

#include "telepathy-module.moc"
