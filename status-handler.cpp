/*
    Copyright (C) 2017-2018  James D. Smith <smithjd15@gmail.com>
    Copyright (C) 2014  David Edmundson <kde@davidedmundson.co.uk>
    Copyright (C) 2011  Martin Klapetek <martin.klapetek@gmail.com>

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

#include "status-handler.h"

#include "autoaway.h"
#include "screensaveraway.h"

#include "account-status-helper.h"
#include "status-message-parser.h"

#include "ktp_kded_debug.h"

#include <QTimer>
#include <QVariant>
#include <QHash>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountSet>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Constants>
#include <TelepathyQt/Presence>

StatusHandler::StatusHandler(QObject* parent)
    : QObject(parent),
    m_enabledAccounts(KTp::accountManager()->enabledAccounts())
{
    QDBusConnection::sessionBus().registerObject(QLatin1String("/StatusHandler"), this);

    QDBusConnection::sessionBus().connect(QString(), QLatin1String("/Telepathy"), QLatin1String("org.kde.Telepathy"),
                                      QLatin1String("settingsChange"), this, SIGNAL(settingsChanged()));

    m_accountStatusHelper = new AccountStatusHelper(this);

    m_parsers[QLatin1String("GlobalPresence")] = new StatusMessageParser(this);
    connect(m_parsers[QLatin1String("GlobalPresence")], &StatusMessageParser::statusMessageChanged, [=] {
        qCDebug(KTP_KDED_MODULE) << "global presence parser has new status message" << m_parsers[QLatin1String("GlobalPresence")]->statusMessage();
        setPresence();
    });

    m_parsers[QLatin1String("PluginPresence")] = new StatusMessageParser(this);
    connect(m_parsers[QLatin1String("PluginPresence")], &StatusMessageParser::statusMessageChanged, [=] {
        qCDebug(KTP_KDED_MODULE) << "plugin presence parser has new status message" << m_parsers[QLatin1String("PluginPresence")]->statusMessage();
        setPresence();
    });

    auto addParser = [=] (const QString &accountUID) {
        m_parsers[accountUID] = new StatusMessageParser(this);
        connect(m_parsers[accountUID], &StatusMessageParser::statusMessageChanged, m_parsers[accountUID], [=] {
            qCDebug(KTP_KDED_MODULE) << "account" << accountUID << "parser has new status message" << m_parsers[accountUID]->statusMessage();
            setPresence(accountUID);
        });

        qCDebug(KTP_KDED_MODULE) << "new parser:" << accountUID;
    };

    auto watchRequestedPresenceChange = [=] (const Tp::AccountPtr &account) {
        connect(account.data(), &Tp::Account::requestedPresenceChanged, account.data(), [=] (const Tp::Presence &requestedPresence) {
            if (requestedPresence != m_accountActivePresences[account->uniqueIdentifier()]) {
                m_accountStatusHelper->setRequestedAccountPresence(account->uniqueIdentifier(), requestedPresence.barePresence(), AccountStatusHelper::Session);
            }
        });
    };

    for (const Tp::AccountPtr &account : m_enabledAccounts->accounts()) {
        addParser(account->uniqueIdentifier());
        watchRequestedPresenceChange(account);
    }

    m_pluginPresence.setStatus(Tp::ConnectionPresenceTypeUnset, QLatin1String("unset"), QString());

    AutoAway *autoAway = new AutoAway(this);
    ScreenSaverAway *screenSaverAway = new ScreenSaverAway(this);

    //earlier in list = lower priority
    m_queuePlugins << screenSaverAway << autoAway;

    QTimer *activationTimer = new QTimer();
    activationTimer->setSingleShot(true);
    activationTimer->setInterval(500);
    connect(activationTimer, &QTimer::timeout, [=] {
        QList<TelepathyKDEDModulePlugin*> activePlugins;
        for (TelepathyKDEDModulePlugin* plugin : m_queuePlugins) {
            if (plugin->pluginState() != TelepathyKDEDModulePlugin::Active)
                continue;

            if (KTp::Presence::sortPriority(plugin->requestedPresence().type())
              >= KTp::Presence::sortPriority(m_pluginPresence.type())) {
                activePlugins.prepend(plugin);
            } else {
                activePlugins.append(plugin);
            }
        }

        if (activePlugins.isEmpty()) {
            m_pluginPresence.setStatus(Tp::ConnectionPresenceTypeUnset, QLatin1String("unset"), QString());
        } else {
            m_pluginPresence = activePlugins.at(0)->requestedPresence();
        }

        m_parsers[(QLatin1String("PluginPresence"))]->parseStatusMessage(m_pluginPresence.statusMessage());
        qCDebug(KTP_KDED_MODULE) << "plugin queue activation:" << m_pluginPresence.status()
          << m_parsers[(QLatin1String("PluginPresence"))]->statusMessage();
        setPresence();
    });

    for (TelepathyKDEDModulePlugin* plugin : m_queuePlugins) {
        connect(plugin, &TelepathyKDEDModulePlugin::pluginChanged, activationTimer, static_cast<void (QTimer::*)(void)>(&QTimer::start));
        connect(this, &StatusHandler::settingsChanged, plugin, &TelepathyKDEDModulePlugin::reloadConfig);
    }

    connect(m_accountStatusHelper, &AccountStatusHelper::statusChange, [=] (const QString &accountUID) {
        if (accountUID.isEmpty()) {
            m_parsers[QLatin1String("GlobalPresence")]->parseStatusMessage(m_accountStatusHelper->requestedGlobalPresence().statusMessage);
        } else {
            Tp::Presence presence = Tp::Presence(qvariant_cast<Tp::SimplePresence>(m_accountStatusHelper->requestedAccountPresences().value(accountUID)));
            m_parsers[accountUID]->parseStatusMessage(presence.statusMessage());

            // The global presence parser must be set for accounts using the global presence.
            if ((presence.type() == Tp::ConnectionPresenceTypeUnset)
              && (m_parsers[QLatin1String("GlobalPresence")]->statusMessage().isEmpty()
              != m_accountStatusHelper->requestedGlobalPresence().statusMessage.isEmpty())) {
                m_parsers[QLatin1String("GlobalPresence")]->parseStatusMessage(m_accountStatusHelper->requestedGlobalPresence().statusMessage);
            }
        }

        setPresence(accountUID);
    });

    connect(m_enabledAccounts.data(), &Tp::AccountSet::accountAdded, [=] (const Tp::AccountPtr &account) {
        addParser(account->uniqueIdentifier());
        watchRequestedPresenceChange(account);
    });

    connect(m_enabledAccounts.data(), &Tp::AccountSet::accountRemoved, [=] (const Tp::AccountPtr &account) {
        disconnect(account.data(), &Tp::Account::requestedPresenceChanged, account.data(), Q_NULLPTR);
        disconnect(m_parsers[account->uniqueIdentifier()], &StatusMessageParser::statusMessageChanged, m_parsers[account->uniqueIdentifier()], Q_NULLPTR);
        m_parsers.remove(account->uniqueIdentifier());
        m_accountActivePresences.remove(account->uniqueIdentifier());
        parkAccount(account);
    });
}

StatusHandler::~StatusHandler()
{
    QDBusConnection::sessionBus().unregisterObject(QLatin1String("/StatusHandler"), QDBusConnection::UnregisterTree);

    for (const Tp::AccountPtr &account : KTp::accountManager()->onlineAccounts()->accounts()) {
        disconnect(account.data(), &Tp::Account::requestedPresenceChanged, account.data(), Q_NULLPTR);
        parkAccount(account);
    }
}

void StatusHandler::setPresence(const QString &accountUID)
{
    const QString &globalParsedMessage = m_parsers[QLatin1String("GlobalPresence")]->statusMessage();
    const QString &pluginParsedMessage = m_parsers[QLatin1String("PluginPresence")]->statusMessage();
    const QHash<QString, QString> &pluginTokens = m_parsers[QLatin1String("PluginPresence")]->tokens();

    auto accountPresence = [&] (const QString &accountUID) {
        Tp::Presence presence = Tp::Presence(qvariant_cast<Tp::SimplePresence>(m_accountStatusHelper->requestedAccountPresences().value(accountUID)));
        QHash<QString, QString> tokens;

        if ((presence.type() == Tp::ConnectionPresenceTypeUnset)
            || (m_accountStatusHelper->requestedGlobalPresence().type == Tp::Presence::offline().type())) {
            tokens = m_parsers[QLatin1String("GlobalPresence")]->tokens();
            presence = Tp::Presence(m_accountStatusHelper->requestedGlobalPresence());
            presence.setStatusMessage(globalParsedMessage);
        } else {
            tokens = m_parsers[accountUID]->tokens();

            if (tokens.contains((QLatin1String("%um"))) && tokens[(QLatin1String("%um"))] == QLatin1String("g")) {
                presence.setStatusMessage(globalParsedMessage);
            } else {
                presence.setStatusMessage(m_parsers[accountUID]->statusMessage());
            }
        }

        bool sourcePluginQueue = (presence.type() != Tp::ConnectionPresenceTypeHidden)
          && (presence.type() != Tp::ConnectionPresenceTypeOffline)
          && (m_pluginPresence.type() != Tp::ConnectionPresenceTypeUnset);

        if (sourcePluginQueue) {
            if (KTp::Presence::sortPriority(presence.type()) < KTp::Presence::sortPriority(m_pluginPresence.type())) {
                presence = Tp::Presence(m_pluginPresence.type(), m_pluginPresence.status(), QString());
            }

            if (!pluginTokens.contains(QLatin1String("%um")) && !tokens.contains(QLatin1String("%um"))) {
                presence.setStatusMessage(pluginParsedMessage);
            } else if (tokens.contains((QLatin1String("%um"))) && tokens[(QLatin1String("%um"))] == QLatin1String("g")) {
                presence.setStatusMessage(globalParsedMessage);
            }
        }

        return presence;
    };

    for (const Tp::AccountPtr &account : m_enabledAccounts->accounts()) {
        const Tp::Presence presence = accountPresence(account->uniqueIdentifier());

        if (!accountUID.isEmpty() && (accountUID != account->uniqueIdentifier())) {
            continue;
        }

        connect(account->setRequestedPresence(presence), &Tp::PendingOperation::finished, [=] (Tp::PendingOperation *op) {
            if (op->isValid()) {
                qCDebug(KTP_KDED_MODULE) << account->uniqueIdentifier() << "requested presence change to" << presence.status() << "with status message" << presence.statusMessage();
                m_accountActivePresences[account->uniqueIdentifier()] = presence;
            } else if (op->isError()) {
                qCWarning(KTP_KDED_MODULE()) << account->uniqueIdentifier() << "requested presence change error:" << op->errorMessage();
            }
        });
    }
}

void StatusHandler::parkAccount(const Tp::AccountPtr &account)
{
    Tp::SimplePresence accountPresence = qvariant_cast<Tp::SimplePresence>(m_accountStatusHelper->requestedAccountPresences().value(account->uniqueIdentifier()));
    if (accountPresence.type == Tp::ConnectionPresenceTypeUnset) {
        accountPresence = m_accountStatusHelper->requestedGlobalPresence();
    }
    accountPresence.statusMessage = QLatin1String();

    account->setRequestedPresence(Tp::Presence(accountPresence));
}
