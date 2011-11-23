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


#include "telepathy-kded-module-plugin.h"

#include <KTelepathy/global-presence.h>

TelepathyKDEDModulePlugin::TelepathyKDEDModulePlugin(KTp::GlobalPresence* globalPresence, QObject* parent)
    : QObject(parent),
      m_enabled(false),
      m_pluginPriority(50)
{
    m_globalPresence = globalPresence;
}

TelepathyKDEDModulePlugin::~TelepathyKDEDModulePlugin()
{
}

void TelepathyKDEDModulePlugin::setEnabled(bool enabled)
{
    m_enabled = enabled;

    if(!enabled) {
        setActive(false);
    }
}

void TelepathyKDEDModulePlugin::setActive(bool active)
{
    m_active = active;
    Q_EMIT activate(active);
}
