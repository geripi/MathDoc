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

#include "matheditfunction.h"

#include "pagemathitem.h"
#include "helpers.h"
#include "matheditfactory.h"

#include <QDebug>
#include <QRandomGenerator>

MathEditFunction::MathEditFunction(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEdit(d, parentPageMathItem, parent) {
    m_argument = new MathEdit(d, parentPageMathItem, this);
    initializeParenthesis();
}
MathEditFunction::MathEditFunction(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEditFunction(d, parentPageMathItem, parent) {
    m_content.append(text);
    setCursorTo(0);
}

void MathEditFunction::setFocus(Qt::FocusReason focusReason) {
    MathEdit *cashedChild = getCashedChild();
    if (cashedChild == m_argument && cashedChild->getRightArrowPressed()) { // leave this towards the right
        setCursorToBegin();
        if(parentObject() != getParentPageMathItem()) { // only step out of the MathEdit if there is a MathEdit parent
            setRightArrowPressed(true);
            MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
            pa->setCashedChild(this);
            pa->setFocus();
            pa->setCursorRightOf(this);
            pa->setCashedChild(nullptr);
            setRightArrowPressed(false);
        }
    } else if (cashedChild == m_argument && cashedChild->getLeftArrowPressed()) { // leave this towards the left
        setCursorTo(getContent().size() - 1);
        QGraphicsObject::setFocus(focusReason);
    } else if (!cashedChild) {
        MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
        if (pa->getRightArrowPressed()) {
            QGraphicsObject::setFocus(focusReason);
            setCursorToBegin();
        } else if (pa->getLeftArrowPressed()) {
            m_argument->setFocus(focusReason);
            m_argument->setCursorToEnd();
        }
    } else {
        m_argument->setFocus();
        m_argument->setCursorToEnd();
    }
    setCashedChild(nullptr);
}

void MathEditFunction::setFocusToChild1(Qt::FocusReason focusReason) {
    m_argument->setFocus(focusReason);
}

void MathEditFunction::init() {
    updateBoundingRect();
}

void MathEditFunction::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
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
    qreal textW = textWidth(normStr);
    QColor textColor = Qt::darkBlue;
    painter->setPen(QPen(textColor, 1, Qt::SolidLine));
    qreal textVPos = baseline() - fm.height()*0.5 + fm.ascent();
    painter->drawText(0.0, textVPos, normStr);
    
    qreal leftParX = textW - 1;
    m_leftParenthesis->setPos(leftParX, 0);
    m_leftParenthesis->show();
    
    qreal rightParX = textW +3 + m_argument->getBoundingRectangle().x() + m_argument->getBoundingRectangle().width();
    m_rightParenthesis->setPos(rightParX, 0);
    m_rightParenthesis->show();
    
    cursorPosUpdate();
    if (isCursorVisible()) { // && i == m_cursorPos) {
        painter->setPen(QPen(Qt::black, 1));
        painter->drawLine(getCursorX()+1, getCursorY(), getCursorX()+1, getCursorH());
        painter->setPen(QPen());
    }
}

void MathEditFunction::updateBoundingRect() {
    QString normStr = "";
    if (m_content.length() > 1) normStr = m_content.first(m_content.length()-1);
    
    QRectF bRectArg = m_argument->getBoundingRectangle();
    
    qreal argY = 0.0;
    
    qreal argLength = bRectArg.width() + 2.0*getPaddingH();
    qreal argX = textWidth(normStr) + 2.0*getPaddingH();
    
    m_argument->setPos(argX, argY);
    
    qreal top = bRectArg.top();
    qreal bottom =  bRectArg.bottom();
    
    QRectF rect(0.0, top, argX + argLength + 2.0*getPaddingH(), bottom - top);
    
    prepareGeometryChange();
    setBoundingRectangle(rect);
    
    notifyParentSizeChange();
    qreal argH = m_argument->boundingRect().height();
    qreal argT = m_argument->boundingRect().top();
    updateParenthesisPath(QPointF(0, argT), argH, true);
    updateParenthesisPath(QPointF(0, argT), argH, false);
}

void MathEditFunction::compute(const MathVariable& boolMask) {
    m_argument->compute(boolMask);
    MathVariable var = m_argument->getValue();
    MathVariable res;
    if      (getContent() == QString("("))      { res = var; }
    else if (getContent() == QString("sin("))   { res = sin(var, boolMask); }
    else if (getContent() == QString("asin("))  { res = asin(var, boolMask); }
    else if (getContent() == QString("cos("))   { res = cos(var, boolMask); }
    else if (getContent() == QString("acos("))  { res = acos(var, boolMask); }
    else if (getContent() == QString("tan("))   { res = tan(var, boolMask); }
    else if (getContent() == QString("atan("))  { res = atan(var, boolMask); }
    else if (getContent() == QString("sinh("))  { res = sinh(var, boolMask); }
    else if (getContent() == QString("asinh(")) { res = asinh(var, boolMask); }
    else if (getContent() == QString("cosh("))  { res = cosh(var, boolMask); }
    else if (getContent() == QString("acosh(")) { res = acosh(var, boolMask); }
    else if (getContent() == QString("tanh("))  { res = tanh(var, boolMask); }
    else if (getContent() == QString("atanh(")) { res = atanh(var, boolMask); }
    else if (getContent() == QString("log10(")) { res = log10(var, boolMask); }
    else if (getContent() == QString("ln("))    { res = ln(var, boolMask); }
    else if (getContent() == QString("log2("))  { res = log2(var, boolMask); }
    else if (getContent() == QString("abs("))   { res = abs(var, boolMask); }
    else if (getContent() == QString("round(")) { res = round(var, boolMask); }
    else if (getContent() == QString("floor(")) { res = floor(var, boolMask); }
    else if (getContent() == QString("ceil("))  { res = ceil(var, boolMask); }
    else if (getContent() == QString("trunc(")) { res = trunc(var, boolMask); }
    //else if (getContent() == QString("mod(")) { res = mod(var, boolMask); }
    else if (getContent() == QString("sign("))  { res = sign(var, boolMask); }
    else if (getContent() == QString("min("))   { res = min(var, boolMask); }
    else if (getContent() == QString("max("))   { res = max(var, boolMask); }
    
    setValue(res);
}

void MathEditFunction::keyPressEvent(QKeyEvent* event)
{
    //printContent();
    const bool altPressed = event->modifiers() & Qt::AltModifier;
    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
    
    QString charToInsert = event->text();
    int eventKey = event->key();
    
    if (functionCharacters.contains(charToInsert) && charToInsert != "") {
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
    if (getCursorPos() >= getContent().length()) setCursorTo(getContent().length());
    if (lastChar == "(" && getCursorPos() >= getContent().length()) setCursorTo(getContent().length() - 1);
    if (lastChar != "(") setContent(getContent() + QString("("));
    
    cursorPosUpdate();
    updateBoundingRect();
    update(boundingRect());
    event->accept();
}

void MathEditFunction::leftArrow() {
    setLeftArrowPressed(true);
    setSelectBegin(-1); setSelectEnd(-1); setSelectAnchor(-1);
    if(getCursorPos()>0) {
        setCursorTo(getCursorPos()-1);
    } else {
        if(parentObject() != getParentPageMathItem()) { // only step out of the MathEdit if there is a MathEdit parent
            MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
            pa->setCashedChild(this);
            pa->setFocus();
            pa->setCursorLeftOf(this);
            pa->setCashedChild(nullptr);
        }
    }
    setLeftArrowPressed(false);
}

void MathEditFunction::rightArrow() {
    setRightArrowPressed(true);
    setSelectBegin(-1); setSelectEnd(-1); setSelectAnchor(-1);
    if(getCursorPos()<getContent().length()-1) {
        setCursorTo(getCursorPos()+1);
    } else {
        m_argument->setFocus();
        m_argument->setCursorToBegin();
    }
    setRightArrowPressed(false);
}


bool MathEditFunction::isPartOfFuncStr(QString testStr) {
    bool b = false;
    for (QString fn: getValidFunctions()) {
        if (fn.startsWith(testStr)) {
            b = true;
            break;
        }
    }
    return b;
}

void MathEditFunction::insertTextAt(int32_t pos, const QString& inText) {
    m_argument->insertTextAt(pos, inText);
}

void MathEditFunction::insertItemAt(int32_t pos, MathEdit *id) {
    m_argument->insertItemAt(pos,id);
}

void MathEditFunction::insertItemsAt(int32_t pos, QList<MathEdit*> list) {
    m_argument->insertItemsAt(pos,list);
}

bool MathEditFunction::hasDescendantFocus() const {
    if (hasFocus()) { return true; }
    if (m_argument->hasDescendantFocus()) { return true; }
    
    return false;
}

void MathEditFunction::setCursorRightOf(MathEdit* child) {
    if (child == m_argument) {
        
    } else {
        setCursorTo(0);
    }
}

void MathEditFunction::setCursorLeftOf(MathEdit* child) {
    if (child == m_argument) {
        setCursorTo(getContent().length() - 1);
    } else {
        setCursorTo(0);
    }
}

QJsonObject MathEditFunction::toJson() const {
    QJsonObject object;
    object["type"] = type();
    object["content"] = m_content;
    object["argument"] = m_argument->toJson();
    object["mathFontSize"] = getMathFontSize();
    
    QJsonArray childItemsArray;
    for(MathEdit *child:getContItems()) {
        childItemsArray.append(child->toJson());
    }
    object["childItems"] = childItemsArray;
    
    return object;
}

void MathEditFunction::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathEdit::MEFunction) {
        m_content = object["content"].toString();
        setMathFontSize(object["mathFontSize"].toInt());
        
        const QJsonObject jsonArgument = object["argument"].toObject();
        
        MathEditFactory::createChildTree(jsonArgument, getData(), getParentPageMathItem(), m_argument);
        
    } else {
        qDebug() << "MathLeafEdit::fromJson: Error on loading object!";
    }
}

void MathEditFunction::initializeParenthesis() {
    m_leftParenthesisPixmap = QPixmap();
    m_rightParenthesisPixmap = QPixmap();
//    m_leftBracesPixmap = QPixmap();
//    m_rightBracesPixmap = QPixmap();

    // Initialize QGraphicsPathItems for parentheses
    m_leftParenthesis = new QGraphicsPathItem(this);
    m_rightParenthesis = new QGraphicsPathItem(this);
//    m_leftBraces = new QGraphicsPathItem(this);
//    m_rightBraces = new QGraphicsPathItem(this);
    //m_leftParenthesis->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    //m_rightParenthesis->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    m_leftParenthesis->setCacheMode(QGraphicsItem::NoCache);
    m_rightParenthesis->setCacheMode(QGraphicsItem::NoCache);
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

void MathEditFunction::updateParenthesisPath(const QPointF& topLeft, qreal totalHeight, bool isLeftParenthesis) {
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
