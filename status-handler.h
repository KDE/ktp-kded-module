/*
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

#include <KTp/types.h>

class TelepathyKDEDModulePlugin;
class AutoConnect;

namespace KTp {
    class GlobalPresence;
}

/**
 * This class keeps track of all status modifying functions, such as now playing and autoaway and
 * ensures they do not clash.
 * It consists of a queue of plugins. If any of these plugins are active the presence is set to this
 * this presence.
 *
 * Otherwise we fall back to the last user set presence.
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
    void onAccountManagerReady(Tp::PendingOperation *op);
    void onRequestedPresenceChanged(const KTp::Presence &presence);
    void onPluginActivated(bool);

private:
    private:
    /** Returns the presence we think we should be in. Either from the highest priority plugin, or if none are active, the last user set.*/
    KTp::Presence currentPluginPresence() const;
    QString currentPluginStatusMessage();
    KTp::Presence presenceThrottle();
    const QString statusMessageStack();

    bool activePlugin(); //FIXME should be isActivePlugin
    bool activeStatusMessagePlugin(); //FIXME should be isActiveStatusMessagePlugin
    void setPresence(const KTp::Presence &presence);

    AutoConnect             *m_autoConnect;

    QList<TelepathyKDEDModulePlugin*> m_pluginStack;
    QList<TelepathyKDEDModulePlugin*> m_statusMessagePluginStack;
    KTp::Presence m_lastUserPresence;
    KTp::GlobalPresence *m_globalPresence;
};
