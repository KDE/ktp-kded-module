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

#include "contactnotify.h"

#include <KDebug>

#include <TelepathyQt/ContactManager>
#include <TelepathyQt/Account>
#include <TelepathyQt/Contact>

#include <KNotification>
#include <KAboutData>

#include <KTp/presence.h>

ContactNotify::ContactNotify(const Tp::AccountManagerPtr accountMgr, QObject *parent) :
    QObject(parent)
{
    m_accountManager = accountMgr;
    if (!m_accountManager) {
        return;
    }

    QList<Tp::AccountPtr> onlineAccounts = m_accountManager->allAccounts();
    Q_FOREACH(const Tp::AccountPtr &account, onlineAccounts) {
        // Accounts that are already online should be handled immediately
        if (account.data()->isOnline()) {
            kDebug() << "Account" << account.data()->normalizedName() << "is Online";
            accountIsOnline(&account);
        }

        // Handle accounts coming online
        connect(account.data(), SIGNAL(onlinenessChanged(bool)),
                SLOT(accountCameOnline(bool)));
    }
}

void ContactNotify::accountCameOnline(bool online)
{
    if (online) {
        const Tp::AccountPtr *account = (Tp::AccountPtr *)QObject::sender();
        accountIsOnline(account);
    }
}

void ContactNotify::accountIsOnline(const Tp::AccountPtr *account)
{
    Tp::Contacts contacts = account->data()->connection()->contactManager()->allKnownContacts();
    Q_FOREACH (const Tp::ContactPtr &contact, contacts) {
        connect(contact.data(), SIGNAL(presenceChanged(Tp::Presence)),
                SLOT(contactPresenceChanged(Tp::Presence)));
    }
}

void ContactNotify::contactPresenceChanged(Tp::Presence presence)
{
    Tp::Contact *contact = (Tp::Contact *)QObject::sender();
    KTp::Presence ktpPresence(presence);
    sendNotification(i18nc("%1 is the contact name, %2 is the presence message",
                           "%1 is now %2",
                           contact->alias(),
                           ktpPresence.displayString()),
                     ktpPresence.icon());
}

void ContactNotify::sendNotification(QString text, KIcon icon)
{
    //The pointer is automatically deleted when the event is closed
    KNotification *notification;
    notification = new KNotification(QLatin1String("telepathyInfo"), KNotification::CloseOnTimeout);

    KAboutData aboutData("ktelepathy",0,KLocalizedString(),0);
    notification->setComponentData(KComponentData(aboutData));

    notification->setPixmap(icon.pixmap(48));
    notification->setText(text);
    notification->sendEvent();
}
