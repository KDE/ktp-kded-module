/*
    Copyright (C) 2011  Martin Klapetek <martin.klapetek@gmail.com>
    Copyright (C) 2011  Dario Freddi <dario.freddi@collabora.com>

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


#include "contact-request-handler.h"

#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/Account>

#include <KTp/error-dictionary.h>

#include <QtCore/QFutureWatcher>

#include <KDebug>
#include <KGlobal>
#include <KAboutData>
#include <KMenu>
#include <KAction>
#include <KStatusNotifierItem>

Q_DECLARE_METATYPE(Tp::ContactPtr)

static bool kde_tp_filter_contacts_by_publication_status(const Tp::ContactPtr &contact)
{
    return contact->publishState() == Tp::Contact::PresenceStateAsk;
}

ContactRequestHandler::ContactRequestHandler(const Tp::AccountManagerPtr& am, QObject *parent)
    : QObject(parent)
{
    m_accountManager = am;
    connect(m_accountManager.data(), SIGNAL(newAccount(Tp::AccountPtr)),
            this, SLOT(onNewAccountAdded(Tp::AccountPtr)));

    QList<Tp::AccountPtr> accounts = m_accountManager->allAccounts();

    Q_FOREACH(const Tp::AccountPtr &account, accounts) {
        onNewAccountAdded(account);
    }

    m_noContactsAction = new KAction(i18n("No pending contact requests at the moment"), this);
    m_noContactsAction->setEnabled(false);
}

ContactRequestHandler::~ContactRequestHandler()
{

}

void ContactRequestHandler::onNewAccountAdded(const Tp::AccountPtr& account)
{
    kWarning();
    Q_ASSERT(account->isReady(Tp::Account::FeatureCore));

    if (account->connection()) {
        monitorPresence(account->connection());
    }

    connect(account.data(),
            SIGNAL(connectionChanged(Tp::ConnectionPtr)),
            this, SLOT(onConnectionChanged(Tp::ConnectionPtr)));
}

void ContactRequestHandler::onConnectionChanged(const Tp::ConnectionPtr& connection)
{
    if (!connection.isNull()) {
        monitorPresence(connection);
    }
}

void ContactRequestHandler::monitorPresence(const Tp::ConnectionPtr &connection)
{
    kDebug();
    connect(connection->contactManager().data(), SIGNAL(presencePublicationRequested(Tp::Contacts)),
            this, SLOT(onPresencePublicationRequested(Tp::Contacts)));

    connect(connection->contactManager().data(),
            SIGNAL(stateChanged(Tp::ContactListState)),
            this, SLOT(onContactManagerStateChanged(Tp::ContactListState)));

    onContactManagerStateChanged(connection->contactManager(),
                                 connection->contactManager()->state());
}

void ContactRequestHandler::onContactManagerStateChanged(Tp::ContactListState state)
{
    onContactManagerStateChanged(Tp::ContactManagerPtr(qobject_cast< Tp::ContactManager* >(sender())), state);
}

void ContactRequestHandler::onContactManagerStateChanged(const Tp::ContactManagerPtr &contactManager,
                                                         Tp::ContactListState state)
{
    if (state == Tp::ContactListStateSuccess) {
        QFutureWatcher< Tp::ContactPtr > *watcher = new QFutureWatcher< Tp::ContactPtr >(this);
        connect(watcher, SIGNAL(finished()), this, SLOT(onAccountsPresenceStatusFiltered()));
        watcher->setFuture(QtConcurrent::filtered(contactManager->allKnownContacts(),
                                                  kde_tp_filter_contacts_by_publication_status));

        kDebug() << "Watcher is on";
    } else {
        kDebug() << "Watcher still off, state is" << state << "contactManager is" << contactManager.isNull();
    }
}

void ContactRequestHandler::onAccountsPresenceStatusFiltered()
{
    kDebug() << "Watcher is here";
    QFutureWatcher< Tp::ContactPtr > *watcher = dynamic_cast< QFutureWatcher< Tp::ContactPtr > * >(sender());
    kDebug() << "Watcher is casted";
    Tp::Contacts contacts = watcher->future().results().toSet();
    kDebug() << "Watcher is used";
    if (!contacts.isEmpty()) {
        onPresencePublicationRequested(contacts);
    }
    watcher->deleteLater();
}

void ContactRequestHandler::onPresencePublicationRequested(const Tp::Contacts& contacts)
{
    kDebug() << "New contact requested";

    Q_FOREACH (const Tp::ContactPtr &contact, contacts) {
        Tp::ContactManagerPtr manager = contact->manager();

        if (contact->subscriptionState() == Tp::Contact::PresenceStateYes) {
            Tp::PendingOperation *op = manager->authorizePresencePublication(QList< Tp::ContactPtr >() << contact);
            op->setProperty("__contact", QVariant::fromValue(contact));

            connect(op, SIGNAL(finished(Tp::PendingOperation*)),
                    this, SLOT(onFinalizeSubscriptionFinished(Tp::PendingOperation*)));
        } else {
            // Handle multiaccount requests properly
            if (m_pendingContacts.contains(contact->id())) {
                // It's likely we have a simultaneous request
                bool newReq = true;
                QHash<QString, Tp::ContactPtr>::const_iterator i = m_pendingContacts.constFind(contact->id());
                while (i != m_pendingContacts.constEnd() && i.key() == contact->id()) {
                    if (i.value().data() == contact.data()) {
                        newReq = false;
                        break;
                    }
                    ++i;
                }

                if (newReq) {
                    // Insert multi
                    m_pendingContacts.insertMulti(contact->id(), contact);
                }
            } else {
                // Simple insertion
                m_pendingContacts.insert(contact->id(), contact);
            }

            updateMenus();

            m_notifierItem.data()->showMessage(i18n("New contact request"),    //krazy:exclude=qmethods
                                               i18n("The contact %1 added you to its contact list. You can answer this "
                                                    "request using the tray icon.",
                                                    contact->id()),
                                               QLatin1String("list-add-user"));
        }
    }
}

void ContactRequestHandler::onFinalizeSubscriptionFinished(Tp::PendingOperation *op)
{
    Tp::ContactPtr contact = op->property("__contact").value< Tp::ContactPtr >();

    if (op->isError()) {
        // ARGH
        m_notifierItem.data()->showMessage(i18n("Error adding contact"),
                                           i18n("%1 has been added successfully to your contact list, "
                                                "but might be unable to see your presence. Error details: %2",
                                                contact->alias(), KTp::ErrorDictionary::displayVerboseErrorMessage(op->errorName())),
                                                QLatin1String("dialog-error"));
    } else {
        // Yeah. All fine, so don't notify
    }
}

void ContactRequestHandler::updateNotifierItemTooltip()
{
    if (!m_menuItems.size()) {
        // Set passive
        m_notifierItem.data()->setStatus(KStatusNotifierItem::Passive);
        m_notifierItem.data()->setIconByName(QLatin1String("user-identity"));
        // Add the usual "nothing" action, if needed
        if (!m_notifierMenu->actions().contains(m_noContactsAction)) {
            m_notifierMenu->addAction(m_noContactsAction);
        }

        m_notifierItem.data()->setToolTip(QLatin1String("list-add-user"),
                                          i18n("No incoming contact requests"),
                                          QString());
    } else {
        // Set active
        m_notifierItem.data()->setStatus(KStatusNotifierItem::Active);
        m_notifierItem.data()->setIconByName(QLatin1String("list-add-user"));
        // Remove the "nothing" action, if needed
        if (m_notifierMenu->actions().contains(m_noContactsAction)) {
            m_notifierMenu->removeAction(m_noContactsAction);
        }

        m_notifierItem.data()->setToolTip(QLatin1String("list-add-user"),
                                          i18np("You have 1 incoming contact request",
                                                "You have %1 incoming contact requests",
                                                m_menuItems.size()),
                                          QString());
    }
}

void ContactRequestHandler::onContactRequestApproved()
{
    QString contactId = qobject_cast<KAction*>(sender())->data().toString();

    // Disable the action in the meanwhile
    m_menuItems.value(contactId)->setEnabled(false);

    if (!contactId.isEmpty()) {
        QList<Tp::PendingOperation*> operations;
        QHash<QString, Tp::ContactPtr>::const_iterator i = m_pendingContacts.constFind(contactId);
        while (i != m_pendingContacts.constEnd() && i.key() == contactId) {
            if (!i.value()->manager().isNull()) {
                Tp::PendingOperation *op = i.value()->manager()->authorizePresencePublication(QList< Tp::ContactPtr >() << i.value());
                op->setProperty("__contact", QVariant::fromValue(i.value()));
                operations.append(op);
                ++i;
            }
        }

        // Take the first value, if any
        if (!operations.isEmpty()) {
            Tp::ContactPtr contact = m_pendingContacts.find(contactId).value();

            Tp::PendingComposite *op = new Tp::PendingComposite(operations, true, contact);
            op->setProperty("__contact", QVariant::fromValue(contact));

            connect(op, SIGNAL(finished(Tp::PendingOperation*)),
                    this, SLOT(onAuthorizePresencePublicationFinished(Tp::PendingOperation*)));
        }
    }

}

void ContactRequestHandler::onAuthorizePresencePublicationFinished(Tp::PendingOperation *op)
{
    Tp::ContactPtr contact = op->property("__contact").value< Tp::ContactPtr >();

    if (op->isError()) {
        // ARGH
        m_notifierItem.data()->showMessage(i18n("Error accepting contact request"),
                                           i18n("There was an error while accepting the request: %1",
                                                KTp::ErrorDictionary::displayVerboseErrorMessage(op->errorName())),
                                           QLatin1String("dialog-error"));

        // Re-enable the action
        m_menuItems.value(contact->id())->setEnabled(true);
    } else {
        // Yeah
        m_notifierItem.data()->showMessage(i18n("Contact request accepted"),
                                           i18n("%1 will now be able to see your presence",
                                                contact->alias()), QLatin1String("dialog-ok-apply"));

        // If needed, reiterate the request on the other end
        if (contact->manager()->canRequestPresenceSubscription() &&
            contact->subscriptionState() == Tp::Contact::PresenceStateNo) {
            connect(contact->manager()->requestPresenceSubscription(QList< Tp::ContactPtr >() << contact),
                    SIGNAL(finished(Tp::PendingOperation*)),
                    this, SLOT(onFinalizeSubscriptionFinished(Tp::PendingOperation*)));
        }

        // Update the menu
        m_pendingContacts.remove(contact->id());
        updateMenus();
    }
}

void ContactRequestHandler::onContactRequestDenied()
{
    QString contactId = qobject_cast<KAction*>(sender())->data().toString();

    // Disable the action in the meanwhile
    m_menuItems.value(contactId)->setEnabled(false);

    if (!contactId.isEmpty()) {
        QList<Tp::PendingOperation*> operations;
        QHash<QString, Tp::ContactPtr>::const_iterator i = m_pendingContacts.constFind(contactId);
        while (i != m_pendingContacts.constEnd() && i.key() == contactId) {
            if (!i.value()->manager().isNull()) {
                Tp::PendingOperation *op = i.value()->manager()->removePresencePublication(QList< Tp::ContactPtr >() << i.value());
                op->setProperty("__contact", QVariant::fromValue(i.value()));
                operations.append(op);
            }
            ++i;
        }

        // Take the first value, if any
        if (!operations.isEmpty()) {
            Tp::ContactPtr contact = m_pendingContacts.find(contactId).value();

            Tp::PendingComposite *op = new Tp::PendingComposite(operations, true, contact);
            op->setProperty("__contact", QVariant::fromValue(contact));

            connect(op, SIGNAL(finished(Tp::PendingOperation*)),
                    this, SLOT(onRemovePresencePublicationFinished(Tp::PendingOperation*)));
        }
    }
}

void ContactRequestHandler::onRemovePresencePublicationFinished(Tp::PendingOperation *op)
{
    Tp::ContactPtr contact = op->property("__contact").value< Tp::ContactPtr >();

    if (op->isError()) {
        // ARGH
        m_notifierItem.data()->showMessage(i18n("Error denying contact request"),
                                           i18n("There was an error while denying the request: %1",
                                                KTp::ErrorDictionary::displayVerboseErrorMessage(op->errorName())),
                                                QLatin1String("dialog-error"));

        // Re-enable the action
        m_menuItems.value(contact->id())->setEnabled(true);
    } else {
        // Yeah
        m_notifierItem.data()->showMessage(i18n("Contact request denied"),
                                           i18n("%1 will not be able to see your presence",
                                                contact->alias()), QLatin1String("dialog-ok-apply"));

        // Update the menu
        m_pendingContacts.remove(contact->id());
        updateMenus();
    }
}

void ContactRequestHandler::updateMenus()
{
    if (m_notifierItem.isNull()) {
        m_notifierItem = new KStatusNotifierItem(QLatin1String("telepathy_kde_contact_requests"), this);
        m_notifierItem.data()->setCategory(KStatusNotifierItem::Communications);
        m_notifierItem.data()->setIconByName(QLatin1String("user-identity"));
        m_notifierItem.data()->setAttentionIconByName(QLatin1String("list-add-user"));
        m_notifierItem.data()->setStandardActionsEnabled(false);
        m_notifierItem.data()->setTitle(i18nc("Menu title", "Pending contact requests"));

        m_notifierMenu = new KMenu(0);
        m_notifierMenu->addTitle(i18nc("Context menu title", "Received contact requests"));

        m_notifierItem.data()->setContextMenu(m_notifierMenu);
    }

    kDebug() << m_pendingContacts.keys();

    QHash<QString, Tp::ContactPtr>::const_iterator i;
    for (i = m_pendingContacts.constBegin(); i != m_pendingContacts.constEnd(); ++i) {
        if (m_menuItems.contains(i.key())) {
            // Skip
            continue;
        }

        kDebug();
        Tp::ContactPtr contact = i.value();

        KMenu *contactMenu = new KMenu(m_notifierMenu);
        contactMenu->setTitle(i18n("Request from %1", contact->alias()));
        contactMenu->setObjectName(contact->id());

        KAction *menuAction;
        if (!contact->publishStateMessage().isEmpty()) {
            contactMenu->addTitle(contact->publishStateMessage());
        } else {
            contactMenu->addTitle(contact->alias());
        }
        menuAction = new KAction(KIcon(QLatin1String("dialog-ok-apply")), i18n("Approve"), contactMenu);
        menuAction->setData(i.key());
        connect(menuAction, SIGNAL(triggered()),
                this, SLOT(onContactRequestApproved()));
        contactMenu->addAction(menuAction);

        menuAction = new KAction(KIcon(QLatin1String("dialog-close")), i18n("Deny"), contactMenu);
        menuAction->setData(i.key());
        connect(menuAction, SIGNAL(triggered()),
                this, SLOT(onContactRequestDenied()));
        contactMenu->addAction(menuAction);

        m_notifierMenu->addMenu(contactMenu);
        m_menuItems.insert(i.key(), contactMenu);
    }

    QHash<QString, KMenu*>::iterator j = m_menuItems.begin();
    while (j != m_menuItems.end()) {
        if (m_pendingContacts.contains(j.key())) {
            // Skip
            ++j;
            continue;
        }

        // Remove
        m_notifierMenu->removeAction(j.value()->menuAction());
        j = m_menuItems.erase(j);
    }

    updateNotifierItemTooltip();
}

#include "contact-request-handler.moc"
