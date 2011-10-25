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

#include <KNotification>
#include <KAboutData>

#include "common/error-dictionary.h"

ErrorHandler::ErrorHandler(const Tp::AccountManagerPtr& am, QObject* parent)
    : QObject(parent)
{
    m_accountManager = am;

    Q_FOREACH(const Tp::AccountPtr &account, am->allAccounts()) {
        connect(account.data(),
                SIGNAL(connectionStatusChanged(Tp::ConnectionStatus)),
                this, SLOT(handleErrors(Tp::ConnectionStatus)));
    }

    connect(m_accountManager.data(), SIGNAL(newAccount(Tp::AccountPtr)),
            this, SLOT(handleNewAccount(Tp::AccountPtr)));
}

ErrorHandler::~ErrorHandler()
{

}

void ErrorHandler::handleErrors(const Tp::ConnectionStatus status)
{
    Tp::AccountPtr account(qobject_cast< Tp::Account* >(sender()));

    if (status == Tp::ConnectionStatusDisconnected) {
        QString connectionError = account->connectionError();

        Tp::ConnectionStatusReason reason = account->connectionStatusReason();

        switch (reason) {
            case Tp::ConnectionStatusReasonRequested:
                //do nothing
                break;
            case Tp::ConnectionStatusReasonAuthenticationFailed:
                showMessageToUser(i18n("Could not connect %1. Authentication failed (is your password correct?)", account->displayName()), ErrorHandler::SystemMessageError);
                break;
            case Tp::ConnectionStatusReasonNetworkError:
                showMessageToUser(i18n("Could not connect %1. There was a network error, check your connection", account->displayName()), ErrorHandler::SystemMessageError);
                break;
            default:
                showMessageToUser(ErrorDictionary::instance()->displayVerboseErrorMessage(connectionError), ErrorHandler::SystemMessageError);
                break;
        }
    }
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

void ErrorHandler::handleNewAccount(const Tp::AccountPtr& account)
{
    connect(account.data(), SIGNAL(connectionStatusChanged(Tp::ConnectionStatus)),
            this, SLOT(handleErrors(Tp::ConnectionStatus)));
}
