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


#ifndef NOWPLAYING_LISTWIDGET_H
#define NOWPLAYING_LISTWIDGET_H

#include <QListWidget>
#include <QDragEnterEvent>

class NowPlayingListWidget : public QListWidget
{
     Q_OBJECT

public:
    NowPlayingListWidget(QWidget *parent = 0);
    virtual ~NowPlayingListWidget();

    void setLocalizedTagNames(QStringList tagNames);
    void setItemsIcons(QStringList itemsIcons);
    void setupItems();

protected:
    void resizeEvent(QResizeEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void mousePressEvent(QMouseEvent *event);
    QStringList mimeTypes() const;

private:
    QStringList m_localizedTagNames;
    QStringList m_itemsIcons;
};

#endif // NOWPLAYING_LISTWIDGET_H
