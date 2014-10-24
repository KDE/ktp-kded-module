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

#include <KTp/global-presence.h>

#include <KDebug>
#include <KSharedConfig>
#include <KConfig>
#include <KConfigGroup>

#include <QDBusConnectionInterface>
#include <QDBusInterface>

ScreenSaverAway::ScreenSaverAway(KTp::GlobalPresence *globalPresence, QObject *parent)
    : TelepathyKDEDModulePlugin(globalPresence, parent)
{
    reloadConfig();

    //watch for screen locked
    QDBusConnection::sessionBus().connect(QString(),
                                          QLatin1String("/ScreenSaver"),
                                          QLatin1String("org.freedesktop.ScreenSaver"),
                                          QLatin1String("ActiveChanged"),
                                          this,
                                          SLOT(onActiveChanged(bool)));
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
    if (!isEnabled()) {
        return;
    }

    if (newState) {
        m_screenSaverAwayMessage.replace(QLatin1String("%time"), QDateTime::currentDateTimeUtc().toString(QLatin1String("hh:mm:ss")), Qt::CaseInsensitive);
        setRequestedPresence(Tp::Presence::away(m_screenSaverAwayMessage));
        setActive(true);
    } else {
        kDebug();
        setActive(false);
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
        setEnabled(true);
    } else {
        setEnabled(false);
    }
}
