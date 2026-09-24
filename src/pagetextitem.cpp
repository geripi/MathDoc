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

// pagetextitem.cpp
#include "pagetextitem.h"
#include <QKeyEvent>
#include <QDebug>
#include <QGraphicsScene> // Needed for scene bounding rect
#include <QGraphicsSceneHoverEvent> // For QGraphicsSceneHoverEvent parameter types

// Constructor for PageTextItem.
PageTextItem::PageTextItem(QGraphicsItem *parent)
    : QGraphicsTextItem(parent) {
    // Set initial flags for movability, selection, and geometry change notification
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges | ItemIsFocusable);
    setTextInteractionFlags(Qt::NoTextInteraction); // Initially not editable
    setTabChangesFocus(true); // Pressing Tab moves focus to next item
    setZValue(1); // Ensure text items are above page rectangles

    // Set a default font and size
    QFont font("Arial", 12);
    setFont(font);

    // Enable hover events for custom cursor changes
    setAcceptHoverEvents(true);
    
    setBackGroundColor(Qt::transparent);
}

// Override boundingRect to return a rectangle that includes the border space
// This ensures hover events are received when the mouse is over the border.
QRectF PageTextItem::boundingRect() const {
    QRectF rect = QGraphicsTextItem::boundingRect(); // Get the base text item's bounding rect
    // If the text content is empty or very small, provide a minimum size for the *content*
    if (rect.isEmpty()) {
        rect = QRectF(0, 0, 80, 25); // Minimum content area size
    }
    // Now, expand this content rect by the BORDER_SIZE on all sides
    return rect.adjusted(-BORDER_SIZE, -BORDER_SIZE, BORDER_SIZE, BORDER_SIZE);
}

// Helper function to determine and set the appropriate cursor
void PageTextItem::updateHoverCursor(const QPointF &pos) {
    // The innerRect is now the area that *excludes* the border within the boundingRect()
    // We calculate this based on the item's visual origin and content size.
    // NOTE: This logic assumes boundingRect() *already* includes the border.
    // So, 'innerRect' here represents the 'content area'
    QRectF contentRect = boundingRect().adjusted(BORDER_SIZE, BORDER_SIZE, -BORDER_SIZE, -BORDER_SIZE);; // Get the actual text content bounds
    if (contentRect.isEmpty()) {
        contentRect = QRectF(0, 0, 80, 25); // Use the same minimum content size as in boundingRect()
    }

    // Adjust contentRect relative to the item's coordinate system, where (0,0) is its top-left.
    // The 'pos' from the event is relative to the item's coordinate system.
    // The border region is the area *outside* contentRect but *inside* boundingRect().

    // Check if the mouse is outside the content area but inside the item's total bounds (boundingRect())
    if (boundingRect().contains(pos) && !contentRect.contains(pos)) {
        // Mouse is over the border area: indicate draggable with SizeAllCursor
        setCursor(Qt::OpenHandCursor); // Standard cursor for moving items
    } else {
        // Mouse is over the text content area: indicate editable with Qt::IBeamCursor
        setCursor(Qt::IBeamCursor);
    }
}

// Override hoverEnterEvent to set the cursor when mouse enters the item's area
void PageTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    updateHoverCursor(event->pos());
    QGraphicsTextItem::hoverEnterEvent(event);
}

// Override hoverLeaveEvent to reset the cursor when mouse leaves the item's area
void PageTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    unsetCursor(); // Reset cursor to the default scene cursor
    QGraphicsTextItem::hoverLeaveEvent(event);
}

// Override hoverMoveEvent for continuous cursor updates during pure hover.
// This event is specifically for when the mouse moves over the item *without* any buttons pressed.
void PageTextItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    updateHoverCursor(event->pos());
    //qDebug() << "Hover move position: (" << event->pos().x() << ", " << event->pos().y() << ") - Cursor updated.";
    QGraphicsTextItem::hoverMoveEvent(event); // Pass to base class
}

// Override mouseMoveEvent for continuous cursor updates during hover
void PageTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    QGraphicsTextItem::mouseMoveEvent(event); // Pass event to base class for default handling (e.g., drag if already pressed)
}

// Override mousePressEvent to handle item selection and editing.
void PageTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier)) {
        scene()->clearSelection();
        // Here, innerRect needs to represent the 'content area' that triggers text editing.
        // It should be the *base* QGraphicsTextItem's bounding rect, or the minimum content size if empty.
        //QRectF contentClickRect = QGraphicsTextItem::boundingRect();
        QRectF contentClickRect = boundingRect().adjusted(BORDER_SIZE, BORDER_SIZE, -BORDER_SIZE, -BORDER_SIZE);
        if (contentClickRect.isEmpty()) {
            contentClickRect = QRectF(0, 0, 80, 25); // Use same minimum as in boundingRect() override
        }

        // Check if the mouse click is within the expanded bounding rect (which includes the border)
        // AND if it's *outside* the contentClickRect.
        if (boundingRect().contains(event->pos()) && !contentClickRect.contains(event->pos())) {
            // Case 1: Clicked on the border area (for dragging)
            setTextInteractionFlags(Qt::NoTextInteraction); // Crucial: Disable text editing
            setSelected(true); // Ensure item is selected for dragging
            setFocus();
            qDebug() << "PageTextItem: Clicked border for selection/drag.";
            // Pass to base class. QGraphicsTextItem (via QGraphicsItem) will now see
            // ItemIsMovable and handle the drag initiation because text interaction is off.
            QGraphicsTextItem::mousePressEvent(event);
            // Emit editingFinished if it was previously editing
            if (textInteractionFlags() == Qt::TextEditorInteraction) { emit editingFinished(); }
        } else {
            // Case 2: Clicked inside the text content area (for editing)
            setTextInteractionFlags(Qt::TextEditorInteraction); // Enable text editing
            setFocus(); // Give focus to allow typing
            qDebug() << "PageTextItem: Enabled text editing for item at" << pos();
            // Pass to base class for its text editing interaction.
            QGraphicsTextItem::mousePressEvent(event);
            emit editingStarted();
        }
        emit itemSelected(); qDebug() << "PageTextItem::mousePressEvent: itemSelected() emitted";
        return; // Consume the event
    }
    // For any other mouse buttons (e.g., right-click) or unhandled scenarios,
    // let the base class handle the event.
    QGraphicsTextItem::mousePressEvent(event);
}

// Override focusOutEvent to disable editing when focus is lost.
void PageTextItem::focusOutEvent(QFocusEvent *event) {
    // Clear any text selection when the item loses focus
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.clearSelection();
        setTextCursor(cursor); // Apply the cleared selection
        qDebug() << "PageTextItem: Cleared text selection on focus out.";
    }

    setTextInteractionFlags(Qt::NoTextInteraction); // Disable editing
    QGraphicsTextItem::focusOutEvent(event); // Pass to base class
    qDebug() << "PageTextItem: Disabled text editing for item at" << pos();
    emit editingFinished(); // Ensure this is emitted when focus is lost
}

// Override keyPressEvent to emit itemDataChanged when text content changes
void PageTextItem::keyPressEvent(QKeyEvent *event) {
    QGraphicsTextItem::keyPressEvent(event); // Let the base class handle text editing
    // Emit signal if text changes (e.g., if a character was entered/deleted)
    // This is a simple heuristic; a more robust check might compare old vs new text
    if (event->text().length() > 0 || event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        qDebug() << "PageTextItem: Text content changed. Emitting itemDataChanged.";
        emit itemDataChanged();
    }
}

// Override itemChange to emit itemDataChanged when position changes
QVariant PageTextItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged || change == ItemTransformHasChanged) {
        qDebug() << "PageTextItem: Position/Transform changed. Emitting itemDataChanged.";
        emit itemDataChanged();
    }
    return QGraphicsTextItem::itemChange(change, value);
}

// Implement shape() to provide a larger clickable/draggable area
QPainterPath PageTextItem::shape() const {
    QPainterPath path;
    // Get the current bounding rectangle of the text item.
    // textBoundingRect() is good for text, but boundingRect() might be better if you set a fixed width/height.
    QRectF rect = boundingRect();

    // Expand the rectangle by a few pixels on each side to create the border area.
    qreal borderSize = BORDER_SIZE; // Define the size of the clickable border
    path.addRect(rect.adjusted(-borderSize, -borderSize, borderSize, borderSize));

    return path;
}

void PageTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    //QBrush oldBrush(painter->brush()); //QColor oldColor(oldBrush.color());
    //QBrush brush(m_backGroundColor); painter->setBrush(brush);
    QRectF textRect = boundingRect().adjusted(BORDER_SIZE, BORDER_SIZE, -BORDER_SIZE, -BORDER_SIZE);
    painter->fillRect(textRect,m_backGroundColor);
    //painter->setBrush(oldBrush);
    QGraphicsTextItem::paint(painter, option, widget); // Call base class paint first to draw the text

    if (option->state & QStyle::State_Selected) {
        // Draw a selection rectangle around the item's total boundingRect (which includes the border)
        QPen pen(Qt::blue, 1, Qt::DashLine);
        painter->setPen(pen);
        painter->drawRect(boundingRect());
    }
}

QJsonObject PageTextItem::toJson() const
{
    QJsonObject frameObject;
    frameObject["text"] = toPlainText();
    frameObject["type"] = static_cast<int>(type());
    frameObject["x"] = pos().x(); // Use pos() for scene coordinates
    frameObject["y"] = pos().y(); // Use pos() for scene coordinates
    frameObject["width"] = textWidth(); // Save text width
    
    // Add font information
    QFont font = this->font();
    frameObject["fontFamily"] = font.family();
    frameObject["fontSize"] = font.pointSize();
    frameObject["fontWeight"] = font.weight(); // QFont::Weight is an int alias, safe to save directly
    frameObject["fontItalic"] = font.italic();
    frameObject["fontUnderline"] = font.underline();
    
    // Add text color
    QColor color = this->defaultTextColor();
    frameObject["textColorR"] = color.red();
    frameObject["textColorG"] = color.green();
    frameObject["textColorB"] = color.blue();
    frameObject["textColorA"] = color.alpha();
    
    // Add background color
    QColor bgColor = m_backGroundColor;
    frameObject["bgColorR"] = bgColor.red();
    frameObject["bgColorG"] = bgColor.green();
    frameObject["bgColorB"] = bgColor.blue();
    frameObject["bgColorA"] = bgColor.alpha();
    
    return frameObject;
}

PageTextItem* PageTextItem::fromJson(const QJsonObject& frameObject, QGraphicsItem* parent)
{
    
    // TODO: Change to actually setting the parameters
    PageTextItem* item = new PageTextItem(parent);
    
    
    item->setPlainText(frameObject["text"].toString());
    item->setPos(QPointF(frameObject["x"].toDouble(), frameObject["y"].toDouble()));
    item->setTextWidth(frameObject["width"].toDouble()); // Set text width after loading
    
    // Load font information
    QFont font = item->font();
    if (frameObject.contains("fontFamily")) font.setFamily(frameObject["fontFamily"].toString());
    if (frameObject.contains("fontSize")) font.setPointSize(frameObject["fontSize"].toInt());
    // FIX: Explicitly cast the int to QFont::Weight
    if (frameObject.contains("fontWeight")) font.setWeight(static_cast<QFont::Weight>(frameObject["fontWeight"].toInt()));
    if (frameObject.contains("fontItalic")) font.setItalic(frameObject["fontItalic"].toBool());
    if (frameObject.contains("fontUnderline")) font.setUnderline(frameObject["fontUnderline"].toBool());
    item->setFont(font);
    
    // Load text color
    if (frameObject.contains("textColorR") && frameObject.contains("textColorG") &&
    frameObject.contains("textColorB") && frameObject.contains("textColorA")) {
    QColor color(frameObject["textColorR"].toInt(),
                 frameObject["textColorG"].toInt(),
                 frameObject["textColorB"].toInt(),
                 frameObject["textColorA"].toInt());
    item->setDefaultTextColor(color);
    }
    // Load background color
    if (frameObject.contains("bgColorR") && frameObject.contains("bgColorG") &&
        frameObject.contains("bgColorB") && frameObject.contains("bgColorA")) {
        QColor bgColor(frameObject["bgColorR"].toInt(),
                       frameObject["bgColorG"].toInt(),
                       frameObject["bgColorB"].toInt(),
                       frameObject["bgColorA"].toInt());
        item->setBackGroundColor(bgColor);
    }
    
    return item;
}
