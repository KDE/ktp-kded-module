/*
    Now playing... presence plugin
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


#ifndef TELEPATHY_MPRIS_H
#define TELEPATHY_MPRIS_H

#include <QObject>
#include <TelepathyQt4/Presence>
#include <TelepathyQt4/AccountManager>

class TelepathyMPRIS : public QObject
{
    Q_OBJECT

public:
    TelepathyMPRIS(const Tp::AccountManagerPtr am, QObject *parent = 0);
    virtual ~TelepathyMPRIS();

public Q_SLOTS:
    void onPlayerSignalReceived(const QString &interface, const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
    void onSettingsChanged();

Q_SIGNALS:
    void setPresence(const Tp::Presence &presence);

private:
    Tp::AccountManagerPtr m_accountManager;
    QString m_originalPresence;
};

#endif // TELEPATHY_MPRIS_H
