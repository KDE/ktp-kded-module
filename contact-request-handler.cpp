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


#include "contact-request-handler.h"

#include <TelepathyQt4/Connection>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/Account>

#include <KDebug>
#include <KGlobal>
#include <KAboutData>

#include <QFutureWatcher>
#include <KMenu>
#include <KAction>

bool kde_tp_filter_contacts_by_publication_status(const Tp::ContactPtr &contact)
{
    return contact->publishState() == Tp::Contact::PresenceStateAsk;
}

ContactRequestHandler::ContactRequestHandler(const Tp::AccountManagerPtr& am, QObject *parent)
    : QObject(parent)
{
    m_notifierMenu = new KMenu(0);
    m_notifierMenu->addTitle(i18nc("Context menu title", "Received contact requests"));
    m_accountManager = am;
    connect(m_accountManager.data(), SIGNAL(newAccount(Tp::AccountPtr)),
            this, SLOT(onNewAccountAdded(Tp::AccountPtr)));

    QList<Tp::AccountPtr> accounts = m_accountManager->allAccounts();

    Q_FOREACH(const Tp::AccountPtr &account, accounts) {
        onNewAccountAdded(account);
    }
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

void ContactRequestHandler::onContactManagerStateChanged(const Tp::ContactManagerPtr &contactManager, Tp::ContactListState state)
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
    kWarning() << "New contact requested";

    Q_FOREACH (const Tp::ContactPtr &contact, contacts) {
        Tp::ContactManagerPtr manager = contact->manager();
        Tp::PendingOperation *op = 0;

        if (contact->subscriptionState() == Tp::Contact::PresenceStateYes) {
            op = manager->authorizePresencePublication(QList< Tp::ContactPtr >() << contact);
        } else {
            KNotification *notification = new KNotification(QLatin1String("telepathyInfo"), KNotification::CloseOnTimeout);

            KAboutData aboutData("ktelepathy",0,KLocalizedString(),0);
            notification->setComponentData(KComponentData(aboutData));

            notification->setText(i18n("The contact %1 added you to their contact list. "
                                       "Do you want to allow this person to see your presence "
                                       "and add them to your contact list?", contact->id()));
            notification->sendEvent();

            m_pendingContacts.insert(contact->id(), contact);

            m_notifierItem = getNotifierItem();

            KMenu *contactMenu = new KMenu(m_notifierMenu);
            contactMenu->setTitle(i18n("Request from %1", contact->id()));
            contactMenu->setObjectName(contact->id());

            KAction *menuAction;
            if (!contact->publishStateMessage().isEmpty()) {
                contactMenu->addTitle(contact->publishStateMessage());
            } else {
                contactMenu->addTitle(contact->alias());
            }
            menuAction = new KAction(KIcon(QLatin1String("dialog-ok-apply")), i18n("Approve"), contactMenu);
            connect(menuAction, SIGNAL(triggered(bool)),
                    this, SLOT(onContactRequestApproved()));
            menuAction->setData(contact->id());
            contactMenu->addAction(menuAction);

            menuAction = new KAction(KIcon(QLatin1String("dialog-close")), i18n("Deny"), contactMenu);
            menuAction->setData(contact->id());

            connect(menuAction, SIGNAL(triggered(bool)),
                    this, SLOT(onContactRequestDenied()));
            contactMenu->addAction(menuAction);

            m_notifierMenu->addMenu(contactMenu);
            m_notifierItem->setContextMenu(m_notifierMenu);

            updateNotifierItemTooltip();
        }

        if (op) {
//             connect(op, SIGNAL(finished(Tp::PendingOperation*)),
//                     SLOT(onGenericOperationFinished(Tp::PendingOperation*)));
        }
    }
}

K_GLOBAL_STATIC(QWeakPointer<KStatusNotifierItem>, s_notifierItem)

//static
QSharedPointer<KStatusNotifierItem> ContactRequestHandler::getNotifierItem()
{
    QSharedPointer<KStatusNotifierItem> notifierItem = s_notifierItem->toStrongRef();
    if (!notifierItem) {
        notifierItem = QSharedPointer<KStatusNotifierItem>(new KStatusNotifierItem);
        notifierItem->setCategory(KStatusNotifierItem::Communications);
        notifierItem->setStatus(KStatusNotifierItem::NeedsAttention);
        notifierItem->setIconByName(QLatin1String("user-identity"));
        notifierItem->setAttentionIconByName(QLatin1String("list-add-user"));
        notifierItem->setStandardActionsEnabled(false);
        notifierItem->setProperty("contact_requests_count", 0U);
        *s_notifierItem = notifierItem;
    }

    return notifierItem;
}

void ContactRequestHandler::updateNotifierItemTooltip()
{
    QVariant requestsCount = m_notifierItem->property("contact_requests_count");
    requestsCount = QVariant(requestsCount.toUInt() + 1);
    m_notifierItem->setProperty("contact_requests_count", requestsCount);

    m_notifierItem->setToolTip(QLatin1String("list-add-user"),
                               i18np("You have 1 incoming contact request",
                                     "You have %1 incoming contact requests",
                                     requestsCount.toUInt()),
                               QString());
}

void ContactRequestHandler::onContactRequestApproved()
{
    QString contactId = qobject_cast<KAction*>(sender())->data().toString();

    if (!contactId.isEmpty()) {
        Tp::ContactPtr contact = m_pendingContacts.value(contactId);
        if (!contact.isNull()) {
            Tp::PendingOperation *op = contact->manager()->authorizePresencePublication(QList< Tp::ContactPtr >() << contact);
            //TODO: connect and let user know the result, find and remove the menu from statusnotifier
            //      don't forget to update the tooltip with -1
            //delete m_notifierMenu->findChild<KMenu*>(contactId);
            if (contact->manager()->canRequestPresenceSubscription() && contact->subscriptionState() == Tp::Contact::PresenceStateNo) {
                contact->manager()->requestPresenceSubscription(QList< Tp::ContactPtr >() << contact);
            }
        }
    }
}

void ContactRequestHandler::onContactRequestDenied()
{
QString contactId = qobject_cast<KAction*>(sender())->data().toString();

    if (!contactId.isEmpty()) {
        Tp::ContactPtr contact = m_pendingContacts.value(contactId);
        if (!contact.isNull()) {
            Tp::PendingOperation *op = contact->manager()->removePresencePublication(QList< Tp::ContactPtr >() << contact);
            //TODO: connect and let user know the result, find and remove the menu from statusnotifier
            //      don't forget to update the tooltip with -1
//             delete m_notifierMenu->findChild<KMenu*>(contactId);
        }
    }
}

#include "contact-request-handler.moc"