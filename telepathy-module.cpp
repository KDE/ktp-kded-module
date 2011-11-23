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

#include <TelepathyQt4/AccountFactory>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>
#include <TelepathyQt4/Debug>

#include <KTelepathy/global-presence.h>

#include "telepathy-mpris.h"
#include "autoaway.h"
#include "error-handler.h"
#include "telepathy-kded-module-plugin.h"

#include <KConfigGroup>

K_PLUGIN_FACTORY(TelepathyModuleFactory, registerPlugin<TelepathyModule>(); )
K_EXPORT_PLUGIN(TelepathyModuleFactory("telepathy_module", "telepathy-kded-module"))

TelepathyModule::TelepathyModule(QObject* parent, const QList<QVariant>& args)
    : KDEDModule(parent)
{
    Q_UNUSED(args)

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

    QDBusConnection::sessionBus().connect(QString(), QLatin1String("/Telepathy"), QLatin1String("org.kde.Telepathy"),
                                          QLatin1String("settingsChange"), this, SIGNAL(settingsChanged()) );

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

    m_autoAway = new AutoAway(m_globalPresence, this);
    connect(m_autoAway, SIGNAL(activate(bool)),
            this, SLOT(onPluginActivated(bool)));

    connect(this, SIGNAL(settingsChanged()),
            m_autoAway, SLOT(onSettingsChanged()));

    m_mpris = new TelepathyMPRIS(m_globalPresence, this);
    connect(m_mpris, SIGNAL(activate(bool)),
            this, SLOT(onPluginActivated(bool)));

    connect(this, SIGNAL(settingsChanged()),
            m_mpris, SLOT(onSettingsChanged()));

    m_errorHandler = new ErrorHandler(m_accountManager, this);
}

void TelepathyModule::onPresenceChanged(const Tp::Presence &presence)
{
    //only save if the presence is not auto-set
    if (m_pluginStack.isEmpty()) {
        KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
        KConfigGroup presenceConfig = config->group("LastPresence");

        presenceConfig.writeEntry(QLatin1String("PresenceType"), (uint)m_globalPresence->currentPresence().type());
        presenceConfig.writeEntry(QLatin1String("PresenceStatus"), m_globalPresence->currentPresence().status());
        presenceConfig.writeEntry(QLatin1String("PresenceMessage"), m_globalPresence->currentPresence().statusMessage());

        presenceConfig.sync();
    }
}

void TelepathyModule::onPluginActivated(bool active)
{
    TelepathyKDEDModulePlugin *plugin = qobject_cast<TelepathyKDEDModulePlugin*>(sender());
    Q_ASSERT(plugin);

    if (active) {
        kDebug() << "Received activation request, current active plugins:" << m_pluginStack.size();
        if (m_pluginStack.isEmpty()) {
            m_globalPresence->saveCurrentPresence();
            m_pluginStack.append(plugin);
        } else if (!m_pluginStack.contains(plugin)) {
            int i;
            for (i = 0; i < m_pluginStack.size(); i++) {
                if (plugin->pluginPriority() >= m_pluginStack.at(i)->pluginPriority()) {
                    break;
                }
            }
            m_pluginStack.insert(i, plugin);
        }

        kDebug() << "Activating" << plugin->pluginName();

        if (!m_globalPresence->onlineAccounts()->accounts().isEmpty()) {
            //signal all global presence instances that they should not save global presence message
            QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/Telepathy"),
                                                              QLatin1String( "org.kde.Telepathy"),
                                                              QLatin1String("presenceChanger"));
            message.setArguments(QList<QVariant>() << m_pluginStack.first()->pluginName());
            QDBusConnection::sessionBus().send(message);

            m_globalPresence->setPresence(m_pluginStack.first()->requestedPresence());
        }
    } else {
        kDebug() << "Received deactivation request, current active plugins:" << m_pluginStack.size();
        while (!m_pluginStack.isEmpty()) {
            if (!m_pluginStack.first()->isActive()) {
                kDebug() << "Deactivating" << m_pluginStack.first()->pluginName();
                m_pluginStack.removeFirst();
            } else {
                break;
            }
        }

        if (!m_globalPresence->onlineAccounts()->accounts().isEmpty()) {
            if (m_pluginStack.isEmpty()) {
                //signal out that presences are back to user control
                QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/Telepathy"),
                                                                  QLatin1String( "org.kde.Telepathy"),
                                                                  QLatin1String("presenceChanger"));
                message.setArguments(QList<QVariant>() << QString::fromLatin1("user"));
                QDBusConnection::sessionBus().send(message);

                m_globalPresence->restoreSavedPresence();
            } else {
                m_globalPresence->setPresence(m_pluginStack.first()->requestedPresence());
            }
        }
    }

    kDebug() << "Active plugins (" << m_pluginStack.size() << ")";
    for(int i = 0; i < m_pluginStack.size(); i++) {
        kDebug() << "  " << m_pluginStack.at(i)->pluginName();
    }
}

#include "telepathy-module.moc"
