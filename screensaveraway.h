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


#ifndef SCREENSAVERAWAY_H
#define SCREENSAVERAWAY_H

#include "telepathy-kded-module-plugin.h"

class ScreenSaverAway : public TelepathyKDEDModulePlugin
{
    Q_OBJECT

public:
    explicit ScreenSaverAway(QObject *parent = 0);
    ~ScreenSaverAway();

    QString pluginName() const;

public Q_SLOTS:
    void reloadConfig();

private Q_SLOTS:
    void onActiveChanged(bool newState);

private:
    QDBusInterface *m_screenSaverInterface;
    QString m_screenSaverAwayMessage;
};

#endif // SCREENSAVERAWAY_H
