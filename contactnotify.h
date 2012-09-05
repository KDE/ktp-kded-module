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

class KIcon;

#include <TelepathyQt/Types>
#include <TelepathyQt/Connection>

class ContactNotify : public QObject
{
    Q_OBJECT
public:
    ContactNotify(Tp::AccountManagerPtr accountMgr, QObject *parent = 0);

private:
    Tp::AccountManagerPtr m_accountManager;
    Tp::ConnectionPtr m_connection;
    void sendNotification(QString, KIcon, const Tp::Contact*);

private Q_SLOTS:
    void accountCameOnline(bool);
    void accountIsOnline(const Tp::Account *);
    void contactPresenceChanged(const Tp::Presence);
    void onStatusChanged(const Tp::ConnectionStatus);
};

#endif // CONTACTNOTIFY_H
