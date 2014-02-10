/*
    Manage auto-connecting and restoring of the last presence
    Copyright (C) 2012  Dominik Cermak <d.cermak@arcor.de>

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

#ifndef AUTOCONNECT_H
#define AUTOCONNECT_H

#include <KConfigGroup>
#include <KDebug>

#include <TelepathyQt/Presence>

namespace KTp {
    class Presence;
}

class AutoConnect : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        Disabled,
        Enabled,
        Manual
    };

    AutoConnect(QObject *parent = 0 );
    ~AutoConnect();

    void savePresence(const KTp::Presence &presence);

    /**
     * \brief Returns a string for the given enum value.
     *
     * \param mode The enum value to convert to a string.
     *
     * \return The string for the enum value.
     */
    static inline QString modeToString(const Mode mode)
    {
        switch (mode) {
        case AutoConnect::Disabled:
            return QString::fromLatin1("disabled");
        case AutoConnect::Enabled:
            return QString::fromLatin1("enabled");
        case AutoConnect::Manual:
            return QString::fromLatin1("manual");
        default:
            kWarning() << "Got not recognized mode: '" << mode << "'.";
            kWarning() << "Treat as AutoConnect::Manual (" << AutoConnect::Manual << ").";
            return QString::fromLatin1("manual");
        }
    }

    /**
     * \brief Returns the enum value for the given string.
     *
     * \param mode The string to convert to an enum value.
     *
     * \return The enum value for the string.
     */
    static inline Mode stringToMode(const QString &mode)
    {
        if (mode == modeToString(AutoConnect::Disabled)) {
            return AutoConnect::Disabled;
        } else if (mode == modeToString(AutoConnect::Enabled)) {
            return AutoConnect::Enabled;
        } else if (mode == modeToString(AutoConnect::Manual)) {
            return AutoConnect::Manual;
        } else {
            kWarning() << "Got not recognized string: '" << mode << "'. Treat as 'manual'";
            return AutoConnect::Manual;
        }
    }

private:
    Tp::Presence m_presence;
    KConfigGroup m_kdedConfig;
    KConfigGroup m_presenceConfig;
};

#endif // AUTOCONNECT_H
