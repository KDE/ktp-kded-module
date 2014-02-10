/*
    Class for displaying connection errors
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


#include "error-handler.h"

#include <KTp/core.h>
#include <KTp/error-dictionary.h>

#include <TelepathyQt/Account>
#include <TelepathyQt/AccountManager>
#include <TelepathyQt/Connection>

#include <KNotification>
#include <KAboutData>
#include <KDebug>

#include <Solid/Networking>

#include <QScopedPointer>
#include <QTimer>

/** Stores the last error message for an account
    For every new error if we're online we wait 30 seconds and show 1 notification for all errors. This will be the only error we show for that account until the user reconnects.
*/

class ConnectionError
{
public:
    ConnectionError(Tp::ConnectionStatusReason connectionStatusReason,
                    const QString &connectionError,
                    const Tp::Connection::ErrorDetails &connectionErrorDetails);

    Tp::ConnectionStatusReason connectionStatusReason() const;
    QString connectionError() const;
    Tp::Connection::ErrorDetails connectionErrorDetails() const;

    bool shown() const;
    void setShown(bool);

    QDateTime errorTime() const;

private:
    bool m_shown;
    Tp::ConnectionStatusReason m_connectionStatusReason;
    Tp::Connection::ErrorDetails m_connectionErrorDetails;
    QString m_connectionError;
    QDateTime m_errorTime;
};

ConnectionError::ConnectionError(Tp::ConnectionStatusReason connectionStatusReason, const QString &connectionError, const Tp::Connection::ErrorDetails &connectionErrorDetails):
    m_connectionStatusReason(connectionStatusReason),
    m_connectionErrorDetails(connectionErrorDetails),
    m_connectionError(connectionError)
{
    m_shown = false;
    m_errorTime = QDateTime::currentDateTime();
}

Tp::ConnectionStatusReason ConnectionError::connectionStatusReason() const
{
    return m_connectionStatusReason;
}

QString ConnectionError::connectionError() const
{
    return m_connectionError;
}

Tp::Connection::ErrorDetails ConnectionError::connectionErrorDetails() const
{
    return m_connectionErrorDetails;
}

QDateTime ConnectionError::errorTime() const
{
    return m_errorTime;
}

bool ConnectionError::shown() const
{
    return m_shown;
}

void ConnectionError::setShown(bool)
{
    m_shown = true;
}


ErrorHandler::ErrorHandler(QObject *parent)
    : QObject(parent)
{
    Q_FOREACH(const Tp::AccountPtr &account, KTp::accountManager()->allAccounts()) {
        onNewAccount(account);
    }

    connect(KTp::accountManager().data(), SIGNAL(newAccount(Tp::AccountPtr)),
            this, SLOT(onNewAccount(Tp::AccountPtr)));
}

ErrorHandler::~ErrorHandler()
{

}

void ErrorHandler::showErrorNotification()
{
    //if we're not currently connected to the network, any older errors were probably related to this, ignore them.
    if (Solid::Networking::status() != Solid::Networking::Connected) {
        return;
    }

    QString errorMessage;

    QHash<Tp::AccountPtr, ConnectionError>::iterator i = m_errorMap.begin();
    while (i != m_errorMap.constEnd()) {
        const Tp::AccountPtr account = i.key();
        ConnectionError &error = i.value();

        //try to group as many error messages as we can, but we still want to give accounts a chance to reconnect
        //only want to show messages that are at least 20 seconds old to give the account a chance to connect.
        if (!error.shown() && error.errorTime().secsTo(QDateTime::currentDateTime()) > 20) {
            switch (error.connectionStatusReason()) {
            case Tp::ConnectionStatusReasonNetworkError:
                errorMessage += i18nc("%1 is the account name", "Could not connect %1. There was a network error, check your connection", account->displayName()) + QLatin1String("<br>");
                break;
            default:
                if (error.connectionError() == QLatin1String(TP_QT_ERROR_CANCELLED)) {
                    break;
                }
                if (error.connectionErrorDetails().hasServerMessage()) {
                    errorMessage += i18nc("%1 is the account name, %2 the error message", "There was a problem while trying to connect %1 - %2", account->displayName(), error.connectionErrorDetails().serverMessage()) + QLatin1String("<br>");
                } else {
                    errorMessage += i18nc("%1 is the account name, %2 the error message", "There was a problem while trying to connect %1 - %2", account->displayName(), KTp::ErrorDictionary::displayVerboseErrorMessage(error.connectionError())) + QLatin1String("<br>");
                }
                break;
            }
            error.setShown(true);
        }
        ++i;
    }

    if (!errorMessage.isEmpty()) {
        if (errorMessage.endsWith(QLatin1String("<br>"))) {
            errorMessage.chop(4);
        }

        showMessageToUser(errorMessage, ErrorHandler::SystemMessageError);
    }
}

void ErrorHandler::onConnectionStatusChanged(const Tp::ConnectionStatus status)
{
    Tp::AccountPtr account(qobject_cast< Tp::Account* >(sender()));

    //if we're not connected to the network, errors are pointless
    if (Solid::Networking::status() != Solid::Networking::Connected) {
        return;
    }

    if (status == Tp::ConnectionStatusDisconnected) {
        //if this is the first error for this account, store the details of the error to show
        if (account->connectionStatusReason() == Tp::ConnectionStatusReasonRequested) {
            m_errorMap.remove(account);
        } else if (!m_errorMap.contains(account)) {
            m_errorMap.insert(account, ConnectionError(account->connectionStatusReason(), account->connectionError(), account->connectionErrorDetails()));
            QTimer::singleShot(30 * 1000, this, SLOT(showErrorNotification())); //a timer is kept per account because we want to show 30 seconds after the first still valid error.
        }

    } else  if (status == Tp::ConnectionStatusConnected) {
        //we are now connected, removed pending error messages
        m_errorMap.remove(account);
    }
}

void ErrorHandler::onRequestedPresenceChanged()
{
    Tp::AccountPtr account(qobject_cast< Tp::Account* >(sender()));
    m_errorMap.remove(account);
}

void ErrorHandler::showMessageToUser(const QString &text, const ErrorHandler::SystemMessageType type)
{
    //The pointer is automatically deleted when the event is closed
    KNotification *notification;
    if (type == ErrorHandler::SystemMessageError) {
        notification = new KNotification(QLatin1String("telepathyError"), KNotification::Persistent);
    } else {
        notification = new KNotification(QLatin1String("telepathyInfo"), KNotification::CloseOnTimeout);
    }

    KAboutData aboutData("ktelepathy",0,KLocalizedString(),0);
    notification->setComponentData(KComponentData(aboutData));

    notification->setText(text);
    notification->sendEvent();
}

void ErrorHandler::onNewAccount(const Tp::AccountPtr &account)
{
    connect(account.data(), SIGNAL(connectionStatusChanged(Tp::ConnectionStatus)),
            this, SLOT(onConnectionStatusChanged(Tp::ConnectionStatus)));

    connect(account.data(), SIGNAL(requestedPresenceChanged(Tp::Presence)), SLOT(onRequestedPresenceChanged()));
    connect(account.data(), SIGNAL(removed()), SLOT(onAccountRemoved()));
}

void ErrorHandler::onAccountRemoved()
{
    Tp::AccountPtr account(qobject_cast<Tp::Account*>(sender()));
    Q_ASSERT(account);
    m_errorMap.remove(account);
}
