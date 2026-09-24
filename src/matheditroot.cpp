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

#include "matheditroot.h"
#include "matheditcreatenew.h"
#include "helpers.h"
#include "matheditfactory.h"
#include "pagemathitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QRandomGenerator>

MathEditRoot::MathEditRoot(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEdit(d, parentPageMathItem, parent) {
    m_argument = new MathEdit(d, parentPageMathItem, this);
    m_index = new MathEdit(QString("2"),d, parentPageMathItem, this);
    //m_base = MathEditCreateNew::newMathEdit("10", MathEdit::MEConstant, d, parentPageMathItem, this);
    m_index->setMathFontSize(qRound(static_cast<qreal>(getMathFontSize())*0.6));
    
    initializeParenthesis();
}
MathEditRoot::MathEditRoot(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEditRoot(d, parentPageMathItem, parent) {
    m_content.append(text);
    setCursorTo(0);
}

void MathEditRoot::setFocus(Qt::FocusReason focusReason) {
    Q_UNUSED(focusReason);
    
    MathEdit *cashedChild = getCashedChild();
    if (cashedChild == m_argument && cashedChild->getRightArrowPressed()) { // leave this towards the right
        setCursorToEnd();
        rightArrow();
    } else if (cashedChild == m_argument && cashedChild->getLeftArrowPressed()) { 
        m_index->setFocus();
        m_index->setCursorToEnd();
    } else if (cashedChild == m_index && cashedChild->getRightArrowPressed()) {
        m_argument->setFocus();
        m_argument->setCursorToBegin();
    } else if (cashedChild == m_index && cashedChild->getLeftArrowPressed()) { // leave this towards the left
        setCursorToBegin();
        leftArrow();
    } else if (!cashedChild) {
        MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
        if (pa->getRightArrowPressed()) {
            m_index->setFocus();
            m_index->setCursorToBegin();
        } else if (pa->getLeftArrowPressed()) {
            m_argument->setFocus();
            m_argument->setCursorToEnd();
        } else {
            m_index->setFocus();
            m_index->setCursorToEnd();
        }
    } else {
        m_argument->setFocus();
        m_argument->setCursorToEnd();
    }
    setCashedChild(nullptr);
}

void MathEditRoot::setFocusToChild1(Qt::FocusReason focusReason) {
    m_argument->setFocus(focusReason);
}
void MathEditRoot::setFocusToChild2(Qt::FocusReason focusReason) {
    m_index->setFocus(focusReason);
}

void MathEditRoot::init() {
    updateBoundingRect();
}

void MathEditRoot::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    painter->setRenderHint(QPainter::Antialiasing, false);
    QFontMetricsF fm(getFont()), subFm(getSubFont());
    
    if (hasFocus()) {
        //int r = QRandomGenerator::global()->bounded(256);
        painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
        painter->setBrush(QColor(255, 255, 127)); // Qt::lightyellow does not exist
        //painter->setBrush(QColor(r, 255, 127, 127)); // Qt::lightyellow does not exist
        painter->drawRect(getBoundingRectangle());
    } else {
        if(m_argument->getContent().isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 223, 223, 127));
            painter->drawRect(getBoundingRectangle());
        } else {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(getBoundingRectangle());
        }
    }
    
    QString normStr = m_content.first(m_content.length()-1);
    QColor textColor = Qt::darkBlue;
    painter->setPen(QPen(textColor, 1, Qt::SolidLine));
    //qreal textVPos = baseline() - fm.height()*0.5 + fm.ascent();
    //painter->drawText(0.0, textVPos, normStr);
    
    QRectF indexBBox = m_index->boundingRect(), argBBox = m_argument->boundingRect();
    
    painter->drawLine(QPointF(                                0, 0.6*getPaddingV()),
                      QPointF(indexBBox.width()-2*getPaddingH(), 0.6*getPaddingV()));
    
    painter->drawLine(QPointF(indexBBox.width()-2*getPaddingH(), 0.6*getPaddingV()),
                      QPointF(                indexBBox.width(), boundingRect().bottom()));
    
    painter->drawLine(QPointF(                indexBBox.width(), boundingRect().bottom()),
                      QPointF(indexBBox.width()+2*getPaddingH(), boundingRect().top()));
    
    painter->drawLine(QPointF(indexBBox.width()+2*getPaddingH(),   boundingRect().top()),
                      QPointF(indexBBox.width()+4*getPaddingH()+argBBox.width(), boundingRect().top()));
    
    cursorPosUpdate();
    if (isCursorVisible()) { // && i == m_cursorPos) {
        painter->setPen(QPen(Qt::black, 1));
        painter->drawLine(QPointF(getCursorX()+1, getCursorY()), QPointF(getCursorX()+1, getCursorH()));
        painter->setPen(QPen());
    }
}

void MathEditRoot::updateBoundingRect() {
    QFontMetricsF fm(getFont());
    QFontMetricsF indexFm(m_index->getFont());
    
    
    QString normStr = "";
    if (m_content.length() > 1) normStr = m_content.first(m_content.length()-1);
    
    QRectF bRectArg = m_argument->getBoundingRectangle();
    QRectF bRectIndex = m_index->getBoundingRectangle();
    
    qreal argY = 0.0;
    
    qreal argLength = bRectArg.width() + 2.0*getPaddingH();
    qreal argX = m_index->getBoundingRectangle().width()+2*getPaddingH();
    
    m_argument->setPos(argX, argY);
    
    qreal indexY = 0.0*fm.height() - m_index->getBoundingRectangle().bottom();
    
    qreal indexX = 0;
    
    m_index->setPos(indexX, indexY);
    
    qreal top = qMin(bRectArg.top(), bRectIndex.top() + indexY)-0.6*getPaddingV();
    qreal bottom = qMax(bRectArg.bottom(), bRectIndex.bottom() + indexY);
    
    QRectF rect(0.0, top, argX + argLength + 2.0*getPaddingH(), bottom - top);
    
    prepareGeometryChange();
    setBoundingRectangle(rect);
    
    notifyParentSizeChange();
    qreal argH = m_argument->boundingRect().height();
    qreal argT = m_argument->boundingRect().top();
    updateParenthesisPath(QPointF(0, argT), argH, true);
    updateParenthesisPath(QPointF(0, argT), argH, false);
}

void MathEditRoot::compute(const MathVariable& boolMask) {
    m_argument->compute(boolMask);
    m_index->compute(boolMask);
    setValue(root(m_argument->getValue(), m_index->getValue(), boolMask));
}

void MathEditRoot::keyPressEvent(QKeyEvent* event)
{
    //printContent();
    const bool altPressed = event->modifiers() & Qt::AltModifier;
    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
    Q_UNUSED(ctrlPressed);
    
    QString charToInsert = event->text();
    int eventKey = event->key();
    
    if (functionCharacters.contains(charToInsert)) {
        QString testStr = m_content;
        testStr.remove("(");
        testStr.insert(m_cursorPos, charToInsert);
        if (isPartOfFuncStr(testStr)) {
            m_content.insert(m_cursorPos, charToInsert);
            m_cursorPos++;
            emit getParentPageMathItem()->itemDataChanged();
        }
    }
    
    if (eventKey == Qt::Key_Backspace) {
        backspace();
        emit getParentPageMathItem()->itemDataChanged();
    } else if (eventKey == Qt::Key_Delete) {
        if (shiftPressed) {
            if (shiftDel()) {
                event->accept();
                emit getParentPageMathItem()->itemDataChanged();
                return;
            }
        } else {
            del();
        }
        emit getParentPageMathItem()->itemDataChanged();
    } else if (eventKey == Qt::Key_Left) {
        if (shiftPressed) shiftLeftArrow();
        else leftArrow();
    } else if (eventKey == Qt::Key_Right) {
        if (shiftPressed) shiftRightArrow();
        else rightArrow();
    } else if (eventKey == Qt::Key_Up) {
        upArrow();
    } else if (eventKey == Qt::Key_Down) {
        downArrow();
    } else if (eventKey == Qt::Key_Enter || eventKey == Qt::Key_Return) {
        MathEdit *par = qobject_cast<MathEdit*>(parentObject());
        par->keyPressEvent(event);
    } else {
        if (!altPressed) {
            QGraphicsObject::keyPressEvent(event);
        }
    }
    
    // handle any funny business regarding the trailing "(" character at the end.
    // It has to be in the m_content string, but is never drawn by paint().
    QString lastChar = "";
    m_content.remove("(");
    if (!m_content.isEmpty()) { lastChar = m_content.last(1); }
    if (lastChar == "(" && getCursorPos() >= m_content.length()) setCursorTo(m_content.length() - 1);
    if (lastChar != "(") m_content.append("(");
    
    cursorPosUpdate();
    updateBoundingRect();
    update(boundingRect());
    event->accept();
}

void MathEditRoot::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    event->accept();
    m_index->setFocus();
    QGraphicsObject::mousePressEvent(event);
}

void MathEditRoot::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    QGraphicsObject::mouseMoveEvent(event);
}

bool MathEditRoot::isPartOfFuncStr(QString testStr) {
    bool b = false;
    for (QString fn: getValidFunctions()) {
        if (fn.startsWith(testStr)) {
            b = true;
            break;
        }
    }
    return b;
}

void MathEditRoot::insertTextAt(int64_t pos, const QString& inText) {
    m_argument->insertTextAt(pos, inText);
}

void MathEditRoot::insertItemAt(int64_t pos, MathEdit *id) {
    m_argument->insertItemAt(pos,id);
}

void MathEditRoot::insertItemsAt(int64_t pos, QList<MathEdit*> list) {
    m_argument->insertItemsAt(pos,list);
}

bool MathEditRoot::hasDescendantFocus() const {
    if (hasFocus()) { return true; }
    if (m_argument->hasDescendantFocus()) { return true; }
    if (m_index->hasDescendantFocus()) { return true; }
    
    return false;
}

QJsonObject MathEditRoot::toJson() const {
    QJsonObject object;
    object["type"] = type();
    object["content"] = m_content;
    object["argument"] = m_argument->toJson();
    object["index"] = m_index->toJson();
    object["mathFontSize"] = static_cast<int>(getMathFontSize());
    
    QJsonArray childItemsArray;
    for(MathEdit *child:getContItems()) {
        childItemsArray.append(child->toJson());
    }
    object["childItems"] = childItemsArray;
    
    return object;
}

void MathEditRoot::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathEdit::MERoot) {
        m_content = object["content"].toString();
        setMathFontSize(object["mathFontSize"].toInt());
        
        const QJsonObject jsonArgument = object["argument"].toObject();
        const QJsonObject jsonIndex = object["index"].toObject();
        
        MathEditFactory::createChildTree(jsonArgument, getData(), getParentPageMathItem(), m_argument);
        MathEditFactory::createChildTree(jsonIndex, getData(), getParentPageMathItem(), m_index);
        
    } else {
        qDebug() << "MathLeafEdit::fromJson: Error on loading object!";
    }
}


void MathEditRoot::initializeParenthesis() {
    m_leftParenthesisPixmap = QPixmap();
    m_rightParenthesisPixmap = QPixmap();
//    m_leftBracesPixmap = QPixmap();
//    m_rightBracesPixmap = QPixmap();

    // Initialize QGraphicsPathItems for parentheses
    m_leftParenthesis = new QGraphicsPathItem(this);
    m_rightParenthesis = new QGraphicsPathItem(this);
//    m_leftBraces = new QGraphicsPathItem(this);
//    m_rightBraces = new QGraphicsPathItem(this);
    m_leftParenthesis->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    m_rightParenthesis->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
//    m_leftBraces->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
//    m_rightBraces->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    m_leftParenthesis->setAcceptHoverEvents(false);
    m_rightParenthesis->setAcceptHoverEvents(false);
//    m_leftBraces->setAcceptHoverEvents(false);
//    m_rightBraces->setAcceptHoverEvents(false);
    if(m_paintParenthesisFast) {
        m_leftParenthesis->setPen(QPen(QColor(0, 0, 191),1));
        m_leftParenthesis->setBrush(Qt::NoBrush);
        m_rightParenthesis->setPen(QPen(QColor(0, 0, 191),1));
        m_rightParenthesis->setBrush(Qt::NoBrush);
    } else {
        m_leftParenthesis->setPen(QPen(Qt::NoPen));
        m_leftParenthesis->setBrush(QColor(0, 0, 191));
        m_rightParenthesis->setPen(QPen(Qt::NoPen));
        m_rightParenthesis->setBrush(QColor(0, 0, 191));
    }
/*    if(m_paintBracesFast) {
        m_leftBraces->setPen(QPen(QColor(0, 0, 191),1));
        m_leftBraces->setBrush(Qt::NoBrush);
        m_rightBraces->setPen(QPen(QColor(0, 0, 191),1));
        m_rightBraces->setBrush(Qt::NoBrush);
    } else {
        m_leftBraces->setPen(QPen(Qt::NoPen));
        m_leftBraces->setBrush(QColor(0, 0, 191));
        m_rightBraces->setPen(QPen(Qt::NoPen));
        m_rightBraces->setBrush(QColor(0, 0, 191));
    }
*/
    m_leftParenthesis->hide();
    m_rightParenthesis->hide();
//    m_leftBraces->hide();
//    m_rightBraces->hide();
}

void MathEditRoot::updateParenthesisPath(const QPointF& topLeft, qreal totalHeight, bool isLeftParenthesis) {
    //qDebug() << "MathLeafEdit::updateParenthesisPath";
    qreal stretchBy = totalHeight -20;
    if (stretchBy < 0.0) stretchBy = 0.0;
    qreal dx = topLeft.x();
    qreal dy = topLeft.y();
    QPointF Point1, cPoint1_1, cPoint1_2, Point2;
    QPointF Point3, cPoint3_1, cPoint3_2, Point4;
    QPointF Point5, cPoint5_1, cPoint5_2, Point6;
    QPointF Point7, cPoint7_1, cPoint7_2, Point8;
    if (isLeftParenthesis) {
        QPainterPath leftPath, leftLinePath;
        Point1    = QPointF(5.8+dx,  0.0+dy);	
        cPoint1_1 = QPointF(2.3+dx,  1.8+dy);
        cPoint1_2 = QPointF(0.0+dx,  4.3+dy);
        Point2    = QPointF(0.0+dx,  9.5+dy);
        
        Point3    = QPointF(0.0+dx, 10.5+dy + stretchBy);	
        cPoint3_1 = QPointF(0.0+dx, 15.7+dy + stretchBy);
        cPoint3_2 = QPointF(2.3+dx, 18.2+dy + stretchBy);
        Point4    = QPointF(5.8+dx, 20.0+dy + stretchBy);
        
        Point5    = QPointF(5.8+dx, 19.0+dy + stretchBy);	
        cPoint5_1 = QPointF(2.8+dx, 17.3+dy + stretchBy);
        cPoint5_2 = QPointF(2.1+dx, 13.2+dy + stretchBy);
        Point6    = QPointF(2.1+dx, 10.5+dy + stretchBy);
        
        Point7    = QPointF(2.1+dx,  9.5+dy);	
        cPoint7_1 = QPointF(2.1+dx,  6.8+dy);
        cPoint7_2 = QPointF(2.2+dx,  2.7+dy);
        Point8    = QPointF(5.8+dx,  1.0+dy);
        
        leftPath.moveTo(Point1);
        leftPath.cubicTo(cPoint1_1, cPoint1_2, Point2);
        leftPath.lineTo(Point3);
        leftPath.cubicTo(cPoint3_1, cPoint3_2, Point4);
        leftPath.lineTo(Point5);
        leftPath.cubicTo(cPoint5_1, cPoint5_2, Point6);
        leftPath.lineTo(Point7);
        leftPath.cubicTo(cPoint7_1, cPoint7_2, Point8);
        leftPath.closeSubpath();
        
        leftLinePath.moveTo(Point1);
        leftLinePath.lineTo(Point2);
        leftLinePath.lineTo(Point3);
        leftLinePath.lineTo(Point4);
        leftLinePath.lineTo(Point5);
        leftLinePath.lineTo(Point6);
        leftLinePath.lineTo(Point7);
        leftLinePath.lineTo(Point8);
        leftLinePath.closeSubpath();
        
        if (m_paintParenthesisFast) m_leftParenthesis->setPath(leftLinePath);
        else m_leftParenthesis->setPath(leftPath);
    } else {
        QPainterPath rightPath, rightLinePath;
        Point1    = QPointF(0.0+dx,  0.0+dy);	//0.0 -> 5.8
        cPoint1_1 = QPointF(3.5+dx,  1.8+dy);//2.3 -> 3.5
        cPoint1_2 = QPointF(5.8+dx,  4.3+dy);
        Point2    = QPointF(5.8+dx,  9.5+dy);
        
        Point3    = QPointF(5.8+dx, 10.5+dy + stretchBy);	
        cPoint3_1 = QPointF(5.8+dx, 15.7+dy + stretchBy);
        cPoint3_2 = QPointF(3.5+dx, 18.2+dy + stretchBy);
        Point4    = QPointF(0.0+dx, 20.0+dy + stretchBy);
        
        Point5    = QPointF(0.0+dx, 19.0+dy + stretchBy);	
        cPoint5_1 = QPointF(3.0+dx, 17.3+dy + stretchBy);//2.8 -> 3.0
        cPoint5_2 = QPointF(3.7+dx, 13.2+dy + stretchBy);//2.1 -> 3.7
        Point6    = QPointF(3.7+dx, 10.5+dy + stretchBy);
        
        Point7    = QPointF(3.7+dx,  9.5+dy);	
        cPoint7_1 = QPointF(3.7+dx,  6.8+dy);
        cPoint7_2 = QPointF(3.6+dx,  2.7+dy);//2.2 -> 3.6
        Point8    = QPointF(0.0+dx,  1.0+dy);
        
        rightPath.moveTo(Point1);
        rightPath.cubicTo(cPoint1_1, cPoint1_2, Point2);
        rightPath.lineTo(Point3);
        rightPath.cubicTo(cPoint3_1, cPoint3_2, Point4);
        rightPath.lineTo(Point5);
        rightPath.cubicTo(cPoint5_1, cPoint5_2, Point6);
        rightPath.lineTo(Point7);
        rightPath.cubicTo(cPoint7_1, cPoint7_2, Point8);
        rightPath.closeSubpath();
        
        rightLinePath.moveTo(Point1);
        rightLinePath.lineTo(Point2);
        rightLinePath.lineTo(Point3);
        rightLinePath.lineTo(Point4);
        rightLinePath.lineTo(Point5);
        rightLinePath.lineTo(Point6);
        rightLinePath.lineTo(Point7);
        rightLinePath.lineTo(Point8);
        rightLinePath.closeSubpath();
        
        if (m_paintParenthesisFast) m_rightParenthesis->setPath(rightLinePath);
        else m_rightParenthesis->setPath(rightPath);
    }
}
