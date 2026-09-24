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

// pagea4item.h
#ifndef PAGEA4ITEM_H
#define PAGEA4ITEM_H

#include <QGraphicsRectItem> // Required for QGraphicsRectItem

/**
 * @brief The PageA4Item class represents a single A4 page within the QGraphicsScene.
    // No additional functionality added, this is just a branch class to distinguish
    // between different types of QGraphicsRectItem (page background, image, ...)
 */
class PageA4Item : public QGraphicsRectItem
{
public:
    /**
     * @brief Constructor for PageA4Item.
     * @param rect The rectangle defining the page's geometry.
     * @param parent The parent QGraphicsItem, if any.
     */
    explicit PageA4Item(QGraphicsItem *parent = nullptr)
        :QGraphicsRectItem(parent) { setPen(Qt::NoPen); }
    explicit PageA4Item(const QRectF &rect, QGraphicsItem *parent = nullptr)
        : QGraphicsRectItem(rect, parent) { setPen(Qt::NoPen); }
    explicit PageA4Item(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent = nullptr)
        :QGraphicsRectItem(x, y, width, height, parent) { setPen(Qt::NoPen); }
        
    enum { Type = UserType + 1 }; // unique per subclass
    int type() const override { return Type; }
};

#endif // PAGEA4ITEM_H

