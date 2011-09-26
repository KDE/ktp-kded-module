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

#include <KIdleTime>
#include <TelepathyQt4/AccountManager>
#include <TelepathyQt4/AccountSet>
#include <KDebug>

AutoAway::AutoAway(const Tp::AccountManagerPtr& am, QObject* parent)
    : QObject(parent)
{
    m_accountManager = am;
    m_awayTimeoutId = KIdleTime::instance()->addIdleTimeout(10 * 1000);
    m_extAwayTimeoutId = KIdleTime::instance()->addIdleTimeout(20 * 1000);
    m_prevPresence = Tp::Presence::available();

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
    KIdleTime::instance()->catchNextResumeEvent();
    if (id == m_awayTimeoutId) {
        if (!m_accountManager->onlineAccounts()->accounts().isEmpty()) {
            if (m_accountManager->onlineAccounts()->accounts().first()->currentPresence().type() != Tp::Presence::away().type() ||
                m_accountManager->onlineAccounts()->accounts().first()->currentPresence().type() != Tp::Presence::xa().type() ||
                m_accountManager->onlineAccounts()->accounts().first()->currentPresence().type() != Tp::Presence::hidden().type()) {

                m_prevPresence = m_accountManager->onlineAccounts()->accounts().first()->currentPresence();
                emit setPresence(Tp::Presence::away());

            }
        } else if (id == m_extAwayTimeoutId) {
            if (!m_accountManager->onlineAccounts()->accounts().isEmpty()) {
                if (m_accountManager->onlineAccounts()->accounts().first()->currentPresence().type() == Tp::Presence::away().type()) {
                    emit setPresence(Tp::Presence::xa());
                }
            }
        }
    }
}

void AutoAway::backFromIdle()
{
    kDebug();
    emit setPresence(m_prevPresence);
}
