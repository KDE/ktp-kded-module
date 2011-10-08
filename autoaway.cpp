/*
    Auto away-presence setter-class
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

#include "autoaway.h"

#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/AccountSet>

#include <KDebug>
#include <KIdleTime>
#include <KConfig>
#include <KConfigGroup>
#include "global-presence.h"

// #define PLUGIN_PRIORITY 99

AutoAway::AutoAway(QObject *parent)
    : TelepathyKDEDModulePlugin(parent),
      m_awayTimeoutId(-1),
      m_extAwayTimeoutId(-1)
//       m_idle(false)
{
    m_pluginPriority = 99;
    readConfig();

    connect(KIdleTime::instance(), SIGNAL(timeoutReached(int)),
            this, SLOT(timeoutReached(int)));

    connect(KIdleTime::instance(), SIGNAL(resumingFromIdle()),
            this, SLOT(backFromIdle()));
}

AutoAway::~AutoAway()
{
}

void AutoAway::timeoutReached(int id)
{
    if (!m_enabled) {
        return;
    }
    KIdleTime::instance()->catchNextResumeEvent();
    if (id == m_awayTimeoutId) {
        if (!m_globalPresence->onlineAccounts()->accounts().isEmpty()) {
            if (m_globalPresence->currentPresence().type() != Tp::Presence::away().type() ||
                m_globalPresence->currentPresence().type() != Tp::Presence::xa().type() ||
                m_globalPresence->currentPresence().type() != Tp::Presence::hidden().type()) {

//                 m_globalPresence->saveCurrentPresence(100);
//                 m_idle = true;
                m_requestedPresence = Tp::Presence::away();
                Q_EMIT activate(true);
//                 Q_EMIT requestPresenceChange(Tp::Presence::away());

            }
        } else if (id == m_extAwayTimeoutId) {
            if (!m_globalPresence->onlineAccounts()->accounts().isEmpty()) {
                if (m_globalPresence->currentPresence().type() == Tp::Presence::away().type()) {
//                     Q_EMIT requestPresenceChange(Tp::Presence::xa());
                        m_requestedPresence = Tp::Presence::xa();
                        Q_EMIT activate(true);
                }
            }
        }
    }
}

void AutoAway::backFromIdle()
{
    kDebug();
    Q_EMIT activate(false);
}

void AutoAway::readConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    KConfigGroup kdedConfig = config->group("KDED");

    bool autoAwayEnabled = kdedConfig.readEntry("autoAwayEnabled", true);
    bool autoXAEnabled = kdedConfig.readEntry("autoXAEnabled", true);

    //remove all our timeouts and only readd them if auto-away is enabled
    //WARNING: can't use removeAllTimeouts because this runs inside KDED, it would remove other KDED timeouts as well
    KIdleTime::instance()->removeIdleTimeout(m_awayTimeoutId);
    KIdleTime::instance()->removeIdleTimeout(m_extAwayTimeoutId);

    if (autoAwayEnabled) {
        int awayTime = kdedConfig.readEntry("awayAfter", 5);
        m_awayTimeoutId = KIdleTime::instance()->addIdleTimeout(awayTime * 60 * 1000);
        m_enabled = true;
    } else {
        m_enabled = false;
    }
    if (autoAwayEnabled && autoXAEnabled) {
        int xaTime = kdedConfig.readEntry("xaAfter", 15);
        m_extAwayTimeoutId = KIdleTime::instance()->addIdleTimeout(xaTime * 60 * 1000);
    }
}

void AutoAway::onSettingsChanged()
{
    readConfig();
}
/*
bool AutoAway::isIdle()
{
    return m_idle;
}*/
