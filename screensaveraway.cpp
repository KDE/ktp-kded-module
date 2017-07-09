/*
    Set away - option when screen saver activated-class
    Copyright (C) 2013  Lucas Betschart <lucasbetschart@gmail.com>

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

#include "screensaveraway.h"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QDBusConnectionInterface>
#include <QDBusInterface>

ScreenSaverAway::ScreenSaverAway(QObject *parent)
    : TelepathyKDEDModulePlugin(parent)
{
    m_screenSaverInterface = new QDBusInterface(QLatin1String("org.freedesktop.ScreenSaver"),
								QLatin1String("/ScreenSaver"),
								QString(),
								QDBusConnection::sessionBus(), this);

    reloadConfig();
}

ScreenSaverAway::~ScreenSaverAway()
{
}

QString ScreenSaverAway::pluginName() const
{
    return QString::fromLatin1("screen-saver-away");
}

void ScreenSaverAway::onActiveChanged(bool newState)
{
    if (newState) {
        QString awayMessage = m_screenSaverAwayMessage;
        QDBusReply<int> idleTime = m_screenSaverInterface->asyncCall(QLatin1String("GetSessionIdleTime"));
        awayMessage.replace(QRegularExpression(QLatin1String("%te\\b")), QLatin1String("%te+") + QString::number(std::round(idleTime.value() / 1000 / 60)));
        setPlugin(Tp::Presence::away(awayMessage));
    } else {
        setPlugin(Enabled);
    }
}

void ScreenSaverAway::reloadConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathyrc"));
    config.data()->reparseConfiguration();
    KConfigGroup kdedConfig = config->group("KDED");

    bool screenSaverAwayEnabled = kdedConfig.readEntry("screenSaverAwayEnabled", true);
    m_screenSaverAwayMessage = kdedConfig.readEntry(QLatin1String("screenSaverAwayMessage"), QString());

    if (screenSaverAwayEnabled) {
        //watch for screen locked
        connect(m_screenSaverInterface, SIGNAL(ActiveChanged(bool)), this, SLOT(onActiveChanged(bool)));
    } else {
        m_screenSaverInterface->disconnect();
    }

    setPlugin(State(screenSaverAwayEnabled));
}
