/*
    Copyright (C) 2012 Rohan Garg <rohangarg@kubuntu.org>

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

#ifndef CONTACTNOTIFY_H
#define CONTACTNOTIFY_H

#include <TelepathyQt/Types>
#include <TelepathyQt/Connection>

#include <KTp/global-contact-manager.h>

class KIcon;

using namespace KTp;

class ContactNotify : public QObject
{
    Q_OBJECT
public:
    ContactNotify(const Tp::AccountManagerPtr &accountMgr, QObject *parent = 0);

private Q_SLOTS:
    void onContactsChanged(const Tp::Contacts &contactsAdded, const Tp::Contacts &contactsRemoved);
    void contactPresenceChanged(const Tp::Presence &presence);

private:
    Tp::AccountManagerPtr m_accountManager;
    void sendNotification(const QString &text, const KIcon &icon, const Tp::ContactPtr &contact);
    QHash<QString, int> m_presenceHash;
};

#endif // CONTACTNOTIFY_H
