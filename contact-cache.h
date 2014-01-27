/*
    Copyright (C) 2014  David Edmundson <kde@davidedmundson.co.uk>
    Copyright (C) 2014  Alexandr Akulich <akulichalexander@gmail.com>

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

#ifndef CONTACTCACHE_H
#define CONTACTCACHE_H

#include <QObject>
#include <QSqlDatabase>
#include <TelepathyQt/Types>

namespace Tp {
class PendingOperation;
}

class ContactCache : public QObject
{
    Q_OBJECT

public:
    ContactCache(QObject *parent = 0);

private Q_SLOTS:
    void onAccountManagerReady(Tp::PendingOperation *op);
    void onNewAccount(const Tp::AccountPtr &account);
    void onAccountRemoved();
    void onContactManagerStateChanged();
    void onAccountConnectionChanged(const Tp::ConnectionPtr &connection);
    void onAllKnownContactsChanged(const Tp::Contacts &added, const Tp::Contacts &removed);

private:
    void connectToAccount(const Tp::AccountPtr &account);
    bool accountIsInteresting(const Tp::AccountPtr &account) const;
    void syncContactsOfAccount(const Tp::AccountPtr &account);
    void checkContactManagerState(const Tp::ContactManagerPtr &contactManager);
    QSqlDatabase m_db;
};

#endif // CONTACTCACHE_H
