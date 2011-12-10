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


#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <QObject>
#include <TelepathyQt/AccountManager>

class ErrorHandler : public QObject
{
    Q_OBJECT
public:
    ErrorHandler(const Tp::AccountManagerPtr& am, QObject *parent = 0);
    virtual ~ErrorHandler();

    enum SystemMessageType {
        /*
         * this will show a system message to the user
         * but it will fade after short timout,
         * thus it should be used for non-important messages
         * like "Connecting..." etc.
         */
        SystemMessageInfo,

        /*
         * message with this class will stay visible until user
         * closes it and will have light-red background
         */
        SystemMessageError
    };

private Q_SLOTS:
    void handleErrors(const Tp::ConnectionStatus status);
    void showMessageToUser(const QString &text, const ErrorHandler::SystemMessageType type);
    void handleNewAccount(const Tp::AccountPtr &account);

private:
    Tp::AccountManagerPtr m_accountManager;
};

#endif // ERROR_HANDLER_H
