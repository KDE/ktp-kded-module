/*
    Copyright (C) 2014  David Edmundson <davidedmundson@kde.org>

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

#include "status-handler.h"

#include <QtGui/QApplication>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusReply>

#include <TelepathyQt/Types>

int main(int argc, char *argv[])
{
    //QApplication is needed above QCoreApplication for KIdleTime
    QApplication app(argc, argv);
    Tp::registerTypes();

    QDBusInterface kdedInterface("org.kde.kded5","/kded","org.kde.kded5");
    QDBusReply<QStringList> reply =  kdedInterface.call("loadedModules");

    if (reply.value().contains("kded_ktp_integration_module")) {
        qDebug() << "The KTp KDED module is already running.";
        qDebug() << "To unload it run:";
        qDebug() << "qdbus org.kde.kded5 /kded org.kde.kded5.unloadModule kded_ktp_integration_module";
        app.exit();
    }

    StatusHandler statusHandler(&app);

    return app.exec();
}
