/* Copyright (C) 2025, 2026 Gerald Pichler (gerald.pichler@chello.at)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//pagegraphicsview.cpp
#include "pagegraphicsview.h"
#include "pagegraphicsscene.h"

#include <QMouseEvent>
#include <QWheelEvent>

PageGraphicsView::PageGraphicsView(QGraphicsScene *scene, Data* d, QWidget *parent)
    : QGraphicsView(scene, parent), m_data(d)
{
    setRenderHint(QPainter::Antialiasing);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(AnchorUnderMouse);
    setBackgroundBrush(QColor(200, 200, 200)); // Light gray background
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setDragMode(QGraphicsView::RubberBandDrag);
}

void PageGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        zoomBy(event->angleDelta().y() > 0 ? 1.1 : 0.9);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void PageGraphicsView::zoomBy(qreal factor)
{
    m_zoomFactor *= factor;
    scale(factor, factor);
}

void PageGraphicsView::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier)) {
        QPointF scenePos = mapToScene(event->pos());
        ensureVisible(QRectF(scenePos, QSizeF(1, 1)), 20, 20);
        if (auto *pgScene = dynamic_cast<PageGraphicsScene *>(scene())) {
            pgScene->setCursorPosition(scenePos);
        }
    }
}
