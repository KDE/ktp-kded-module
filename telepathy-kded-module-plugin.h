/*
    Parent class for Telepathy KDED Plugins
    Copyright (C) 2017  James D. Smith <smithjd15@gmail.com>
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

class TelepathyKDEDModulePlugin : public QObject
{
    Q_OBJECT

/* The parent class for the Telepathy KDED plugins. */

public:
    explicit TelepathyKDEDModulePlugin(QObject *parent = 0);
    virtual ~TelepathyKDEDModulePlugin();

    enum State {
        Disabled = false,
        Enabled = true,
        Active
    };
    Q_ENUM(State)

    /**
     * \brief The plugin operating state, i.e. if it is active, enabled, or
     * disabled.
     *
     * \return State enum indicating the plugin's operating state.
     */
    State pluginState() const { return m_pluginState; }

    /**
     * \brief Plugin name. Deriving classes must return a valid plugin name in this method.
     */
    virtual QString pluginName() const = 0;

    /**
     * \brief A plugin presence.
     *
     * \return A KTp::Presence.
     */
    Tp::Presence requestedPresence() const { return m_requestedPresence; }

public Q_SLOTS:
    /**
     * \brief Plugin-specific configuration reload. Deriving classes must have this method reimplemented.
     */
    virtual void reloadConfig() = 0;

Q_SIGNALS:
    void pluginChanged();

protected:
    void setPlugin(State state);
    void setPlugin(const Tp::Presence &presence);

private:
    Tp::Presence m_requestedPresence;
    State m_pluginState;
};

#endif // TELEPATHY_KDED_MODULE_PLUGIN_H
