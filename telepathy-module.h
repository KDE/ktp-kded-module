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

#include <TelepathyQt/AccountManager>
#include <KTp/presence.h>

class ContactRequestHandler;
namespace Tp {
    class PendingOperation;
}

namespace KTp {
    class GlobalPresence;
}

class TelepathyKDEDModulePlugin;
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
    void onPresenceChanged(const KTp::Presence &presence);
    void onPluginActivated(bool);

private:
    /** Returns the presence we think we should be in. Either from the highest priority plugin, or if none are active, the last user set.*/
    KTp::Presence currentPluginPresence();

private:
    Tp::AccountManagerPtr    m_accountManager;
    AutoAway                *m_autoAway;
    TelepathyMPRIS          *m_mpris;
    ErrorHandler            *m_errorHandler;
    KTp::GlobalPresence     *m_globalPresence;
    ContactRequestHandler   *m_contactHandler;

    QList<TelepathyKDEDModulePlugin*> m_pluginStack;
    KTp::Presence m_lastUserPresence;
};

#endif // TELEPATHY_MODULE_H
