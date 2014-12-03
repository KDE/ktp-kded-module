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
#include "ktp_kded_debug.h"

#include <KTp/error-dictionary.h>
#include <KTp/core.h>
#include <KTp/Widgets/contact-info-dialog.h>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Connection>
#include <TelepathyQt/Contact>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingComposite>
#include <TelepathyQt/PendingOperation>

#include <KStatusNotifierItem>
#include <KLocalizedString>

#include <QAction>
#include <QIcon>
#include <QFutureWatcher>
#include <QtConcurrentFilter>
#include <QMenu>

Q_DECLARE_METATYPE(Tp::ContactPtr)

static bool kde_tp_filter_contacts_by_publication_status(const Tp::ContactPtr &contact)
{
    return contact->publishState() == Tp::Contact::PresenceStateAsk && !contact->isBlocked();
}

ContactRequestHandler::ContactRequestHandler(QObject *parent)
    : QObject(parent)
{
    connect(KTp::accountManager().data(), SIGNAL(newAccount(Tp::AccountPtr)),
            this, SLOT(onNewAccountAdded(Tp::AccountPtr)));

    QList<Tp::AccountPtr> accounts = KTp::accountManager()->allAccounts();

    Q_FOREACH(const Tp::AccountPtr &account, accounts) {
        onNewAccountAdded(account);
    }

}

ContactRequestHandler::~ContactRequestHandler()
{

}

void ContactRequestHandler::onNewAccountAdded(const Tp::AccountPtr &account)
{
    qCWarning(KTP_KDED_MODULE);
    Q_ASSERT(account->isReady(Tp::Account::FeatureCore));

    if (account->connection()) {
        handleNewConnection(account->connection());
    }

    connect(account.data(),
            SIGNAL(connectionChanged(Tp::ConnectionPtr)),
            this, SLOT(onConnectionChanged(Tp::ConnectionPtr)));
}

void ContactRequestHandler::onConnectionChanged(const Tp::ConnectionPtr &connection)
{
    if (!connection.isNull()) {
        handleNewConnection(connection);
    }
}

void ContactRequestHandler::handleNewConnection(const Tp::ConnectionPtr &connection)
{
    qCDebug(KTP_KDED_MODULE);
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

        qCDebug(KTP_KDED_MODULE) << "Watcher is on";
    } else {
        qCDebug(KTP_KDED_MODULE) << "Watcher still off, state is" << state << "contactManager is" << contactManager.isNull();
    }
}

void ContactRequestHandler::onAccountsPresenceStatusFiltered()
{
    qCDebug(KTP_KDED_MODULE) << "Watcher is here";
    QFutureWatcher< Tp::ContactPtr > *watcher = dynamic_cast< QFutureWatcher< Tp::ContactPtr > * >(sender());
    qCDebug(KTP_KDED_MODULE) << "Watcher is casted";
    Tp::Contacts contacts = watcher->future().results().toSet();
    qCDebug(KTP_KDED_MODULE) << "Watcher is used";
    if (!contacts.isEmpty()) {
        onPresencePublicationRequested(contacts);
    }
    watcher->deleteLater();
}

void ContactRequestHandler::onPresencePublicationRequested(const Tp::Contacts &contacts)
{
    qCDebug(KTP_KDED_MODULE) << "New contact requested";

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

            connect(contact.data(), SIGNAL(invalidated()), this, SLOT(onContactInvalidated()));

            updateMenus();

            if (!m_notifierItem.isNull()) {
                m_notifierItem.data()->showMessage(i18n("New contact request"),    //krazy:exclude=qmethods
                                                   i18n("The contact %1 wants to be able to chat with you.",
                                                        contact->id()),
                                                   QLatin1String("list-add-user"));
            }
        }
    }
}

void ContactRequestHandler::onFinalizeSubscriptionFinished(Tp::PendingOperation *op)
{
    Tp::ContactPtr contact = op->property("__contact").value< Tp::ContactPtr >();

    Q_ASSERT(!contact.isNull());

    if (op->isError()) {
        // ARGH
        if (!m_notifierItem.isNull()) {
            m_notifierItem.data()->showMessage(i18n("Error adding contact"),
                                               i18n("%1 has been added successfully to your contact list, "
                                                    "but might be unable to see when you are online. Error details: %2",
                                                    contact->alias(), KTp::ErrorDictionary::displayVerboseErrorMessage(op->errorName())),
                                               QLatin1String("dialog-error"));
        }
    } else {
        // Update the menu
        m_pendingContacts.remove(contact->id());
        updateMenus();
    }
}

void ContactRequestHandler::onContactInvalidated()
{
    Tp::ContactPtr contact = Tp::ContactPtr(qobject_cast<Tp::Contact*>(sender()));

    m_pendingContacts.remove(contact->id());
    updateMenus();
}

void ContactRequestHandler::onNotifierActivated(bool active, const QPoint &pos)
{
    if (active) {
        if (m_notifierItem) {
            m_notifierItem.data()->contextMenu()->popup(pos);
        }
    }
}

void ContactRequestHandler::onContactRequestApproved()
{
    QString contactId = qobject_cast<QAction*>(sender())->data().toString();

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
            }
            ++i;
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

void ContactRequestHandler::onShowContactDetails()
{
    QString contactId = qobject_cast<QAction*>(sender())->data().toString();

    if (!contactId.isEmpty()) {
        const Tp::ContactPtr contact = m_pendingContacts.find(contactId).value();
        const Tp::ContactManagerPtr manager = contact->manager();
        Q_FOREACH (const Tp::AccountPtr &account, KTp::accountManager()->allAccounts()) {
            if (account->connection() == manager->connection()) {
                KTp::ContactInfoDialog *dialog = new KTp::ContactInfoDialog(account, contact);
                connect(dialog, SIGNAL(closeClicked()), dialog, SLOT(deleteLater()));
                dialog->show();
                break;
            }
        }
    }
}

void ContactRequestHandler::onAuthorizePresencePublicationFinished(Tp::PendingOperation *op)
{
    Tp::ContactPtr contact = op->property("__contact").value< Tp::ContactPtr >();

    if (op->isError()) {
        if (!m_notifierItem.isNull()) {
            m_notifierItem.data()->showMessage(i18n("Error granting contact authorization"),
                                               i18n("There was an error while accepting the request: %1",
                                                    KTp::ErrorDictionary::displayVerboseErrorMessage(op->errorName())),
                                               QLatin1String("dialog-error"));
        }

        // Re-enable the action
        m_menuItems.value(contact->id())->setEnabled(true);
    } else {
        // op succeeded
        if (!m_notifierItem.isNull()) {
            m_notifierItem.data()->showMessage(i18n("Contact request accepted"),
                                               i18n("%1 will now be able to see when you are online",
                                                    contact->alias()), QLatin1String("dialog-ok-apply"));
        }

        // If needed, reiterate the request on the other end
        if (contact->manager()->canRequestPresenceSubscription() &&
                contact->subscriptionState() == Tp::Contact::PresenceStateNo) {

            Tp::PendingOperation *op = contact->manager()->requestPresenceSubscription(QList< Tp::ContactPtr >() << contact);
            op->setProperty("__contact", QVariant::fromValue(contact));

            connect(op,
                    SIGNAL(finished(Tp::PendingOperation*)),
                    this, SLOT(onFinalizeSubscriptionFinished(Tp::PendingOperation*)));
        } else {
            // Update the menu
            m_pendingContacts.remove(contact->id());
            updateMenus();
        }
    }
}

void ContactRequestHandler::onContactRequestDenied()
{
    QString contactId = qobject_cast<QAction*>(sender())->data().toString();

    // Disable the action in the meanwhile
    m_menuItems.value(contactId)->setEnabled(false);

    if (!contactId.isEmpty()) {
        QList<Tp::PendingOperation*> operations;
        QHash<QString, Tp::ContactPtr>::const_iterator i = m_pendingContacts.constFind(contactId);
        while (i != m_pendingContacts.constEnd() && i.key() == contactId) {
            if (!i.value()->manager().isNull()) {
                //don't publish our presence to that user
                Tp::PendingOperation *op = i.value()->manager()->removePresencePublication(QList< Tp::ContactPtr >() << i.value());
                op->setProperty("__contact", QVariant::fromValue(i.value()));
                operations.append(op);

                //and block that contact
                if (i.value()->manager()->canBlockContacts()) {
                    Tp::PendingOperation *blockOp = i.value()->manager()->blockContacts(QList<Tp::ContactPtr>() << i.value());
                    operations.append(blockOp);
                }
            }
            ++i;
        }

        // Wait until all operations complete
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
        if (!m_notifierItem.isNull()) {
            m_notifierItem.data()->showMessage(i18n("Contact request denied"),
                                               i18n("%1 will not be able to see when you are online",
                                                    contact->alias()), QLatin1String("dialog-information"));
        }
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
        m_notifierItem.data()->setStatus(KStatusNotifierItem::Active);

        QMenu *notifierMenu = new QMenu(0);
        notifierMenu->setTitle(i18nc("Context menu title", "Received contact requests"));

        connect(m_notifierItem.data(), SIGNAL(activateRequested(bool,QPoint)), SLOT(onNotifierActivated(bool,QPoint)));

        m_notifierItem.data()->setContextMenu(notifierMenu);
    }

    qCDebug(KTP_KDED_MODULE) << m_pendingContacts.keys();

    //add members in pending contacts not in the menu to the menu.
    QHash<QString, Tp::ContactPtr>::const_iterator i;
    for (i = m_pendingContacts.constBegin(); i != m_pendingContacts.constEnd(); ++i) {
        if (m_menuItems.contains(i.key())) {
            // Skip
            continue;
        }

        qCDebug(KTP_KDED_MODULE);
        Tp::ContactPtr contact = i.value();

        QMenu *contactMenu = new QMenu(m_notifierItem.data()->contextMenu());
        contactMenu->setTitle(i18n("Request from %1", contact->alias()));
        contactMenu->setObjectName(contact->id());

        QAction *menuAction;

        menuAction = new QAction(QIcon(QLatin1String("user-identity")), i18n("Contact Details"), contactMenu);
        menuAction->setData(i.key());
        connect(menuAction, SIGNAL(triggered()),
                this, SLOT(onShowContactDetails()));
        contactMenu->addAction(menuAction);

        if (!contact->publishStateMessage().isEmpty()) {
            contactMenu->insertSection(menuAction, contact->publishStateMessage());
        } else {
            contactMenu->insertSection(menuAction, contact->alias());
        }

        contactMenu->addSeparator();

        menuAction = new QAction(QIcon::fromTheme(QLatin1String("dialog-ok-apply")), i18n("Approve"), contactMenu);
        menuAction->setData(i.key());
        connect(menuAction, SIGNAL(triggered()),
                this, SLOT(onContactRequestApproved()));
        contactMenu->addAction(menuAction);

        menuAction = new QAction(QIcon::fromTheme(QLatin1String("dialog-close")), i18n("Deny"), contactMenu);
        menuAction->setData(i.key());
        connect(menuAction, SIGNAL(triggered()),
                this, SLOT(onContactRequestDenied()));
        contactMenu->addAction(menuAction);

        m_notifierItem.data()->contextMenu()->addMenu(contactMenu);
        m_menuItems.insert(i.key(), contactMenu);
    }

    //remove items that are still in the menu, but not in pending contacts
    QHash<QString, QMenu*>::iterator j = m_menuItems.begin();
    while (j != m_menuItems.end()) {
        if (m_pendingContacts.contains(j.key())) {
            // Skip
            ++j;
            continue;
        }

        // Remove
        m_notifierItem.data()->contextMenu()->removeAction(j.value()->menuAction());
        j = m_menuItems.erase(j);
    }

    if (m_menuItems.size() > 0) {
        //if menu still contains items, update the tooltip to have the correct number
        m_notifierItem.data()->setToolTip(QLatin1String("list-add-user"),
                                          i18np("You have 1 contact wanting to chat with you",
                                                "You have %1 contacts wanting to chat with you",
                                                m_menuItems.size()),
                                          QString());
    } else {
        //if empty delete the status notifier item
        m_notifierItem.data()->deleteLater();
    }

}

#include "contact-request-handler.moc"
