/*
    Status managing and auto connect class
    Copyright (C) 2017  James D. Smith <smithjd15@gmail.com>

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

#ifndef ACCOUNTSTATUSHELPER_H
#define ACCOUNTSTATUSHELPER_H

#include <KConfigGroup>
#include <KSharedConfig>
#include <KActivities/kactivities/consumer.h>

#include <QHash>
#include <QDBusAbstractAdaptor>

#include <TelepathyQt/AccountSet>
#include <TelepathyQt/Types>

/* This class contains incoming global and account presence change related
 * functionality. Its responsibilities include receiving and exporting
 * requested global and account presences, and autoconnect.
 */

class AccountStatusHelper : public QDBusAbstractAdaptor
{
    Q_OBJECT

    Q_CLASSINFO("D-Bus Interface", "org.kde.Telepathy.AccountStatusHelper")
    Q_PROPERTY(QVariantHash requestedAccountPresences READ requestedAccountPresences)
    Q_PROPERTY(Tp::SimplePresence requestedGlobalPresence READ requestedGlobalPresence)

public:
    AccountStatusHelper(QObject *parent = 0);
    ~AccountStatusHelper();

    enum PresenceClass
    {
        Persistent = 0,
        Session = 1
    };
    Q_ENUM(PresenceClass)

    /**
     * \brief A hash of the accounts and account presences.
     *
     * \return A Tp::AccountPtr::uniqueIdentifier() and Tp::SimplePresence QVariantHash.
     */
    QVariantHash requestedAccountPresences() const;

    /**
     * \brief The requested global presence.
     *
     * \return A Tp::SimplePresence.
     */
    Tp::SimplePresence requestedGlobalPresence() const;

public Q_SLOTS:
    /**
     * \brief Set the user requested global presence.
     *
     * \param presence A Tp::SimplePresence. A session presence of type unknown
     * with a status message will attach the status message to the persistent
     * presence. A presence type of unset will clear a presence.
     *
     * \param presenceClass The PresenceClass, e.g. Persistent or Session.
     */
    void setRequestedGlobalPresence(const Tp::SimplePresence &presence, uint presenceClass);

    /**
     * \brief Set the account user requested presence.
     *
     * \param accountUID A Tp::AccountPtr::uniqueIdentifier().
     *
     * \param presence A Tp::SimplePresence. A session presence of type unknown
     * with a status message will attach the status message to the persistent
     * presence. A presence type of unset will clear a presence.
     *
     * \param presenceClass The PresenceClass, e.g. Persistent or Session.
     */
    void setRequestedAccountPresence(const QString &accountUID, const Tp::SimplePresence &presence, uint presenceClass);

    void reloadConfig();

Q_SIGNALS:
    void statusChange(const QString &accountUID = QString());

private:
    void setDiskPresence(const QString &presenceGroup, const Tp::SimplePresence &presence, const QString &activity);
    Tp::SimplePresence getDiskPresence(const QString &presenceGroup, const QString &activity) const;

    Tp::AccountSetPtr m_enabledAccounts;
    KSharedConfigPtr m_telepathyConfig;
    KActivities::Consumer *m_activities;

    QHash<QString, QVariant> m_requestedAccountPresences;
    Tp::SimplePresence m_requestedGlobalPresence;

    bool m_autoConnect;
};

#endif // ACCOUNTSTATUSHELPER_H
