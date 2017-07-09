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

#include "account-status-helper.h"
#include "ktp_kded_debug.h"

#include <KTp/types.h>

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KActivities/kactivities/consumer.h>

#include <QVariant>

#include <TelepathyQt/AccountManager>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/Account>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Types>

AccountStatusHelper::AccountStatusHelper(QObject *parent)
    : QDBusAbstractAdaptor(parent),
    m_enabledAccounts(KTp::accountManager()->enabledAccounts()),
    m_telepathyConfig(KSharedConfig::openConfig(QLatin1String("ktelepathyrc"))),
    m_activities(new KActivities::Consumer())
{
    Tp::registerTypes();
    reloadConfig();

    QDBusConnection::sessionBus().connect(QString(), QLatin1String("/Telepathy"),
                                          QLatin1String("org.kde.Telepathy"),
                                          QLatin1String("settingsChange"), this, SLOT(reloadConfig()));

    connect(m_enabledAccounts.data(), &Tp::AccountSet::accountAdded, [&] (const Tp::AccountPtr &account) {
        Tp::SimplePresence accountPresence = getDiskPresence(account->uniqueIdentifier(), m_activities->currentActivity());

        m_requestedAccountPresences[account->uniqueIdentifier()] = QVariant::fromValue<Tp::SimplePresence>(accountPresence);
        Q_EMIT statusChange(account->uniqueIdentifier());
    });
    connect(m_enabledAccounts.data(), &Tp::AccountSet::accountRemoved, [&] (const Tp::AccountPtr &account) {
        m_requestedAccountPresences.remove(account->uniqueIdentifier());
    });

    auto loadActivity = [=] (const QString &id) {
        m_requestedGlobalPresence = getDiskPresence(QLatin1String("LastPresence"), id);

        for (const Tp::AccountPtr &account : m_enabledAccounts->accounts()) {
            Tp::SimplePresence accountPresence = getDiskPresence(account->uniqueIdentifier(), id);

            m_requestedAccountPresences[account->uniqueIdentifier()] = QVariant::fromValue<Tp::SimplePresence>(accountPresence);
        }
    };

    auto activityServiceStatusChanged = [=] (KActivities::Consumer::ServiceStatus status) {
        if (status == KActivities::Consumer::Running) {
            loadActivity(m_activities->currentActivity());

            if (m_autoConnect) {
                for (const Tp::AccountPtr &account : m_enabledAccounts->accounts()) {
                    Q_EMIT statusChange(account->uniqueIdentifier());
                }
            }
        } else if (status == KActivities::Consumer::NotRunning) {
            qCWarning(KTP_KDED_MODULE) << "activity service not running, user account presences won't load or save";
        }
    };

    connect(m_activities, &KActivities::Consumer::serviceStatusChanged, this, activityServiceStatusChanged);
    connect(m_activities, &KActivities::Consumer::currentActivityChanged, [=] (const QString &id) {
        if (m_activities->serviceStatus() == KActivities::Consumer::Running) {
            if (getDiskPresence(QLatin1String("LastPresence"), id).type == Tp::ConnectionPresenceTypeUnset) {
                setDiskPresence(QLatin1String("LastPresence"), m_requestedGlobalPresence, id);
            }

            loadActivity(id);

            for (const Tp::AccountPtr &account : m_enabledAccounts->accounts()) {
                Q_EMIT statusChange(account->uniqueIdentifier());
            }
        }
    });

    connect(m_activities, &KActivities::Consumer::activityRemoved, [this] (const QString &id) {
        KConfigGroup activityGroup = m_telepathyConfig->group(id);
        activityGroup.deleteGroup();
        activityGroup.sync();
    });

    activityServiceStatusChanged(m_activities->serviceStatus());
}

AccountStatusHelper::~AccountStatusHelper()
{
}

QHash<QString, QVariant> AccountStatusHelper::requestedAccountPresences() const
{
    return m_requestedAccountPresences;
}

Tp::SimplePresence AccountStatusHelper::requestedGlobalPresence() const
{
    return m_requestedGlobalPresence;
}

void AccountStatusHelper::setRequestedGlobalPresence(const Tp::SimplePresence &presence, uint presenceClass)
{
    if (PresenceClass(presenceClass) == PresenceClass::Session) {
        if (presence.type == Tp::ConnectionPresenceTypeUnset) {
            m_requestedGlobalPresence = getDiskPresence(QLatin1String("LastPresence"), m_activities->currentActivity());
        } else if (presence.type == Tp::ConnectionPresenceTypeUnknown) {
            m_requestedGlobalPresence.statusMessage = presence.statusMessage;
        } else {
            m_requestedGlobalPresence = presence;
        }
    } else if (PresenceClass(presenceClass) == PresenceClass::Persistent) {
        m_requestedGlobalPresence = presence;

        if (m_requestedGlobalPresence.type != Tp::ConnectionPresenceTypeOffline) {
            setDiskPresence(QLatin1String("LastPresence"), presence, m_activities->currentActivity());
        }
    }

    qCDebug(KTP_KDED_MODULE) << "new requested global presence"
      << PresenceClass(presenceClass) << presence.status << "with status message"
      << presence.statusMessage;

    Q_EMIT statusChange();
}

void AccountStatusHelper::setRequestedAccountPresence(const QString &accountUID, const Tp::SimplePresence &presence, uint presenceClass)
{
    if (PresenceClass(presenceClass) == PresenceClass::Session) {
        if (presence.type == Tp::ConnectionPresenceTypeUnset) {
            m_requestedAccountPresences[accountUID] = QVariant::fromValue<Tp::SimplePresence>(getDiskPresence(accountUID, m_activities->currentActivity()));
        } else if (presence.type == Tp::ConnectionPresenceTypeUnknown) {
            Tp::SimplePresence statusMessagePresence = qvariant_cast<Tp::SimplePresence>(m_requestedAccountPresences[accountUID]);
            statusMessagePresence.statusMessage = presence.statusMessage;
            m_requestedAccountPresences[accountUID] = QVariant::fromValue<Tp::SimplePresence>(statusMessagePresence);
        } else {
            m_requestedAccountPresences[accountUID] = QVariant::fromValue<Tp::SimplePresence>(presence);
        }
    } else if (PresenceClass(presenceClass) == PresenceClass::Persistent) {
        m_requestedAccountPresences[accountUID] = QVariant::fromValue<Tp::SimplePresence>(presence);
        setDiskPresence(accountUID, presence, m_activities->currentActivity());
    }

    qCDebug(KTP_KDED_MODULE) << "new requested account presence"
      << PresenceClass(presenceClass) << presence.status << "with status message"
      << presence.statusMessage << "for account" << accountUID;

    Q_EMIT statusChange(accountUID);
}

void AccountStatusHelper::reloadConfig()
{
    KConfigGroup kdedConfig = m_telepathyConfig->group("KDED");

    m_autoConnect = kdedConfig.readEntry(QLatin1String("autoConnect"), false);
}

void AccountStatusHelper::setDiskPresence(const QString &presenceGroup, const Tp::SimplePresence &presence, const QString &activity)
{
    KConfigGroup diskPresenceGroup = m_telepathyConfig->group(activity).group(presenceGroup);

    if (m_activities->serviceStatus() != KActivities::Consumer::Running) {
        return;
    }

    if (presence.type != Tp::ConnectionPresenceTypeUnset) {
        diskPresenceGroup.writeEntry(QLatin1String("PresenceType"), (uint)presence.type);
        diskPresenceGroup.writeEntry(QLatin1String("PresenceStatus"), presence.status);
        diskPresenceGroup.writeEntry(QLatin1String("PresenceMessage"), presence.statusMessage);
    } else if (diskPresenceGroup.exists()) {
        diskPresenceGroup.deleteGroup();
    }

    m_telepathyConfig->sync();
}

Tp::SimplePresence AccountStatusHelper::getDiskPresence(const QString &presenceGroup, const QString &activity) const
{
    Tp::SimplePresence diskPresence;
    KConfigGroup diskPresenceGroup = m_telepathyConfig->group(activity).group(presenceGroup);

    diskPresence.type = diskPresenceGroup.readEntry<uint>(QLatin1String("PresenceType"), (uint)Tp::ConnectionPresenceTypeUnset);
    diskPresence.status = diskPresenceGroup.readEntry<QString>(QLatin1String("PresenceStatus"), QLatin1String("unset"));
    diskPresence.statusMessage = diskPresenceGroup.readEntry<QString>(QLatin1String("PresenceMessage"), QString());

    return diskPresence;
}
