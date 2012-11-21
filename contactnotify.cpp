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
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Contact>

#include <KNotification>
#include <KAboutData>

#include <KTp/presence.h>
#include <KTp/global-contact-manager.h>

using namespace KTp;

ContactNotify::ContactNotify(const Tp::AccountManagerPtr &accountMgr, QObject *parent) :
    QObject(parent)
{
    Q_ASSERT(accountMgr);
    m_accountManager = accountMgr;
    if (!m_accountManager) {
        return;
    }

    GlobalContactManager *contactManager = new GlobalContactManager(m_accountManager, this);

    Tp::Presence currentPresence;

    Q_FOREACH(const Tp::ContactPtr &contact, contactManager->allKnownContacts()) {
        connect(contact.data(), SIGNAL(presenceChanged(Tp::Presence)),
                SLOT(contactPresenceChanged(Tp::Presence)));

        currentPresence = contact->presence();
        m_presenceHash[contact->id()] = Presence::sortPriority(currentPresence.type());
    }

    connect(contactManager, SIGNAL(allKnownContactsChanged(Tp::Contacts,Tp::Contacts)),
            SLOT(onContactsChanged(Tp::Contacts,Tp::Contacts)));
}


void ContactNotify::contactPresenceChanged(const Tp::Presence &presence)
{
    KTp::Presence ktpPresence(presence);
    Tp::ContactPtr contact(qobject_cast<Tp::Contact*>(QObject::sender()));
    int priority = m_presenceHash[contact->id()];

    // Don't show presence messages when moving from a higher priority to a lower
    // priority, for eg : When a contact changes from Online -> Away, don't send
    // a notification, but do send a notification when a contact changes from
    // Away -> Online
    if (KTp::Presence::sortPriority(presence.type()) < priority) {
        sendNotification(i18nc("%1 is the contact name, %2 is the presence name",
                               "%1 is now %2",
                               contact->alias(),
                               ktpPresence.displayString()),
                         ktpPresence.icon(),
                         contact);
    }

    m_presenceHash.insert(contact->id(), Presence::sortPriority(presence.type()));
}

void ContactNotify::sendNotification(const QString &text, const KIcon &icon, const Tp::ContactPtr &contact)
{
    //The pointer is automatically deleted when the event is closed
    KNotification *notification;
    notification = new KNotification(QLatin1String("contactInfo"), KNotification::CloseOnTimeout);

    KAboutData aboutData("ktelepathy", 0, KLocalizedString(), 0);
    notification->setComponentData(KComponentData(aboutData));

    notification->setPixmap(icon.pixmap(48));
    notification->setText(text);
    notification->addContext(QLatin1String("contact"), contact.data()->id());
    notification->sendEvent();
}

void ContactNotify::onContactsChanged(const Tp::Contacts &contactsAdded, const Tp::Contacts &contactsRemoved)
{
    Tp::Presence currentPresence;

    Q_FOREACH(const Tp::ContactPtr &contact, contactsAdded) {
        connect(contact.data(), SIGNAL(presenceChanged(Tp::Presence)),
                SLOT(contactPresenceChanged(Tp::Presence)));

        currentPresence = contact->presence();
        m_presenceHash[contact->id()] = Presence::sortPriority(currentPresence.type());

    }

    Q_FOREACH(const Tp::ContactPtr &contact, contactsRemoved) {
        m_presenceHash.remove(contact->id());
    }
 }
