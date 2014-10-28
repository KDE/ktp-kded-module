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

#include <KTp/core.h>
#include <KTp/presence.h>
#include <KTp/global-contact-manager.h>

#include <TelepathyQt/ContactManager>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Contact>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KNotification>

ContactNotify::ContactNotify(QObject *parent) :
    QObject(parent)
{
    KTp::GlobalContactManager *contactManager = KTp::contactManager();
    Tp::Presence currentPresence;

    Q_FOREACH(const Tp::ContactPtr &contact, contactManager->allKnownContacts()) {
        connect(contact.data(), SIGNAL(presenceChanged(Tp::Presence)),
                SLOT(contactPresenceChanged(Tp::Presence)));

        currentPresence = contact->presence();
        m_presenceHash[contact->id()] = KTp::Presence::sortPriority(currentPresence.type());
    }

    connect(contactManager, SIGNAL(allKnownContactsChanged(Tp::Contacts,Tp::Contacts)),
            SLOT(onContactsChanged(Tp::Contacts,Tp::Contacts)));
}


void ContactNotify::contactPresenceChanged(const Tp::Presence &presence)
{
    KTp::Presence ktpPresence(presence);
    KTp::ContactPtr contact(qobject_cast<KTp::Contact*>(QObject::sender()));
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
                         contact->avatarPixmap(),
                         contact);
    }

    m_presenceHash.insert(contact->id(), KTp::Presence::sortPriority(presence.type()));
}

void ContactNotify::sendNotification(const QString &text, const QPixmap &pixmap, const Tp::ContactPtr &contact)
{
    //The pointer is automatically deleted when the event is closed
    KNotification *notification;
    notification = new KNotification(QLatin1String("contactInfo"), KNotification::CloseOnTimeout);

    notification->setComponentName(QStringLiteral("ktelepathy"));

    notification->setPixmap(pixmap);
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
        connect(contact.data(), SIGNAL(avatarTokenChanged(QString)),
                SLOT(contactAvatarTokenChanged(QString)));

        currentPresence = contact->presence();
        m_presenceHash[contact->id()] = KTp::Presence::sortPriority(currentPresence.type());

    }

    Q_FOREACH(const Tp::ContactPtr &contact, contactsRemoved) {
        m_presenceHash.remove(contact->id());
    }
 }

void ContactNotify::saveAvatarTokens()
{
    KSharedConfigPtr config = KSharedConfig::openConfig(QLatin1String("ktelepathy-avatarsrc"));

    QHashIterator<QString, QString> i(m_avatarTokensHash);

    while (i.hasNext()) {
        i.next();
        KConfigGroup avatarsGroup = config->group(i.key());
        avatarsGroup.writeEntry(QLatin1String("avatarToken"), i.value());
    }

    config->sync();
}

void ContactNotify::contactAvatarTokenChanged(const QString &avatarToken)
{
    Tp::ContactPtr contact(qobject_cast<Tp::Contact*>(sender()));

    if (!contact) {
        return;
    }

    m_avatarTokensHash[contact->id()] = avatarToken;
    QTimer::singleShot(0, this, SLOT(saveAvatarTokens()));
}
