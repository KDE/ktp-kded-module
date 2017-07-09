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


#include "telepathy-kded-module-plugin.h"

#include "ktp_kded_debug.h"

TelepathyKDEDModulePlugin::TelepathyKDEDModulePlugin(QObject *parent)
    : QObject(parent),
    m_pluginState(Disabled)
{
    m_requestedPresence.setStatus(Tp::ConnectionPresenceTypeUnset, QLatin1String("unset"), QString());
}

TelepathyKDEDModulePlugin::~TelepathyKDEDModulePlugin()
{
}

void TelepathyKDEDModulePlugin::setPlugin(State state)
{
    m_pluginState = state;

    qCDebug(KTP_KDED_MODULE) << pluginName() << "state change:" << m_pluginState;

    Q_EMIT pluginChanged();
}

void TelepathyKDEDModulePlugin::setPlugin(const Tp::Presence &presence)
{
    m_requestedPresence = presence;
    m_pluginState = Active;

    qCDebug(KTP_KDED_MODULE) << pluginName() << "presence change request:" << m_requestedPresence.status() << m_requestedPresence.statusMessage();

    Q_EMIT pluginChanged();
}
