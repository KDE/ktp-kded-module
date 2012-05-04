/*
    Copyright (C) 2012 Othmane Moustaouda <othmane.moustaouda@gmail.com>

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


#ifndef NOWPLAYING_LINEEDIT_H
#define NOWPLAYING_LINEEDIT_H

#include <QLineEdit>

class NowPlayingLineEdit : public QLineEdit    //krazy:exclude=qclasses
{
     Q_OBJECT

public:
    NowPlayingLineEdit(QWidget *parent = 0);
    virtual ~NowPlayingLineEdit();

    void setLocalizedTagNames(QStringList tagNames);

protected:
    void dropEvent(QDropEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    QStringList m_localizedTagNames;
};

#endif // NOWPLAYING_LINEEDIT_H
