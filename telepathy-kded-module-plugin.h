/*
    Parent class for Telepathy KDED Plugins
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


#ifndef TELEPATHY_KDED_MODULE_PLUGIN_H
#define TELEPATHY_KDED_MODULE_PLUGIN_H

#include <QObject>
#include <TelepathyQt/Presence>

namespace KTp {
class GlobalPresence;
}

class TelepathyKDEDModulePlugin : public QObject
{
    Q_OBJECT

public:
    explicit TelepathyKDEDModulePlugin(KTp::GlobalPresence *globalPresence, QObject *parent = 0);
    virtual ~TelepathyKDEDModulePlugin();

    bool isActive() const { return m_active; }
    bool isEnabled() const { return m_enabled; }
    /// Deriving classes must return a valid plugin name in this method
    virtual QString pluginName() const = 0;

    Tp::Presence requestedPresence() const { return m_requestedPresence; }
    QString requestedStatusMessage() const { return m_requestedStatusMessage; }

public Q_SLOTS:
    /// Deriving classes with configuration must have this method reimplemented
    virtual void reloadConfig() = 0;

Q_SIGNALS:
    void requestPresenceChange(const Tp::Presence &presence);
    void requestStatusMessageChange(const QString statusMessage);
    void activate(bool);

protected:
    void setActive(bool active);
    void setEnabled(bool enabled);
    void setRequestedPresence(const Tp::Presence &presence) { m_requestedPresence = presence; }
    void setRequestedStatusMessage(const QString statusMessage) { m_requestedStatusMessage = statusMessage; }

    KTp::GlobalPresence *m_globalPresence;

private:
    Tp::Presence m_requestedPresence;
    QString m_requestedStatusMessage;
    bool m_enabled;
    bool m_active;
};

#endif // TELEPATHY_KDED_MODULE_PLUGIN_H
