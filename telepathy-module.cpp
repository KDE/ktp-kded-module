/*
    KDE integration module for Telepathy
    Copyright (C) 2011  Martin Klapetek <martin.klapetek@gmail.com>
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

#include "telepathy-module.h"

#include "contact-cache.h"
#include "contact-request-handler.h"
#include "contactnotify.h"
#include "error-handler.h"
#include "status-handler.h"

#include <KTp/contact-factory.h>
#include <KTp/core.h>

#include <TelepathyQt/AccountFactory>
#include <TelepathyQt/PendingReady>
#include <TelepathyQt/Debug>

#include <KConfigGroup>
#include <KPluginFactory>

K_PLUGIN_FACTORY(TelepathyModuleFactory, registerPlugin<TelepathyModule>(); )

TelepathyModule::TelepathyModule(QObject *parent, const QList<QVariant> &args)
    : KDEDModule(parent),
    m_statusHandler(new StatusHandler(this)),
    m_contactHandler( 0 ),
    m_contactNotify( 0 ),
    m_errorHandler( 0 )
{
    Q_UNUSED(args)

    Tp::registerTypes();
    Tp::enableDebug(false);
    Tp::enableWarnings(false);

    connect(KTp::accountManager()->becomeReady(), SIGNAL(finished(Tp::PendingOperation*)), SLOT(onAccountManagerReady(Tp::PendingOperation*)));
}

TelepathyModule::~TelepathyModule()
{
}

void TelepathyModule::onAccountManagerReady(Tp::PendingOperation *op)
{
    if (op->isError()) {
        return;
    }

    m_errorHandler = new ErrorHandler(this);
    m_contactHandler = new ContactRequestHandler(this);
    m_contactNotify = new ContactNotify(this);
    new ContactCache(this);
}


#include "telepathy-module.moc"
