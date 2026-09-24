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

#include "matheditconditionalfunc.h"
#include "matheditcreatenew.h"
#include "helpers.h"
#include "matheditfactory.h"
#include "pagemathitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QRandomGenerator>

MathEditConditionalFunc::MathEditConditionalFunc(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEdit(d, parentPageMathItem, parent) {
    initializeBraces();
}
MathEditConditionalFunc::MathEditConditionalFunc(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEditConditionalFunc(d, parentPageMathItem, parent) {
    m_content.append(text);
    setCursorTo(0);
}

void MathEditConditionalFunc::initialize() {
    m_boolList.append(new MathEdit(getData(), getParentPageMathItem(), this));
    m_exprList.append(new MathEdit(getData(), getParentPageMathItem(), this));
}

void MathEditConditionalFunc::setFocus(Qt::FocusReason focusReason) {
    Q_UNUSED(focusReason);
    
    MathEdit *cashedChild = getCashedChild();
    
    bool cashedChildIsExpr;
    int32_t lineID = 0;
    for (int32_t i = 0; i<getExprList().length(); i++) {
        if (getExprList().at(i) == cashedChild) {
            lineID = i;
            cashedChildIsExpr = true;
            break;
        }
        if (getBoolList().at(i) == cashedChild) {
            lineID = i;
            cashedChildIsExpr = false;
            break;
        }
    }
    if (cashedChild) {
        if (cashedChild->getRightArrowPressed()) {
            if (cashedChildIsExpr) {
                if (lineID != getExprList().length() -1) { // there is another line, set cursor to begin of next line
                    getBoolList().at(lineID + 1)->setFocus();
                    getBoolList().at(lineID + 1)->setCursorToBegin();
                } else { // this was the last line: exit right
                    setCursorToEnd();
                    rightArrow();
                }
            } else { // Cashed child is a bool item
                getExprList().at(lineID)->setFocus();
                getExprList().at(lineID)->setCursorToBegin();
            }
        } else if (cashedChild->getLeftArrowPressed()) {
            if (cashedChildIsExpr) {
                getBoolList().at(lineID)->setFocus();
                getBoolList().at(lineID)->setCursorToEnd();
            } else { // Cashed child is a bool item
                if (lineID != 0) { // there is another line above, set cursor to begin of next line above
                    getExprList().at(lineID - 1)->setFocus();
                    getExprList().at(lineID - 1)->setCursorToEnd();
                } else { // this was the first line: exit left
                    setCursorToBegin();
                    leftArrow();
                }
            }
        }
    } else { // no cashedChild is given, set cursor to begin of first bool item or end of last expr item
        MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
        if (pa->getRightArrowPressed()) {
            getBoolList().at(0)->setFocus();
            getBoolList().at(0)->setCursorToBegin();
        } else if (pa->getLeftArrowPressed()) {
            getExprList().at(getExprList().length()-1)->setFocus();
            getExprList().at(getExprList().length()-1)->setCursorToEnd();
        } else {
            getBoolList().at(0)->setFocus();
            getBoolList().at(0)->setCursorToBegin();
        }
    }
    setCashedChild(nullptr);
}

void MathEditConditionalFunc::setFocusToBoolItem(int32_t ID, Qt::FocusReason focusReason) {
    m_boolList.at(ID)->setFocus(focusReason);
}
void MathEditConditionalFunc::setFocusToExprItem(int32_t ID, Qt::FocusReason focusReason) {
    m_exprList.at(ID)->setFocus(focusReason);
}

void MathEditConditionalFunc::init() {
    updateBoundingRect();
}

void MathEditConditionalFunc::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
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
        if(getContent().isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 223, 223, 127));
            painter->drawRect(getBoundingRectangle());
        } else {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(getBoundingRectangle());
        }
    }
    
    QColor textColor = Qt::darkGreen;
    painter->setPen(QPen(textColor, 1, Qt::SolidLine)); painter->setFont(getFont());
    for (int32_t i = 0; i<m_boolList.length(); i++) {
        painter->drawText(QPointF(m_exprList.at(i)->pos().x() - 9.5*getPaddingH(),
                          m_exprList.at(i)->pos().y() + m_exprList.at(i)->baseline() + 0.4*fm.ascent()),
                          ":");
    }
    
    m_leftBraces->setPos(0, 0);
    m_leftBraces->show();
    m_rightBraces->setPos(getBoundingRectangle().width() - 3.0*getPaddingH(), 0);
    m_rightBraces->show();
    
    cursorPosUpdate();
    if (isCursorVisible()) { // && i == m_cursorPos) {
        painter->setPen(QPen(Qt::black, 1));
        painter->drawLine(QPointF(getCursorX()+1, getCursorY()), QPointF(getCursorX()+1, getCursorH()));
        painter->setPen(QPen());
    }
}

void MathEditConditionalFunc::updateBoundingRect() {
    const qsizetype count = qMin(m_boolList.size(), m_exprList.size());
    if (count == 0) {
        prepareGeometryChange();
        setBoundingRectangle(QRectF());
        notifyParentSizeChange();
        return;
    }
    qreal maxBoolW = 0.0;
    qreal maxExprW = 0.0;
    for (int i = 0; i < count; ++i) {
        maxBoolW = qMax( maxBoolW, m_boolList.at(i)->getBoundingRectangle().width() );
        maxExprW = qMax( maxExprW, m_exprList.at(i)->getBoundingRectangle().width());
    }
    const qreal rowGap = getPaddingV();
    
    QVector<qreal> boolY(count);
    QVector<qreal> exprY(count);
    
    /* First calculate the vertical layout relative to an
     * arbitrary first baseline at y = 0. */
    qreal currentBaseline = 0.0;
    qreal overallTop = std::numeric_limits<qreal>::max();
    qreal overallBottom = std::numeric_limits<qreal>::lowest();
    
    for (int i = 0; i < count; ++i) {
        MathEdit *boolEdit = m_boolList.at(i);
        MathEdit *exprEdit = m_exprList.at(i);
        
        const QRectF boolRect = boolEdit->getBoundingRectangle();
        const QRectF exprRect = exprEdit->getBoundingRectangle();
        
        /* Position both children so that their baselines
         * coincide at currentBaseline.
         * childPosY + childBaseline = currentBaseline */
        boolY[i] = currentBaseline - boolEdit->baseline();
        exprY[i] = currentBaseline - exprEdit->baseline();
        
        /* Calculate actual row extents. */
        const qreal boolTop = boolY[i] + boolRect.top();
        const qreal boolBottom = boolY[i] + boolRect.bottom();
        const qreal exprTop = exprY[i] + exprRect.top();
        const qreal exprBottom = exprY[i] + exprRect.bottom();
        const qreal rowTop = qMin(boolTop, exprTop);
        const qreal rowBottom = qMax(boolBottom, exprBottom);
        
        overallTop = qMin(overallTop, rowTop);
        overallBottom = qMax(overallBottom, rowBottom);
        
        /* Calculate the amount below THIS row's baseline. */
        const qreal rowBelow =
        rowBottom - currentBaseline;
        
        /* Find the amount above the NEXT row's baseline. */
        if (i + 1 < count) {
            
            MathEdit *nextBool = m_boolList.at(i + 1);
            MathEdit *nextExpr = m_exprList.at(i + 1);
            
            const QRectF nextBoolRect = nextBool->getBoundingRectangle();
            const QRectF nextExprRect = nextExpr->getBoundingRectangle();
            
            const qreal nextBoolAbove = nextBool->baseline() - nextBoolRect.top();
            const qreal nextExprAbove = nextExpr->baseline() - nextExprRect.top();
            const qreal nextRowAbove = qMax(nextBoolAbove, nextExprAbove);
            
            /* Place next baseline far enough below the
             * current row that the two bounding rectangles
             * cannot overlap. */
            currentBaseline += rowBelow + rowGap + nextRowAbove;
        }
    }
    
    /* Now we know the complete vertical extent.
     * Center it around y = 0 so that this object's baseline
     * is at the vertical center of its bounding rectangle. */
    const qreal height = overallBottom - overallTop;
    const qreal center = 0.5 * (overallTop + overallBottom);
    const qreal yShift = -center;
    
    /* Actually position the children.*/
    for (int i = 0; i < count; ++i) {
        m_boolList.at(i)->setPos(3.0*getPaddingH(), boolY[i] + yShift);
        m_exprList.at(i)->setPos(3.0*getPaddingH() + maxBoolW + 10.0*getPaddingH(), exprY[i] + yShift);
    }
    const qreal width = 10.0*getPaddingH() + maxBoolW + 3.0*getPaddingH() + maxExprW + 4.0*getPaddingH();
    
    QRectF rect( 0.0, -height * 0.5, width, height);
    
    prepareGeometryChange();
    setBoundingRectangle(rect);
    
    notifyParentSizeChange();
    
    /* Brace geometry. */
    const QRectF firstBoolRect = m_boolList.first()->getBoundingRectangle();
    const QRectF lastBoolRect = m_boolList.last()->getBoundingRectangle();
    const QRectF firstExprRect = m_exprList.first()->getBoundingRectangle();
    const QRectF lastExprRect = m_exprList.last()->getBoundingRectangle();
    
    const qreal argBoolTop = m_boolList.first()->pos().y() + firstBoolRect.top();
    const qreal argExprTop = m_exprList.first()->pos().y() + firstExprRect.top();
    const qreal argTop = qMin(argBoolTop, argExprTop);
    
    const qreal argBoolBottom = m_boolList.last()->pos().y() + lastBoolRect.bottom();
    const qreal argExprBottom = m_exprList.last()->pos().y() + lastExprRect.bottom();
    const qreal argBottom = qMax(argBoolBottom, argExprBottom);
    
    const qreal argHeight = argBottom - argTop;
    
    updateBracesPath( QPointF(0.0, argTop), argHeight, true);
    updateBracesPath( QPointF(0.0, argTop), argHeight, false );
}

void MathEditConditionalFunc::compute(const MathVariable& boolMask) {
    Q_UNUSED(boolMask);
    
    for (MathEdit *m: m_boolList)  {
        m->compute();
    }
    
    MathVariable value = m_boolList.at(0)->getValue(); // Get the correct number of entries
    for (int32_t i = 0; i< m_boolList.length(); i++) {
        MathVariable boolM = m_boolList.at(i)->getValue();
        m_exprList.at(i)->compute(boolM);
        MathVariable exprM = m_exprList.at(i)->getValue();
        for (int32_t k = 0; k< boolM.size(); k++) {
            if (boolM[k] != 0.0) {
                value[k] = exprM[k];
            }
        }
    }
    
    
    value.setUnit(m_exprList.at(0)->getValue().unit());
    // Check if units are the same everywhere
    bool allUnitsAreTheSame = true;
    for (MathEdit *m: m_exprList) {
        if (value.unit() != m->getValue().unit()) {
            allUnitsAreTheSame = false;
            setMathError(QString("ERROR: Not all results in conditional function have the same unit"));
            break;
        }
    }
    if (allUnitsAreTheSame) setValue(value);
}

void MathEditConditionalFunc::setFocusToChild1(Qt::FocusReason focusReason) {
    Q_UNUSED(focusReason);
    
    getBoolList().at(0)->setFocus();
}

void MathEditConditionalFunc::setFocusToChild2(Qt::FocusReason focusReason) {
    Q_UNUSED(focusReason);
    getExprList().at(0)->setFocus();
}


void MathEditConditionalFunc::keyPressEvent(QKeyEvent* event)
{
    //printContent();
    const bool altPressed = event->modifiers() & Qt::AltModifier;
    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
    Q_UNUSED(ctrlPressed);
    
    QString charToInsert = event->text();
    int eventKey = event->key();
    
    
    if (eventKey == Qt::Key_Backspace) {
        backspace();
        emit getParentPageMathItem()->itemDataChanged();
    } else if (eventKey == Qt::Key_Delete) {
        del();
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
        enter();
        emit getParentPageMathItem()->itemDataChanged();
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

void MathEditConditionalFunc::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    event->accept();
    QGraphicsObject::mousePressEvent(event);
}

void MathEditConditionalFunc::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    QGraphicsObject::mouseMoveEvent(event);
}

bool MathEditConditionalFunc::hasDescendantFocus() const {
    if (hasFocus()) { return true; }
    for (MathEdit *m: m_boolList) {
        if (m->hasDescendantFocus()) { return true; }
    }
    for (MathEdit *m: m_exprList) {
        if (m->hasDescendantFocus()) { return true; }
    }
    
    return false;
}

QJsonObject MathEditConditionalFunc::toJson() const {
    QJsonObject object;
    object["type"] = type();
    object["content"] = m_content;
    //object["argument"] = m_argument->toJson();
    //object["base"] = m_base->toJson();
    object["mathFontSize"] = static_cast<int>(getMathFontSize());
    
    QJsonArray boolListArray;
    for(MathEdit *item: getBoolList()) {
        boolListArray.append(item->toJson());
    }
    object["boolList"] = boolListArray;
    
    QJsonArray exprListArray;
    for(MathEdit *item: getExprList()) {
        exprListArray.append(item->toJson());
    }
    object["exprList"] = exprListArray;
    
    return object;
}

void MathEditConditionalFunc::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathEdit::MEConditionalFunc) {
        m_content = object["content"].toString();
        setMathFontSize(object["mathFontSize"].toInt());
        
        const QJsonArray jsonBoolList = object["boolList"].toArray();
        const QJsonArray jsonExprList = object["exprList"].toArray();
        
        for (const QJsonValue& value : jsonBoolList) {
            if (!value.isObject()) {
                qWarning() << "MathEditConditionalFunc::fromJson: jsonBoolList contains a value which is not an object.";
                continue;
            }
            MathEdit* child = MathEditFactory::fromJsonFact(value.toObject(), getData(), getParentPageMathItem(), this);
            if (child) { m_boolList.append(child); }
        }
        for (const QJsonValue& value : jsonExprList) {
            if (!value.isObject()) {
                qWarning() << "MathEditConditionalFunc::fromJson: jsonExprList contains a value which is not an object.";
                continue;
            }
            MathEdit* child = MathEditFactory::fromJsonFact(value.toObject(), getData(), getParentPageMathItem(), this);
            if (child) { m_exprList.append(child); }
        }
    } else {
        qDebug() << "MathLeafEdit::fromJson: Error on loading object!";
    }
}

bool MathEditConditionalFunc::shiftDel() {
    if (getExprList().length() <= 1) return false; // Do not delete last remaining line
    
    MathEdit *cashedChild = getCashedChild();
    if (!cashedChild) return false;
    
    int32_t lineID = 0;
    for (int32_t i = 0; i<getExprList().length(); i++) {
        if (getExprList().at(i) == cashedChild) {
            lineID = i;
            break;
        }
        if (getBoolList().at(i) == cashedChild) {
            lineID = i;
            break;
        }
    }
    setCashedChild(nullptr);
    
    getParentPageMathItem()->blockCompute();
    MathEdit *expr = m_exprList.at(lineID);
    MathEdit *boolean = m_boolList.at(lineID);
    
    m_exprList.removeAt(lineID);
    m_boolList.removeAt(lineID);
    
    expr->deleteLater();
    boolean->deleteLater();
    getParentPageMathItem()->releaseCompute();
    
    QMetaObject::invokeMethod(this, [this]() {
        cursorPosUpdate();
        updateBoundingRect();
        update(boundingRect());
    }, Qt::QueuedConnection);    
    
    return true;
}

void MathEditConditionalFunc::enter() {
    MathEdit *cashedChild = getCashedChild();
    
    bool cashedChildIsExpr=false;
    int64_t lineID = 0;
    for (int64_t i = 0; i<getExprList().length(); i++) {
        if (getExprList().at(i) == cashedChild) {
            lineID = i;
            cashedChildIsExpr = true;
            break;
        }
        if (getBoolList().at(i) == cashedChild) {
            lineID = i;
            cashedChildIsExpr = false;
            break;
        }
    }
    
    if (cashedChildIsExpr) {
        m_boolList.insert(lineID+1, new MathEdit(getData(), getParentPageMathItem(), this));
        m_exprList.insert(lineID+1, new MathEdit(getData(), getParentPageMathItem(), this));
    } else {
        m_boolList.insert(lineID, new MathEdit(getData(), getParentPageMathItem(), this));
        m_exprList.insert(lineID, new MathEdit(getData(), getParentPageMathItem(), this));
    }
    updateBoundingRect();
}

void MathEditConditionalFunc::initializeBraces() {
    m_leftBracesPixmap = QPixmap();
    m_rightBracesPixmap = QPixmap();

    // Initialize QGraphicsPathItems for parentheses
    m_leftBraces = new QGraphicsPathItem(this);
    m_rightBraces = new QGraphicsPathItem(this);
    m_leftBraces->setCacheMode(QGraphicsItem::NoCache);
    m_rightBraces->setCacheMode(QGraphicsItem::NoCache);
    m_leftBraces->setAcceptHoverEvents(false);
    m_rightBraces->setAcceptHoverEvents(false);
    if(m_paintBracesFast) {
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

    m_leftBraces->hide();
    m_rightBraces->hide();
}

void MathEditConditionalFunc::updateBracesPath(const QPointF& topLeft, qreal totalHeight, bool isLeftBrace) {
    //qDebug() << "MathEditConditionalFunc::updateParenthesisPath";
    qreal stretchBy = 0.5 * (totalHeight -20); // Stretch in upper and lower half 0.5 times
    if (stretchBy < 0.0) stretchBy = 0.0;
    qreal dx = topLeft.x();
    qreal dy = topLeft.y();
    QPointF Point1, cPoint1_1, cPoint1_2;
    QPointF Point2, cPoint2_1, cPoint2_2;
    QPointF Point3, cPoint3_1, cPoint3_2;
    QPointF Point4, cPoint4_1, cPoint4_2; Q_UNUSED(cPoint4_1); Q_UNUSED(cPoint4_2);
    QPointF Point5, cPoint5_1, cPoint5_2;
    QPointF Point6, cPoint6_1, cPoint6_2;
    QPointF Point7, cPoint7_1, cPoint7_2;
    QPointF Point8, cPoint8_1, cPoint8_2; Q_UNUSED(cPoint8_1); Q_UNUSED(cPoint8_2);
    QPointF Point9, cPoint9_1, cPoint9_2;
    QPointF Point10, cPoint10_1, cPoint10_2;
    QPointF Point11, cPoint11_1, cPoint11_2;
    QPointF Point12, cPoint12_1, cPoint12_2;
    QPointF Point13, cPoint13_1, cPoint13_2;
    QPointF Point14, cPoint14_1, cPoint14_2;
    QPointF Point15;
    if (isLeftBrace) {
        QPainterPath leftPath, leftLinePath;
        Point1     = QPointF(5.8+dx,  0.0+dy);	
        cPoint1_1  = QPointF(1.6+dx,  0.0+dy);
        cPoint1_2  = QPointF(0.5+dx,  1.5+dy);
        Point2     = QPointF(0.5+dx,  3.2+dy);
        cPoint2_1  = QPointF(0.5+dx,  4.9+dy);
        cPoint2_2  = QPointF(1.6+dx,  5.7+dy + stretchBy);
        Point3     = QPointF(1.6+dx,  7.3+dy + stretchBy);	
        cPoint3_1  = QPointF(1.6+dx,  9.6+dy + stretchBy);
        cPoint3_2  = QPointF(0.1+dx,  9.3+dy + stretchBy);
        Point4     = QPointF(0.0+dx,  9.3+dy + stretchBy);
        
        Point5     = QPointF(0.0+dx, 10.7+dy + stretchBy);	
        cPoint5_1  = QPointF(0.1+dx, 10.7+dy + stretchBy);
        cPoint5_2  = QPointF(1.6+dx, 10.4+dy + stretchBy);
        Point6     = QPointF(1.6+dx, 12.7+dy + stretchBy);
        cPoint6_1  = QPointF(1.6+dx, 14.3+dy + stretchBy);
        cPoint6_2  = QPointF(0.5+dx, 15.1+dy + 2.0*stretchBy);
        Point7     = QPointF(0.5+dx, 16.8+dy + 2.0*stretchBy);	
        cPoint7_1  = QPointF(0.5+dx, 18.5+dy + 2.0*stretchBy);
        cPoint7_2  = QPointF(1.6+dx, 20.0+dy + 2.0*stretchBy);
        Point8     = QPointF(5.8+dx, 20.0+dy + 2.0*stretchBy);
        
        Point9     = QPointF(5.8+dx, 19.0+dy + 2.0*stretchBy);	
        cPoint9_1  = QPointF(5.6+dx, 18.9+dy + 2.0*stretchBy);
        cPoint9_2  = QPointF(1.8+dx, 18.7+dy + 2.0*stretchBy);
        Point10    = QPointF(1.8+dx, 16.7+dy + 2.0*stretchBy);	
        cPoint10_1 = QPointF(1.8+dx, 15.4+dy + 2.0*stretchBy);
        cPoint10_2 = QPointF(2.7+dx, 14.1+dy + stretchBy);
        Point11    = QPointF(2.7+dx, 12.7+dy + stretchBy);	
        cPoint11_1 = QPointF(2.7+dx, 11.1+dy + stretchBy);
        cPoint11_2 = QPointF(1.7+dx, 10.0+dy + stretchBy);
        Point12    = QPointF(1.0+dx, 10.0+dy + stretchBy);
        cPoint12_1 = QPointF(1.7+dx, 10.0+dy + stretchBy);
        cPoint12_2 = QPointF(2.7+dx,  8.9+dy + stretchBy);
        Point13    = QPointF(2.7+dx,  7.3+dy + stretchBy);	
        cPoint13_1 = QPointF(2.7+dx,  5.9+dy + stretchBy);
        cPoint13_2 = QPointF(1.8+dx,  4.6+dy);
        Point14    = QPointF(1.8+dx,  3.3+dy);
        cPoint14_1 = QPointF(1.8+dx,  1.3+dy);
        cPoint14_2 = QPointF(5.6+dx,  1.1+dy);
        Point15    = QPointF(5.8+dx,  1.0+dy);	
        
        leftPath.moveTo(Point1);
        leftPath.cubicTo(cPoint1_1, cPoint1_2, Point2);
        leftPath.cubicTo(cPoint2_1, cPoint2_2, Point3);
        leftPath.cubicTo(cPoint3_1, cPoint3_2, Point4);
        leftPath.lineTo(Point5);
        leftPath.cubicTo(cPoint5_1, cPoint5_2, Point6);
        leftPath.cubicTo(cPoint6_1, cPoint6_2, Point7);
        leftPath.cubicTo(cPoint7_1, cPoint7_2, Point8);
        leftPath.lineTo(Point9);
        leftPath.cubicTo(cPoint9_1, cPoint9_2, Point10);
        leftPath.cubicTo(cPoint10_1, cPoint10_2, Point11);
        leftPath.cubicTo(cPoint11_1, cPoint11_2, Point12);
        leftPath.cubicTo(cPoint12_1, cPoint12_2, Point13);
        leftPath.cubicTo(cPoint13_1, cPoint13_2, Point14);
        leftPath.cubicTo(cPoint14_1, cPoint14_2, Point15);
        leftPath.closeSubpath();
        
        m_leftBraces->setPath(leftPath);
    } else {
        QPainterPath rightPath, rightLinePath;
        Point1     = QPointF(5.8-5.8+dx,  0.0+dy);	
        cPoint1_1  = QPointF(5.8-1.6+dx,  0.0+dy);
        cPoint1_2  = QPointF(5.8-0.5+dx,  1.5+dy);
        Point2     = QPointF(5.8-0.5+dx,  3.2+dy);
        cPoint2_1  = QPointF(5.8-0.5+dx,  4.9+dy);
        cPoint2_2  = QPointF(5.8-1.6+dx,  5.7+dy + stretchBy);
        Point3     = QPointF(5.8-1.6+dx,  7.3+dy + stretchBy);	
        cPoint3_1  = QPointF(5.8-1.6+dx,  9.6+dy + stretchBy);
        cPoint3_2  = QPointF(5.8-0.1+dx,  9.3+dy + stretchBy);
        Point4     = QPointF(5.8-0.0+dx,  9.3+dy + stretchBy);
        
        Point5     = QPointF(5.8-0.0+dx, 10.7+dy + stretchBy);	
        cPoint5_1  = QPointF(5.8-0.1+dx, 10.7+dy + stretchBy);
        cPoint5_2  = QPointF(5.8-1.6+dx, 10.4+dy + stretchBy);
        Point6     = QPointF(5.8-1.6+dx, 12.7+dy + stretchBy);
        cPoint6_1  = QPointF(5.8-1.6+dx, 14.3+dy + stretchBy);
        cPoint6_2  = QPointF(5.8-0.5+dx, 15.1+dy + 2.0*stretchBy);
        Point7     = QPointF(5.8-0.5+dx, 16.8+dy + 2.0*stretchBy);	
        cPoint7_1  = QPointF(5.8-0.5+dx, 18.5+dy + 2.0*stretchBy);
        cPoint7_2  = QPointF(5.8-1.6+dx, 20.0+dy + 2.0*stretchBy);
        Point8     = QPointF(5.8-5.8+dx, 20.0+dy + 2.0*stretchBy);
        
        Point9     = QPointF(5.8-5.8+dx, 19.0+dy + 2.0*stretchBy);	
        cPoint9_1  = QPointF(5.8-5.6+dx, 18.9+dy + 2.0*stretchBy);
        cPoint9_2  = QPointF(5.8-1.8+dx, 18.7+dy + 2.0*stretchBy);
        Point10    = QPointF(5.8-1.8+dx, 16.7+dy + 2.0*stretchBy);	
        cPoint10_1 = QPointF(5.8-1.8+dx, 15.4+dy + 2.0*stretchBy);
        cPoint10_2 = QPointF(5.8-2.7+dx, 14.1+dy + stretchBy);
        Point11    = QPointF(5.8-2.7+dx, 12.7+dy + stretchBy);	
        cPoint11_1 = QPointF(5.8-2.7+dx, 11.1+dy + stretchBy);
        cPoint11_2 = QPointF(5.8-1.7+dx, 10.0+dy + stretchBy);
        Point12    = QPointF(5.8-1.0+dx, 10.0+dy + stretchBy);
        cPoint12_1 = QPointF(5.8-1.7+dx, 10.0+dy + stretchBy);
        cPoint12_2 = QPointF(5.8-2.7+dx,  8.9+dy + stretchBy);
        Point13    = QPointF(5.8-2.7+dx,  7.3+dy + stretchBy);	
        cPoint13_1 = QPointF(5.8-2.7+dx,  5.9+dy + stretchBy);
        cPoint13_2 = QPointF(5.8-1.8+dx,  4.6+dy);
        Point14    = QPointF(5.8-1.8+dx,  3.3+dy);
        cPoint14_1 = QPointF(5.8-1.8+dx,  1.3+dy);
        cPoint14_2 = QPointF(5.8-5.6+dx,  1.1+dy);
        Point15    = QPointF(5.8-5.8+dx,  1.0+dy);	
        
        rightPath.moveTo(Point1);
        rightPath.cubicTo(cPoint1_1, cPoint1_2, Point2);
        rightPath.cubicTo(cPoint2_1, cPoint2_2, Point3);
        rightPath.cubicTo(cPoint3_1, cPoint3_2, Point4);
        rightPath.lineTo(Point5);
        rightPath.cubicTo(cPoint5_1, cPoint5_2, Point6);
        rightPath.cubicTo(cPoint6_1, cPoint6_2, Point7);
        rightPath.cubicTo(cPoint7_1, cPoint7_2, Point8);
        rightPath.lineTo(Point9);
        rightPath.cubicTo(cPoint9_1, cPoint9_2, Point10);
        rightPath.cubicTo(cPoint10_1, cPoint10_2, Point11);
        rightPath.cubicTo(cPoint11_1, cPoint11_2, Point12);
        rightPath.cubicTo(cPoint12_1, cPoint12_2, Point13);
        rightPath.cubicTo(cPoint13_1, cPoint13_2, Point14);
        rightPath.cubicTo(cPoint14_1, cPoint14_2, Point15);
        rightPath.closeSubpath();
        
        m_rightBraces->setPath(rightPath);
    }
}

