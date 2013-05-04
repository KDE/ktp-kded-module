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

#include "nowplaying-listwidget.h"

#include <KIcon>
#include <KIconLoader>
#include <QScrollBar>

NowPlayingListWidget::NowPlayingListWidget(QWidget *parent)
: QListWidget(parent)
{
    setFlow(QListWidget::LeftToRight);
    setDragEnabled(true);

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollMode(ScrollPerPixel);
}

NowPlayingListWidget::~NowPlayingListWidget()
{

}

void NowPlayingListWidget::setLocalizedTagNames(QStringList tagNames)
{
    m_localizedTagNames = tagNames;
}

void NowPlayingListWidget::setItemsIcons(QStringList itemsIcons)
{
    m_itemsIcons = itemsIcons;
}

void NowPlayingListWidget::setupItems()
{
    QString tagName;

    //items adding order is based on that in config/telepathy-kded-config.cpp
    for (int i = 0; i < m_localizedTagNames.size(); i++) {
        tagName = m_localizedTagNames.at(i);
        tagName = tagName.right(tagName.size() - 1); //cut the '%' character
        tagName = tagName.left(1).toUpper() + tagName.mid(1); //capitalize tag name

        QListWidgetItem *newItem = new QListWidgetItem(KIcon(m_itemsIcons.at(i)), tagName);
        addItem(newItem);
    }
}

void NowPlayingListWidget::resizeEvent(QResizeEvent* event)
{
    QListWidget::resizeEvent(event);

    int height = sizeHintForRow(0) + 2 * ( frameWidth() + 3 ); // add 2*3 for top/bottom padding
    if (horizontalScrollBar() && horizontalScrollBar()->isVisible()) {
        height += horizontalScrollBar()->size().height();
    }

    setMaximumHeight(height);
}

void NowPlayingListWidget::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept();
}

void NowPlayingListWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void NowPlayingListWidget::mousePressEvent(QMouseEvent *event)
{
    //after checking if we are in presence of a drag operation, we can then encapsulate
    //the data to be sent and let start the drag operation
    if (event->button() == Qt::LeftButton && itemAt(event->pos())) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData();

        mimeData->setText(itemAt(event->pos())->text()); //who receives expects plain text data
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

QStringList NowPlayingListWidget::mimeTypes() const
{
    QStringList types;
    types << QLatin1String("text/plain");
    return types;
}
