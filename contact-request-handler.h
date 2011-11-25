/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2011  Martin Klapetek <email>

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


#ifndef CONTACT_REQUEST_HANDLER_H
#define CONTACT_REQUEST_HANDLER_H

#include <QObject>

#include <TelepathyQt4/Types>
#include <TelepathyQt4/AccountManager>

#include <KStatusNotifierItem>
#include <KNotification>

class KMenu;
class KAction;
class ContactRequestHandler : public QObject
{
    Q_OBJECT
public:
    ContactRequestHandler(const Tp::AccountManagerPtr& am, QObject *parent = 0);
    virtual ~ContactRequestHandler();

    void monitorPresence(const Tp::ConnectionPtr &connection);

public Q_SLOTS:
    void onNewAccountAdded(const Tp::AccountPtr &account);
    void onContactManagerStateChanged(Tp::ContactListState state);
    void onContactManagerStateChanged(const Tp::ContactManagerPtr &contactManager, Tp::ContactListState state);
    void onAccountsPresenceStatusFiltered();
    void onPresencePublicationRequested(const Tp::Contacts& contacts);
    void onConnectionChanged(const Tp::ConnectionPtr& connection);
    void updateMenus();

    void onContactRequestApproved();
    void onContactRequestDenied();
    void onAuthorizePresencePublicationFinished(Tp::PendingOperation*);

private:
    KStatusNotifierItem *notifierItem();
    void updateNotifierItemTooltip();

    QWeakPointer<KStatusNotifierItem> m_notifierItem;
    Tp::AccountManagerPtr m_accountManager;
    KMenu *m_notifierMenu;
    QHash<QString, Tp::ContactPtr> m_pendingContacts;
    QHash<QString, KMenu*> m_menuItems;
    KAction *m_noContactsAction;
};

#endif // CONTACT_REQUEST_HANDLER_H
