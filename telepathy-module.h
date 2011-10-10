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


#ifndef TELEPATHY_MODULE_H
#define TELEPATHY_MODULE_H

#include <KDEDModule>

#include <TelepathyQt4/AccountManager>

namespace Tp {
    class PendingOperation;
}

class TelepathyKDEDModulePlugin;
class GlobalPresence;
class ErrorHandler;
class TelepathyMPRIS;
class AutoAway;

class TelepathyModule : public KDEDModule
{
    Q_OBJECT

public:
    TelepathyModule(QObject *parent, const QList<QVariant> &args);
    ~TelepathyModule();

Q_SIGNALS:
    void settingsChanged();

private Q_SLOTS:
    void onAccountManagerReady(Tp::PendingOperation*);
    void onPresenceChanged(const Tp::Presence &presence);
    void onPluginActivated(bool);

private:
    Tp::AccountManagerPtr    m_accountManager;
    AutoAway                *m_autoAway;
    TelepathyMPRIS          *m_mpris;
    ErrorHandler            *m_errorHandler;
    GlobalPresence          *m_globalPresence;

    QList<TelepathyKDEDModulePlugin*> m_pluginStack;
};

#endif // TELEPATHY_MODULE_H
