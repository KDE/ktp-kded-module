/*
    Copyright (C) 2017  James D. Smith <smithjd15@gmail.com>
    Copyright (C) 2014  David Edmundson <kde@davidedmundson.co.uk>

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


#include <QObject>
#include <QHash>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>

#include <TelepathyQt/Presence>

class AccountStatusHelper;
class TelepathyKDEDModulePlugin;
class StatusMessageParser;

/**
 * This class initiates and responds to presence change events, modifying the
 * tp account presence to match the presence reflected in the plugin queue
 * and / or account status helper. It also processes incoming account requested
 * presence changes that aren't the result of a plugin presence or status message parser.
 */

class StatusHandler : public QObject
{
    Q_OBJECT

public:
    StatusHandler(QObject *parent);
    ~StatusHandler();

Q_SIGNALS:
    void settingsChanged();

private Q_SLOTS:
    void setPresence(const QString &accountUID = QString());

private:
    void parkAccount(const Tp::AccountPtr &account);
    Tp::AccountSetPtr m_enabledAccounts;

    AccountStatusHelper *m_accountStatusHelper;

    QList<TelepathyKDEDModulePlugin*> m_queuePlugins;
    QHash<QString,StatusMessageParser*> m_parsers;

    Tp::Presence m_pluginPresence;
};
