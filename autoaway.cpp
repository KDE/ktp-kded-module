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

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <KIdleTime>

AutoAway::AutoAway(QObject *parent)
    : TelepathyKDEDModulePlugin(parent),
    m_awayTimeoutId(-1),
    m_extAwayTimeoutId(-1),
    m_awayPresence(Tp::Presence::away()),
    m_extAwayPresence(Tp::Presence::xa())
{
    reloadConfig();
}

AutoAway::~AutoAway()
{
}

QString AutoAway::pluginName() const
{
    return QString::fromLatin1("auto-away");
}

void AutoAway::timeoutReached(int id)
{
    if (id == m_awayTimeoutId) {
        setPlugin(m_awayPresence);
    } else if (id == m_extAwayTimeoutId) {
        setPlugin(m_extAwayPresence);
    } else {
        return;
    }

    KIdleTime::instance()->catchNextResumeEvent();
}

void AutoAway::backFromIdle()
{
    setPlugin(Enabled);
}

void AutoAway::reloadConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    config.data()->reparseConfiguration();
    KConfigGroup kdedConfig = config->group("KDED");

    bool autoAwayEnabled = kdedConfig.readEntry("autoAwayEnabled", true);
    bool autoXAEnabled = kdedConfig.readEntry("autoXAEnabled", true);

    //WARNING: can't use removeAllTimeouts because this runs inside KDED, it would remove other KDED timeouts as well
    KIdleTime::instance()->removeIdleTimeout(m_awayTimeoutId);
    m_awayTimeoutId = -1;
    KIdleTime::instance()->removeIdleTimeout(m_extAwayTimeoutId);
    m_extAwayTimeoutId = -1;

    if (autoAwayEnabled) {
        connect(KIdleTime::instance(), static_cast<void (KIdleTime::*)(int)>(&KIdleTime::timeoutReached), this, &AutoAway::timeoutReached);
        connect(KIdleTime::instance(), &KIdleTime::resumingFromIdle, this, &AutoAway::backFromIdle);

        int awayTime = kdedConfig.readEntry("awayAfter", 5);
        QString awayMessage = kdedConfig.readEntry(QLatin1String("awayMessage"), QString());
        awayMessage.replace(QRegularExpression(QLatin1String("%te\\b")), QLatin1String("%te+") + QString::number(awayTime));
        m_awayPresence.setStatusMessage(awayMessage);
        m_awayTimeoutId = KIdleTime::instance()->addIdleTimeout(awayTime * 60 * 1000);
    } else {
        disconnect(KIdleTime::instance());
    }

    if (autoAwayEnabled && autoXAEnabled) {
        int xaTime = kdedConfig.readEntry("xaAfter", 15);
        QString xaMessage = kdedConfig.readEntry(QLatin1String("xaMessage"), QString());
        xaMessage.replace(QRegularExpression(QLatin1String("%te\\b")), QLatin1String("%te+") + QString::number(xaTime));
        m_extAwayPresence.setStatusMessage(xaMessage);
        m_extAwayTimeoutId = KIdleTime::instance()->addIdleTimeout(xaTime * 60 * 1000);
    }

    setPlugin(State(autoAwayEnabled));
}
