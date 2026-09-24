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

// pagetextitem.h
#ifndef PAGETEXTITEM_H
#define PAGETEXTITEM_H

#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent> // Required for mousePressEvent override
#include <QTextDocument> // For QTextDocument properties
#include <QKeyEvent>
#include <QStyleOptionGraphicsItem> // Include QStyleOptionGraphicsItem here
#include <QPainter> // For QPainter in paint method
#include <QColor>
#include <QGraphicsSceneHoverEvent> // Include for hover events
#include <QTextCursor> // Required for QTextCursor
#include <QDebug> // For debugging
#include <QJsonObject>
#include <QJsonArray>

// PageTextItem is a custom QGraphicsTextItem that represents a text frame
// within the MathDoc document. It supports basic text editing and selection.
class PageTextItem : public QGraphicsTextItem {
    Q_OBJECT // Required for signals/slots

public:
    enum { Type = UserType + 2 }; // unique per subclass
    int type() const override { return Type; }
    
    explicit PageTextItem(QGraphicsItem *parent = nullptr);
    QColor backGroundColor() { return m_backGroundColor; }
    void setBackGroundColor(QColor c) { m_backGroundColor = c; }
    // Initialize the static const member to ensure a consistent border size
    static constexpr qreal BORDER_SIZE = 7.0; // User-requested border width
    
    QJsonObject toJson() const;
    static PageTextItem* fromJson(const QJsonObject& frameObject, QGraphicsItem* parent = nullptr);
    
signals:
    // Signal emitted when the text content or geometry of this item changes.
    void itemDataChanged();
    // Signals to inform about editing state changes
    void editingStarted(); // Emitted when text editing is enabled
    void editingFinished(); // Emitted when text editing is disabled
    void itemSelected(); // Emitted when this is selected (either click on border or click inside)
    
protected:
    // Override boundingRect to ensure a minimum size for the item
    QRectF boundingRect() const override;
    // Override mousePressEvent to handle item selection and editing.
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    // Overrides for cursor changes on hover
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override; // Also override for continuous cursor updates
    // Override focusOutEvent to disable editing when focus is lost.
    void focusOutEvent(QFocusEvent *event) override;
    // Override keyPressEvent to emit itemDataChanged when text content changes
    void keyPressEvent(QKeyEvent *event) override;
    // Override itemChange to emit itemDataChanged when position changes
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    // Override shape() to include a border for interaction
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    
private:
    // Store the last mouse press position for drag operations.
    QPointF m_lastMousePos; // To be used in mousePressEvent and mouseMoveEvent if you implement custom dragging.
    // Helper to determine and set the appropriate cursor based on mouse position
    void updateHoverCursor(const QPointF &pos);
    QColor m_backGroundColor;

};

#endif // PAGETEXTITEM_H
