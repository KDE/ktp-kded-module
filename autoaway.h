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


#ifndef AUTOAWAY_H
#define AUTOAWAY_H

#include "telepathy-kded-module-plugin.h"

#include <TelepathyQt/Presence>
#include <TelepathyQt/AccountManager>

namespace KTp {
class GlobalPresence;
}

class AutoAway : public TelepathyKDEDModulePlugin
{
    Q_OBJECT

public:
    AutoAway(KTp::GlobalPresence *globalPresence, QObject* parent = 0);
    ~AutoAway();

    void readConfig();
    QString pluginName() const;

public Q_SLOTS:
    void onSettingsChanged();

private Q_SLOTS:
    void timeoutReached(int);
    void backFromIdle();

private:
    int m_awayTimeoutId;
    int m_extAwayTimeoutId;

    QString m_awayMessage;
    QString m_xaMessage;
};

#endif // AUTOAWAY_H
