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

class ErrorHandler;
class ContactNotify;
class StatusHandler;

class TelepathyModule : public KDEDModule
{
    Q_OBJECT

public:
    TelepathyModule(QObject *parent, const QList<QVariant> &args);
    ~TelepathyModule();

Q_SIGNALS:
    void settingsChanged();

private Q_SLOTS:
    void onAccountManagerReady(Tp::PendingOperation *op);

private:
    StatusHandler           *m_statusHandler;
    ContactRequestHandler   *m_contactHandler;
    ContactNotify           *m_contactNotify;
    ErrorHandler            *m_errorHandler;

};

#endif // TELEPATHY_MODULE_H
