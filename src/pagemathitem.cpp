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

// pagemathitem.cpp

#include "mainwindow.h"
#include "pagemathitem.h"
#include "matheditfactory.h"
//#include "mathleafedit.h"
#include "mathedit.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QDebug>
//#include <QGraphicsObject>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QTimer>
#include <QJsonArray>
#include <QJsonValue>
//#include <QFocusEvent>

PageMathItem::PageMathItem(Data *d, QGraphicsItem* parent)
: QGraphicsObject(parent), m_data(d) {
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setAcceptHoverEvents(true); // Enable hover events for custom cursor changes
    
    //setMathLeaf(new MathLeafEdit(m_data, this, this)); // Create the root MathLeafEdit in the constructor
    setMathLeaf(new MathEdit(m_data, this, this)); // Create the root MathEdit in the constructor
    // Set focus on the newly created MathLeafEdit
    if (m_mathLeaf) {
        m_mathLeaf->initialize();
        setSelected(true);
    }
    
    connect(&m_focusCheckTimer, &QTimer::timeout, this, &PageMathItem::checkFocusAndEditingState);
    m_focusCheckTimer.setSingleShot(true); // Ensure it only fires once per start
    QFont font(m_fontName, m_mathFontSize);
    setFont(font);
}

QRectF PageMathItem::boundingRect() const
{
    if (!m_mathLeaf) {
        return QRectF(0, 0, 80, 25);
    }
    
    QRectF bounding = m_mathLeaf->boundingRect();
    bounding.adjust(-BORDER_SIZE, -BORDER_SIZE, BORDER_SIZE, BORDER_SIZE);
    return bounding;
}

void PageMathItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    
    if (isSelected()) {
        painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    } else {
        painter->setPen(Qt::NoPen); 
        //painter->setBrush(QColor(240, 240, 240));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
    }
    
    bool showMathWarn = true; // check if we should show the warning messages
    if (MainWindow *mw = MainWindow::instance()) {
        showMathWarn = mw->showMathWarnings();
    }
    if (MainWindow *mw = MainWindow::instance()) { // check if we should show the structure of the formulas
        m_showStructure = mw->showMathStructure();
    }
    
    if(m_ErrorMessageList.size()>0) {
        painter->setPen(QPen(Qt::red, 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
        paintMesssageList(m_ErrorMessageList, painter);
    } else if(m_WarningMessageList.size()>0 &&
              m_ErrorMessageList.size()==0 &&
              showMathWarn) {
        painter->setPen(QPen(Qt::yellow, 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(boundingRect());
        paintMesssageList(m_WarningMessageList, painter);
    }
    
    Q_UNUSED(option);
    Q_UNUSED(widget);
}
void PageMathItem::paintMesssageList(const QStringList &list, QPainter* painter) {
    qreal lineX = boundingRect().x();
    qreal lineY = boundingRect().y() + boundingRect().height();
    painter->setFont(m_font);
    QFontMetricsF fm(m_font);
    painter->setPen(QPen(Qt::blue));
    for (QString msg: list) {
        painter->drawText(QRectF(lineX, lineY, fm.horizontalAdvance(msg),fm.height()),
                          Qt::AlignLeft | Qt::AlignTop, msg);
        lineY += fm.height();
    }
}

void PageMathItem::addItem(QGraphicsItem* item) {
    item->setParentItem(this);
    
    //if (auto mathLeaf = qgraphicsitem_cast<MathLeafEdit*>(item))
    if (auto mathLeaf = qgraphicsitem_cast<MathEdit*>(item))
    {
        //connect(mathLeaf, &MathLeafEdit::itemSizeChanged, this, &PageMathItem::updateLayout);
        connect(mathLeaf, &MathEdit::itemSizeChanged, this, &PageMathItem::updateLayout);
    }
    
    updateLayout();
}

//void PageMathItem::setMathLeaf(MathLeafEdit* leaf)
void PageMathItem::setMathLeaf(MathEdit* leaf)
{
    if (m_mathLeaf) {
        m_mathLeaf->deleteLater();
    }
    
    m_mathLeaf = leaf;
    m_mathLeaf->setParentItem(this);
    //m_mathLeaf->setPos(m_padding/2.0, m_padding/2.0);
    //m_mathLeaf->setPos(-m_padding, -m_padding);
    
    //connect(m_mathLeaf, &MathLeafEdit::itemSizeChanged, this, &PageMathItem::updateLayout);
    //connect(m_mathLeaf, &MathLeafEdit::gainedFocus, this, &PageMathItem::onChildFocusIn);
    //connect(m_mathLeaf, &MathLeafEdit::lostFocus, this, &PageMathItem::onChildFocusOut);
    connect(m_mathLeaf, &MathEdit::itemSizeChanged, this, &PageMathItem::updateLayout);
    connect(m_mathLeaf, &MathEdit::gainedFocus, this, &PageMathItem::onChildFocusIn);
    connect(m_mathLeaf, &MathEdit::lostFocus, this, &PageMathItem::onChildFocusOut);
    
    updateLayout();
}

QJsonObject PageMathItem::toJson() const
{
    QJsonObject itemObject;
    itemObject["type"] = static_cast<int>(type());
    itemObject["isInDiagramTitle"] = m_isInDiagramTitle;
    itemObject["x"] = pos().x();
    itemObject["y"] = pos().y();
    //itemObject["MathLeafEdit"] = m_mathLeaf->toJson();
    itemObject["MathEdit"] = m_mathLeaf->toJson();
/*    QJsonArray contentArray;
    m_mathLeaf->toJson(contentArray);
    itemObject["content"] = contentArray;
*/    
    return itemObject;
}

PageMathItem* PageMathItem::fromJson(const QJsonObject& json, Data* d, QGraphicsItem* parent) {
    PageMathItem* item = new PageMathItem(d, parent);
    item->setPos(json["x"].toDouble(), json["y"].toDouble());
    //item->m_mathLeaf->fromJson(json["MathLeafEdit"].toObject());
    
    QJsonObject mathEditObject = json["MathEdit"].toObject();
    
    // Create the complete MathEdit tree using the factory.
    MathEdit* mathEdit = MathEditFactory::fromJsonFact(mathEditObject, d, item, item);
    
    if (mathEdit)
    {
        // The PageMathItem constructor may have created a
        // default MathEdit. Replace it with the loaded tree.
        if (item->m_mathLeaf) {
            delete item->m_mathLeaf;
        }
        
        item->m_mathLeaf = mathEdit;
    }
    
    return item;
}

// Override mousePressEvent to handle item selection and editing.
void PageMathItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    // Only handle left mouse button clicks
    if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier)) {
        // Calculate the border area
        QRectF borderRect = boundingRect();
        QRectF contentRect = boundingRect().adjusted(BORDER_SIZE, BORDER_SIZE, -BORDER_SIZE, -BORDER_SIZE);
        
        // If the click is on the border, but not the content
        if (borderRect.contains(event->pos()) && !contentRect.contains(event->pos())) {
            setSelected(true);
            m_dragStartPos = event->pos();
            emit itemSelected(); // triggers a MainWindow Redraw event
        } else {
            event->ignore(); // If not on the border, let the event propagate to children
        }
    }
    // let the base class handle the event which allows it to propagate to children.
    QGraphicsObject::mousePressEvent(event);
}
void PageMathItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
// Only move the item if a drag has been initiated on the border
    if (!m_dragStartPos.isNull() && (event->buttons() & Qt::LeftButton)) {
        QPointF delta = event->pos() - m_dragStartPos;
        setPos(pos() + delta);
    }
    QGraphicsObject::mouseMoveEvent(event);
}

// Helper function to determine and set the appropriate cursor
void PageMathItem::updateHoverCursor(const QPointF &pos) {
    QRectF contentRect = boundingRect().adjusted(BORDER_SIZE, BORDER_SIZE, -BORDER_SIZE, -BORDER_SIZE);; // Get the actual text content bounds
    /*if (contentRect.isEmpty()) {
        contentRect = QRectF(0, 0, 80, 25); // Use the same minimum content size as in boundingRect()
    }*/
    // Check if the mouse is outside the content area but inside the item's total bounds (boundingRect())
    if (boundingRect().contains(pos) && !contentRect.contains(pos)) { // Mouse is over the border area: indicate draggable with SizeAllCursor
        setCursor(Qt::OpenHandCursor); // Standard cursor for moving items
    } else { // Mouse is over the text content area: indicate editable with Qt::IBeamCursor
        setCursor(Qt::IBeamCursor);
    }
}

// Override hoverEnterEvent to set the cursor when mouse enters the item's area
void PageMathItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    updateHoverCursor(event->pos());
    QGraphicsObject::hoverEnterEvent(event);
}

// Override hoverLeaveEvent to reset the cursor when mouse leaves the item's area
void PageMathItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    unsetCursor(); // Reset cursor to the default scene cursor
    QGraphicsObject::hoverLeaveEvent(event);
}

// Override hoverMoveEvent for continuous cursor updates during pure hover.
// This event is specifically for when the mouse moves over the item *without* any buttons pressed.
void PageMathItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    updateHoverCursor(event->pos());
    //qDebug() << "Hover move position: (" << event->pos().x() << ", " << event->pos().y() << ") - Cursor updated.";
    QGraphicsObject::hoverMoveEvent(event); // Pass to base class
}

// Override itemChange to emit itemDataChanged when position changes
QVariant PageMathItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged || change == ItemTransformHasChanged) {
        qDebug() << "PageMathItem: Position/Transform changed. Emitting itemDataChanged.";
        emit itemDataChanged();
    }
    return QGraphicsItem::itemChange(change, value);
}

void PageMathItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    qDebug() << "PageMathItem: mouseReleaseEvent";
checkSanity("PageMathItem::mouseReleaseEvent, before 'emit computeWholeDocument();'");
    emit computeWholeDocument();
    QGraphicsObject::mouseReleaseEvent(event);
}

void PageMathItem::updateLayout() {
    if (m_mathLeaf) {
        m_mathLeaf->setPos(0, 0);
    }
    m_mathLeaf->updateBoundingRectForWholeTree();
    prepareGeometryChange();
    update();
}

void PageMathItem::onChildFocusIn() {
    if (!m_isEditing) {
        m_isEditing = true;
        setSelected(false);
        emit editingStateChanged(true);
    }
}

void PageMathItem::onChildFocusOut() {
    // Start a short timer to wait until all focus events have been processed.
    // After this timer expired checkFocusAndEditingState() is executed.
    //m_mathLeaf->cleanContent();
    m_focusCheckTimer.start(50); // 50ms delay should be sufficient
}
void PageMathItem::checkFocusAndEditingState() {
    // This slot is called after the delay. Now, the scene's focus should be stable.
    if (m_mathLeaf) {
        if (!m_mathLeaf->hasDescendantFocus()) {
            // No MathLeafEdit within this PageMathItem's hierarchy has focus.
            m_isEditing = false;
            emit editingStateChanged(false);
            
            // Only remove the item if it's empty AND no descendant has focus
            if (m_mathLeaf->isEmpty()) {
                if (scene()) {
                    scene()->removeItem(this);
                    deleteLater(); // Schedule for deletion
                }
            } else {
                m_data->setValue(m_mathLeaf->getContent(), m_mathLeaf->getValue());
                emit computeWholeDocument();
            }
        }
    }
}

void PageMathItem::compute() {
    if (m_isComputeBlocked) return;
    if (m_isComputing) return;
        
    m_isComputing = true;
        
    m_ErrorMessageList.clear();
    m_WarningMessageList.clear();
    rootMathLeaf()->compute();
    
    m_isComputing = false;
}

void PageMathItem::addErrorMessage(const QString &m) {
    m_ErrorMessageList.append(m);
}
void PageMathItem::addWarningMessage(const QString &m) {
    m_WarningMessageList.append(m);
}

void PageMathItem::checkSanity(QString msg) {
    qDebug() << "PageMathItem::checkSanity msg = " << msg
             << "  ;  m_mathLeaf =" << m_mathLeaf
             << "  ;  m_data =" << m_data;
}

void PageMathItem::printStructure() {
    qDebug() << "Base:";
    m_mathLeaf->printStructure();
}
