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

#include "contact-cache.h"

#include <KTp/core.h>
#include <KTp/contact.h>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AvatarData>
#include <TelepathyQt/Connection>
#include <TelepathyQt/ContactManager>
#include <TelepathyQt/PendingOperation>
#include <TelepathyQt/PendingReady>

#include <KDebug>
#include <KGlobal>
#include <KStandardDirs>

#include <QSqlQuery>
#include <QSqlDriver>
#include <QSqlField>

/*
 * This class waits for a connection to load then saves the pernament
 * data from all contacts into a database that can be loaded by the kpeople plugin
 * It will not stay up-to-date, applications should load from the database, then
 * fetch volatile and up-to-date data from TpQt
 *
 * We don't hold a reference to the contact to keep things light
 */

inline QString formatString(const QSqlQuery &query, const QString &str)
{
    QSqlField f(QLatin1String(""), QVariant::String);
    f.setValue(str);
    return query.driver()->formatValue(f);
}

ContactCache::ContactCache(QObject *parent):
    QObject(parent),
    m_db(QSqlDatabase::addDatabase(QLatin1String("QSQLITE")))
{
    m_db.setDatabaseName(KGlobal::dirs()->locateLocal("data", QLatin1String("ktp/cache.db")));
    m_db.open();

    if (!m_db.tables().contains(QLatin1String("groups"))) {
        QSqlQuery preparationsQuery(m_db);
        if (m_db.tables().contains(QLatin1String("contacts"))) {
            preparationsQuery.exec(QLatin1String("DROP TABLE contacts;"));
        }

        preparationsQuery.exec(QLatin1String("CREATE TABLE contacts (accountId VARCHAR NOT NULL, contactId VARCHAR NOT NULL, alias VARCHAR, avatarFileName VARCHAR, groupsIds VARCHAR);"));
        preparationsQuery.exec(QLatin1String("CREATE TABLE groups (groupId INTEGER, groupName VARCHAR);"));
        preparationsQuery.exec(QLatin1String("CREATE UNIQUE INDEX idIndex ON contacts (accountId, contactId);"));
    }

    connect(KTp::accountManager()->becomeReady(), SIGNAL(finished(Tp::PendingOperation*)), SLOT(onAccountManagerReady(Tp::PendingOperation*)));
}

void ContactCache::onAccountManagerReady(Tp::PendingOperation *op)
{
    if (!op || op->isError()) {
        kWarning() << "ContactCache: Failed to initialize AccountManager:" << op->errorName();
        kWarning() << op->errorMessage();

        return;
    }

    connect(KTp::accountManager().data(), SIGNAL(newAccount(Tp::AccountPtr)), SLOT(onNewAccount(Tp::AccountPtr)));

    QSqlQuery purgeQuery(m_db);
    QStringList formattedAccountsIds;

    Q_FOREACH (const Tp::AccountPtr &account, KTp::accountManager()->allAccounts()) {
        if (!accountIsInteresting(account)) {
            continue;
        }

        connectToAccount(account);
        if (!account->connection().isNull()) {
            onAccountConnectionChanged(account->connection());
        }

        formattedAccountsIds.append(formatString(purgeQuery, account->uniqueIdentifier()));
    }

    // Cleanup contacts
    if (formattedAccountsIds.isEmpty()) {
        purgeQuery.prepare(QLatin1String("DELETE * FROM contacts;"));
    } else {
        purgeQuery.prepare(QString(QLatin1String("DELETE FROM contacts WHERE accountId not in (%1);")).arg(formattedAccountsIds.join(QLatin1String(","))));
    }
    purgeQuery.exec();

    // Cleanup groups
    QStringList usedGroups;

    QSqlQuery usedGroupsQuery(m_db);
    usedGroupsQuery.prepare(QLatin1String("SELECT groupsIds FROM contacts;"));
    usedGroupsQuery.exec();

    while (usedGroupsQuery.next()) {
        usedGroups.append(usedGroupsQuery.value(0).toString().split(QLatin1String(",")));
    }
    usedGroups.removeDuplicates();

    purgeQuery.prepare(QString(QLatin1String("UPDATE groups SET groupName = '' WHERE groupId not in (%1);")).arg(usedGroups.join(QLatin1String(","))));
    purgeQuery.exec();

    // Load groups
    QSqlQuery groupsQuery(m_db);
    groupsQuery.exec(QLatin1String("SELECT groupName FROM groups ORDER BY groupId;"));

    while (groupsQuery.next()) {
        m_groups.append(groupsQuery.value(0).toString());
    }
}

void ContactCache::onNewAccount(const Tp::AccountPtr &account)
{
    if (!accountIsInteresting(account)) {
        return;
    }

    connectToAccount(account);
    if (!account->connection().isNull()) {
        onAccountConnectionChanged(account->connection());
    }
}

void ContactCache::onAccountRemoved()
{
    Tp::Account *account = qobject_cast<Tp::Account*>(sender());

    if (!account) {
        return;
    }

    QSqlQuery purgeQuery(m_db);
    purgeQuery.prepare(QLatin1String("DELETE FROM contacts WHERE accountId = ?;"));
    purgeQuery.bindValue(0, account->uniqueIdentifier());
    purgeQuery.exec();
}

void ContactCache::onContactManagerStateChanged()
{
    Tp::ContactManagerPtr contactManager(qobject_cast<Tp::ContactManager*>(sender()));
    checkContactManagerState(Tp::ContactManagerPtr(contactManager));
}

void ContactCache::onAccountConnectionChanged(const Tp::ConnectionPtr &connection)
{
    if (connection.isNull() || (connection->status() != Tp::ConnectionStatusConnected)) {
        return;
    }

    //this is needed to make the contact manager roster
    //when this finishes the contact manager will change state
    connection->becomeReady(Tp::Features() << Tp::Connection::FeatureRoster << Tp::Connection::FeatureRosterGroups);

    if (connect(connection->contactManager().data(), SIGNAL(stateChanged(Tp::ContactListState)), this, SLOT(onContactManagerStateChanged()), Qt::UniqueConnection)) {
        /* Check current contactManager state and do sync contact only if it is not performed due to already connected contactManager. */
        checkContactManagerState(connection->contactManager());
    }
}

void ContactCache::onAllKnownContactsChanged(const Tp::Contacts &added, const Tp::Contacts &removed)
{
    /* Delete both added and removed contacts, because it's faster than accurate comparsion and partial update of exist contacts. */
    Tp::Contacts toBeRemoved = added;
    toBeRemoved.unite(removed);

    m_db.transaction();
    QSqlQuery removeQuery(m_db);
    removeQuery.prepare(QLatin1String("DELETE FROM contacts WHERE accountId = ? AND contactId = ?;"));
    Q_FOREACH (const Tp::ContactPtr &c, toBeRemoved) {
        const KTp::ContactPtr &contact = KTp::ContactPtr::qObjectCast(c);
        removeQuery.bindValue(0, contact->accountUniqueIdentifier());
        removeQuery.bindValue(1, contact->id());
        removeQuery.exec();
    }

    QSqlQuery insertQuery(m_db);
    insertQuery.prepare(QLatin1String("INSERT INTO contacts (accountId, contactId, alias, avatarFileName, groupsIds) VALUES (?, ?, ?, ?, ?);"));
    Q_FOREACH (const Tp::ContactPtr &c, added) {
        if (c->manager()->connection()->protocolName() == QLatin1String("local-xmpp")) {
            continue;
        }

        bindContactToQuery(&insertQuery, c);
        insertQuery.exec();
    }

    m_db.commit();
}

void ContactCache::connectToAccount(const Tp::AccountPtr &account)
{
    connect(account.data(), SIGNAL(removed()), SLOT(onAccountRemoved()));
    connect(account.data(), SIGNAL(connectionChanged(Tp::ConnectionPtr)), SLOT(onAccountConnectionChanged(Tp::ConnectionPtr)));
}

bool ContactCache::accountIsInteresting(const Tp::AccountPtr &account) const
{
    if (account->protocolName() == QLatin1String("local-xmpp")) {// We don't want to cache local-xmpp contacts
        return false;
    }

    /* There may be more filters. */

    return true;
}

void ContactCache::syncContactsOfAccount(const Tp::AccountPtr &account)
{
    m_db.transaction();
    QSqlQuery purgeQuery(m_db);
    purgeQuery.prepare(QLatin1String("DELETE FROM contacts WHERE accountId = ?;"));
    purgeQuery.bindValue(0, account->uniqueIdentifier());
    purgeQuery.exec();

    QSqlQuery insertQuery(m_db);
    insertQuery.prepare(QLatin1String("INSERT INTO contacts (accountId, contactId, alias, avatarFileName, groupsIds) VALUES (?, ?, ?, ?, ?);"));
    Q_FOREACH (const Tp::ContactPtr &c, account->connection()->contactManager()->allKnownContacts()) {
        bindContactToQuery(&insertQuery, c);
        insertQuery.exec();
    }

    m_db.commit();

    connect(account->connection()->contactManager().data(),
            SIGNAL(allKnownContactsChanged(Tp::Contacts,Tp::Contacts,Tp::Channel::GroupMemberChangeDetails)),
            SLOT(onAllKnownContactsChanged(Tp::Contacts,Tp::Contacts)), Qt::UniqueConnection);
}

void ContactCache::checkContactManagerState(const Tp::ContactManagerPtr &contactManager)
{
    if (contactManager->state() == Tp::ContactListStateSuccess) {
        const QString accountPath = TP_QT_ACCOUNT_OBJECT_PATH_BASE + QLatin1Char('/') + contactManager->connection()->property("accountUID").toString();
        Tp::AccountPtr account = KTp::accountManager()->accountForObjectPath(accountPath);
        if (!account.isNull()) {
            syncContactsOfAccount(account);
        } else {
            kWarning() << "Can't access to account by contactManager";
        }
    }
}

int ContactCache::askIdFromGroup(const QString &groupName)
{
    int index = m_groups.indexOf(groupName);
    if (index >= 0) {
        return index;
    }

    QSqlQuery updateGroupsQuery(m_db);

    for (index = 0; index < m_groups.count(); ++index) {
        if (m_groups.at(index).isEmpty()) {
            m_groups[index] = groupName;
            updateGroupsQuery.prepare(QLatin1String("UPDATE groups SET groupName = :newGroupName WHERE groupId = :index;"));
            break;
        }
    }

    if (index >= m_groups.count()) {
        m_groups.append(groupName);
        updateGroupsQuery.prepare(QLatin1String("INSERT INTO groups (groupId, groupName) VALUES (:index, :newGroupName);"));
    }

    updateGroupsQuery.bindValue(QLatin1String(":newGroupName"), groupName);
    updateGroupsQuery.bindValue(QLatin1String(":index"), index);
    updateGroupsQuery.exec();

    return index;
}

void ContactCache::bindContactToQuery(QSqlQuery *query, const Tp::ContactPtr &contact)
{
    const KTp::ContactPtr &ktpContact = KTp::ContactPtr::qObjectCast(contact);
    query->bindValue(0, ktpContact->accountUniqueIdentifier());
    query->bindValue(1, ktpContact->id());
    query->bindValue(2, ktpContact->alias());
    query->bindValue(3, ktpContact->avatarData().fileName);

    QStringList groupsIds;

    Q_FOREACH (const QString &group, ktpContact->groups()) {
        groupsIds.append(QString::number(askIdFromGroup(group)));
    }

    query->bindValue(4, groupsIds.join(QLatin1String(",")));
}
