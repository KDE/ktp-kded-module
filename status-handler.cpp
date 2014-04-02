/*
    Copyright (C) 2014  David Edmundson <kde@davidedmundson.co.uk>

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

#include "status-handler.h"

#include "autoconnect.h"
#include "autoaway.h"
#include "screensaveraway.h"
#include "telepathy-mpris.h"

#include <KTp/global-presence.h>

#include <TelepathyQt/PendingReady>
#include <TelepathyQt/AccountManager>

StatusHandler::StatusHandler(QObject* parent):
    m_autoConnect(0),
    m_globalPresence(0)
{
    connect(KTp::accountManager()->becomeReady(), SIGNAL(finished(Tp::PendingOperation*)),
            SLOT(onAccountManagerReady(Tp::PendingOperation*)));

    QDBusConnection::sessionBus().connect(QString(), QLatin1String("/Telepathy"), QLatin1String("org.kde.Telepathy"),
                                          QLatin1String("settingsChange"), this, SIGNAL(settingsChanged()));
}

StatusHandler::~StatusHandler()
{

}

void StatusHandler::onAccountManagerReady(Tp::PendingOperation *op)
{
    if (op->isError()) {
        return;
    }

    m_globalPresence = new KTp::GlobalPresence(this);
    m_autoConnect = new AutoConnect(this);

    m_globalPresence->setAccountManager(KTp::accountManager());
    connect(m_globalPresence, SIGNAL(requestedPresenceChanged(KTp::Presence)),
            this, SLOT(onRequestedPresenceChanged(KTp::Presence)));

    AutoAway *autoAway = new AutoAway(m_globalPresence, this);
    connect(autoAway, SIGNAL(activate(bool)),
            this, SLOT(onPluginActivated(bool)));

    connect(this, SIGNAL(settingsChanged()),
            autoAway, SLOT(reloadConfig()));

    ScreenSaverAway *screenSaverAway = new ScreenSaverAway(m_globalPresence, this);
    connect(screenSaverAway, SIGNAL(activate(bool)),
            this, SLOT(onPluginActivated(bool)));

    connect(this, SIGNAL(settingsChanged()),
            screenSaverAway, SLOT(reloadConfig()));

    TelepathyMPRIS *mpris = new TelepathyMPRIS(m_globalPresence, this);
    connect(mpris, SIGNAL(activate(bool)),
            this, SLOT(onPluginActivated(bool)));

    connect(this, SIGNAL(settingsChanged()),
            mpris, SLOT(reloadConfig()));

    //earlier in list = higher priority
    m_pluginStack << autoAway << screenSaverAway;

    //status message plugins are second to user-set status messages and presences
    m_statusMessagePluginStack << mpris;


    m_lastUserPresence = m_globalPresence->requestedPresence();
}

void StatusHandler::onRequestedPresenceChanged(const KTp::Presence &presence)
{
    // the difference between user requested offline and network related offline is the connectionStatus is connected or not
    // offline caused by network offline shold not be recorded as user requested.
    if (presence.type() == Tp::ConnectionPresenceTypeOffline
     && m_globalPresence->connectionStatus() != Tp::ConnectionStatusConnected) {
        return;
    }

    //if it's changed to what we set it to. Ignore it.
    if (presence == presenceThrottle()) {
        return;
    }

    //user is manually setting the presence.
    m_lastUserPresence = presence;

    //save presence (needed for autoconnect)
    m_autoConnect->savePresence(presence);

    // keep status messages current. User-requested status message changes (or plugin-requested status and message) won't return
    // to automatic status message until the next status message plugin signals.
    if (activeStatusMessagePlugin()) {
        if (!presence.statusMessage().isEmpty()) {
            return;
        }
        if (presence != presenceThrottle()) {
            setPresence(presenceThrottle());
        }
    }
}

void StatusHandler::onPluginActivated(bool active)
{
    Q_UNUSED(active);
    // cut down on unneccessary status updates by only updating when the current global presence isn't the same as the filtered
    // presence. This also helps with misbehaving plugins, in particular the mpris2 plugin ticks and outputs every second.
    if (m_globalPresence->currentPresence() != presenceThrottle()) {
        setPresence(presenceThrottle());
    }
}

const QString StatusHandler::statusMessageStack()
{
    QString expectedStatusMessage = m_lastUserPresence.statusMessage();
    if (activeStatusMessagePlugin() && m_lastUserPresence.statusMessage().isEmpty()) {
        expectedStatusMessage = currentPluginStatusMessage();
    }
    if (activePlugin() && !currentPluginPresence().statusMessage().isEmpty()) {
        expectedStatusMessage = currentPluginPresence().statusMessage();
    }
    return expectedStatusMessage;
}

KTp::Presence StatusHandler::presenceThrottle()
{
    KTp::Presence expectedPresence = m_lastUserPresence;
    if (activePlugin()) {
        expectedPresence = currentPluginPresence();
    }
    expectedPresence.setStatusMessage(statusMessageStack());
    return expectedPresence;
}

void StatusHandler::setPresence(const KTp::Presence &presence)
{
    Q_FOREACH(const Tp::AccountPtr &account, KTp::accountManager()->allAccounts()) {
        if (account->requestedPresence() != Tp::Presence::offline()) {
            account->setRequestedPresence(presence);
        }
    }
}

KTp::Presence StatusHandler::currentPluginPresence() const
{
    KTp::Presence requestedPresence;
    //search plugins in priority order. If a plugin is active, return the state it thinks it should be in.
    Q_FOREACH(TelepathyKDEDModulePlugin* plugin, m_pluginStack) {
        if (plugin->isActive() && plugin->isEnabled()) {
            requestedPresence = plugin->requestedPresence();
        }
    }
    return requestedPresence;
}

QString StatusHandler::currentPluginStatusMessage()
{
    QString requestedStatusMessage;
    //search plugins in priority order. If a plugin is active, return the message it thinks it should.
    Q_FOREACH(TelepathyKDEDModulePlugin* plugin, m_statusMessagePluginStack) {
    if (plugin->isActive() && plugin->isEnabled()) {
        requestedStatusMessage = plugin->requestedStatusMessage();
        }
    }
    return requestedStatusMessage;
}

bool StatusHandler::activePlugin()
{
    bool activePlugin = false;
    Q_FOREACH(TelepathyKDEDModulePlugin* plugin, m_pluginStack) {
        if (plugin->isActive()) {
            activePlugin = true;
        }
    }
    return activePlugin;
}

bool StatusHandler::activeStatusMessagePlugin()
{
    bool activeStatusMessagePlugin = false;
    Q_FOREACH(TelepathyKDEDModulePlugin* plugin, m_statusMessagePluginStack) {
        if (plugin->isActive()) {
           activeStatusMessagePlugin = true;
        }
    }
    return activeStatusMessagePlugin;
}
