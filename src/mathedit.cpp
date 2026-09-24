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

#include "mathedit.h"
#include "matheditcreatenew.h"
#include "helpers.h"
#include "pagemathitem.h"

#include <iostream>
#include <QFontMetricsF>
#include <QTextLayout>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QValidator>
#include <QRandomGenerator>
#include <QException>

MathEdit::MathEdit(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
: QGraphicsObject(parent), m_data(d), m_parentPageMathItem(parentPageMathItem){
    setFlags(ItemIsFocusable);
    setAcceptHoverEvents(true);
    
    
    if (parentPageMathItem != parent && parentPageMathItem != nullptr) {
        MathEdit *p = qobject_cast<MathEdit*>(parentObject());
        m_mathFontSize = p->getMathFontSize();
    } else {
        m_mathFontSize = 14;
    }
    QFont font(m_fontName, static_cast<int>(m_mathFontSize));
    m_font = font;
    QFont subfont(m_fontName, qRound(static_cast<qreal>(m_mathFontSize)*m_subScriptScale));
    m_subfont = subfont;
    
    setSelected(true);
    
    connect(&m_cursorTimer, &QTimer::timeout, this, &MathEdit::toggleCursor);
    
    setZValue(parent->zValue()+1);
    QMetaObject::invokeMethod(this, "refreshLayout", Qt::QueuedConnection);
    
    
}

MathEdit::MathEdit(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
: MathEdit(d, parentPageMathItem, parent) {
    processInitText(text);
}

void MathEdit::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    QBrush selectBrush(QColor(192, 192, 255));
    
    painter->setRenderHint(QPainter::Antialiasing, false);
    QFontMetricsF fm(m_font), subFm(m_subfont);
    
    if (hasFocus()) {
        //int r = QRandomGenerator::global()->bounded(256);
        painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
        painter->setBrush(QColor(255, 255, 127)); // Qt::lightyellow does not exist
        //painter->setBrush(QColor(r, 255, 127)); // Qt::lightyellow does not exist
        painter->drawRect(m_boundingRectangle);
    } else if (getParentPageMathItem() && getParentPageMathItem()->showStructure()) {
        painter->setPen(QPen(Qt::darkGray, 1, Qt::DotLine));
        painter->setBrush(QColor(239, 239, 239));
        painter->drawRect(m_boundingRectangle);
    } else {
        if(m_content.isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::gray);
            painter->drawRect(m_boundingRectangle);
        } else {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(m_boundingRectangle);
        }
    }
    
    int isString = 0, isChild = 1;
    
    QList<int> typeOfItem; // List of which chunks are occuring in m_content
    QChar lastItem = '#';
    for (QChar item: m_content) {
        if(item == '#') typeOfItem.append(isChild);
        if(lastItem == '#' && item != '#') typeOfItem.append(isString);
        lastItem = item;
    }
    
    // Selection highlighting: ------------------------------------------------------------
    if ( m_selectBegin >= 0 && m_selectEnd >= 0 && m_selectBegin != m_selectEnd) {
        qreal xBegin = getPositionForIndex(m_selectBegin);
        qreal xEnd = getPositionForIndex(m_selectEnd);
        QRectF normSelRect(xBegin, m_boundingRectangle.top(), xEnd-xBegin, m_boundingRectangle.height());
        painter->setPen(QPen(Qt::NoPen)); painter->setBrush(selectBrush);
        painter->drawRect(normSelRect);
    }
        
    int32_t childNum = 0, strNum = 0;
    QStringList contList = m_content.split('#', Qt::SkipEmptyParts);
    qreal currentX = 0.0;
    qreal currentY = 0.0;//baseline();
    if(!m_content.isEmpty()) {
        for(int32_t t: typeOfItem) {
            if (t == isString) {
                QString contStr = contList[strNum]; strNum++;
                QString normStr = contStr.section('_', 0, 0);
                QString subStr = contStr.section('_', 1, -1);
                
                int chunkType = 1;// getChunkType(contStr);
                QColor textColor = Qt::black;
                if (!chunkType) textColor = Qt::darkRed;
                QRectF normRect(currentX, currentY, textWidth(normStr), fm.height());
                //painter->setPen(QPen(Qt::NoPen)); painter->setBrush(QColor(255, 127, 255));
                //painter->drawRect(normRect);
                painter->setPen(QPen(textColor, 1, Qt::SolidLine)); painter->setFont(getFont());
                //painter->drawText(normRect, normStr);
                qreal textVPos = baseline() - fm.height()*0.5 + fm.ascent();
                painter->drawText(QPointF(currentX, textVPos), normStr);
                currentX += textWidth(normStr);
                
                QRectF subRect(currentX, currentY+fm.height()*0.5, subtextWidth(subStr), subFm.height());
                //painter->setPen(QPen(Qt::NoPen)); painter->setBrush(QColor(255, 127, 255));
                //painter->drawRect(subRect);
                painter->setPen(QPen(textColor, 1, Qt::SolidLine)); painter->setFont(getSubFont());
                //painter->drawText(subRect, subStr);
                painter->drawText(QPointF(currentX, textVPos + fm.descent()), subStr);
                currentX += subtextWidth(subStr);
            }
            if (t == isChild) {
                MathEdit *child = m_contItems[childNum]; childNum++;
                if (child->type() == MEOperator && child->getContent() == QString(cdotChar))
                    currentX += child->boundingRect().width()+0.0*m_paddingH;
                else
                    currentX += child->boundingRect().width()+1.0*m_paddingH;
            }
        }
    }
    if (!m_result.isEmpty()) {
        if (!((m_result=="UNDEFINED" && (hasFocus() || hasDescendantFocus() )) || isUsedAsUnit())) {
            painter->setPen(QPen(Qt::black, 1, Qt::SolidLine)); painter->setFont(getFont());
            QRectF resultRect(currentX, currentY, textWidth(m_result), fm.height());
            qreal textVPos = baseline() - fm.height()*0.5 + fm.ascent();
            painter->drawText(QPointF(currentX, textVPos), m_result);
            currentX += textWidth(m_result);
        }
        if (m_unit) {
            currentX += m_unit->boundingRect().width()+2.0*m_paddingH;
        }
    }
    
    cursorPosUpdate();
    if (m_cursorVisible) { // && i == m_cursorPos) {
        painter->setPen(QPen(Qt::black, 1));
        painter->drawLine(QPointF(m_cursorX+1, m_cursorY), QPointF(m_cursorX+1, m_cursorH));
        painter->setPen(QPen());
    }
    
}
void MathEdit::refreshLayout() {
    updateBoundingRect();
}

void MathEdit::updateBoundingRect()
{
//qDebug() << "MathEdit::updateBoundingRect BEGIN: m_content = " << m_content;
    int isString = 0, isChild = 1;
    
    QFontMetricsF fm(m_font), subFm(m_subfont);
    m_centerHeight = baseline();
    
    QList<int> typeOfItem; // List of which chunks are occuring in m_content
    QChar lastItem = '#';
    for (QChar item: m_content) {
        if(item == '#') typeOfItem.append(isChild);
        if(lastItem == '#' && item != '#') typeOfItem.append(isString);
        lastItem = item;
    }
    
    int32_t childNum = 0, strNum = 0;
    QStringList contList = m_content.split('#', Qt::SkipEmptyParts);
    
    qreal w = 0.0, minY = -fm.height()*0.5, maxY = fm.height()*0.5; //minY = -fm.ascent(), maxY = fm.descent();
    bool hasSubStr = false;
    MathEdit *beforeItem = nullptr;
    if(!m_content.isEmpty()) {
        for(int32_t t: typeOfItem) {
            if (t == isString) {
                QString contStr = contList[strNum]; strNum++;
                QString normStr = contStr.section('_', 0, 0);
                QString subStr = contStr.section('_', 1, -1);
                if(!subStr.isEmpty()) hasSubStr = true;
                w += textWidth(normStr) + subtextWidth(subStr);
                QRectF normRect(w, 0.0, textWidth(normStr), fm.height());
                QRectF subRect(w + textWidth(normStr), fm.height()*0.5, subtextWidth(subStr), subFm.height());
            }
            if (t == isChild && m_contItems.length()>childNum) {
                MathEdit *child = m_contItems[childNum]; childNum++;
                //child->updateBoundingRect();
                qreal childY = baseline() - child->baseline() + child->verticalOffset(beforeItem);
                if (child->type() == MEOperator && child->getContent() == QString(cdotChar)) {
                    child->setPos(w, childY);
                    w += child->boundingRect().width()+0.0*m_paddingH;
                } else {
                    child->setPos(w + 1.0*m_paddingH, childY);
                    w += child->boundingRect().width()+1.0*m_paddingH;
                }
                qreal childMinY = child->pos().y() + child->boundingRect().top();
                qreal childMaxY = child->pos().y() + child->boundingRect().bottom();
                minY = qMin(minY, childMinY);
                maxY = qMax(maxY, childMaxY);
                beforeItem = child;
            }
        }
    }
    if (!m_result.isEmpty()) {
        if (!(m_result=="UNDEFINED" && (hasFocus() || (getParentPageMathItem() && getParentPageMathItem()->rootMathLeaf()->hasDescendantFocus()) ))) {
            w += textWidth(m_result);
        }
        if (m_unit) {
            qreal unitY = baseline() - m_unit->baseline() + m_unit->verticalOffset(beforeItem);
            m_unit->setPos(w, unitY);
            w += m_unit->boundingRect().width()+1.0*m_paddingH;
            qreal unitMinY = m_unit->pos().y() + m_unit->boundingRect().top();
            qreal unitMaxY = m_unit->pos().y() + m_unit->boundingRect().bottom();
            minY = qMin(minY, unitMinY);
            maxY = qMax(maxY, unitMaxY);
        }
    }
    
    qreal h = fm.ascent() + fm.descent();
    if (hasSubStr) h = fm.ascent() + fm.descent() + subFm.descent();
    if (h < maxY-minY) h = maxY-minY;
    if (w < 0.25*fm.height()) w = 0.25*fm.height();
    QRectF rect(0.0, minY, w, h);
    prepareGeometryChange();
    m_boundingRectangle = rect;
    notifyParentSizeChange();
}

void MathEdit::notifyParentSizeChange() {
    if(parentObject() != m_parentPageMathItem) { // do this if parent is not the PageMathItem
        MathEdit* parentEdit = qobject_cast<MathEdit*>(parentObject());
        if (parentEdit) {
            parentEdit->updateBoundingRect();
        }
    }
}

void MathEdit::updateBoundingRectForWholeTree() {
    for(MathEdit *child: m_contItems) {
        child->updateBoundingRectForWholeTree();
    }
    updateBoundingRect();
}

qreal MathEdit::baseline() const {
    return 0.0;
    //QFontMetricsF fm(m_font);
    //return fm.ascent();
}

qreal MathEdit::verticalOffset(MathEdit *referenceItem) const {
    Q_UNUSED(referenceItem);
    
    QFontMetricsF fm(getFont());
    return -0.0 * fm.ascent();
}

QJsonObject MathEdit::toJson() const {
    QJsonObject object;
    object["type"] = type();
    object["content"] = m_content;
    object["mathFontSize"] = static_cast<int>(m_mathFontSize);
    if (m_unit) {
        object["unit"] = m_unit->toJson();
    }
    
    QJsonArray childItemsArray;
    for(MathEdit *child: m_contItems) {
        childItemsArray.append(child->toJson());
    }
    object["childItems"] = childItemsArray;
    
    
    return object;
}

void MathEdit::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathEdit::MEBase) {
        m_content = object["content"].toString();
        m_mathFontSize = object["mathFontSize"].toInt();
    } else {
        qDebug() << "MathLeafEdit::fromJson: Error on loading object!";
    }
}

void MathEdit::cleanContent() {
    for(MathEdit *child: m_contItems) {
        child->cleanContent();
    }
    qDeleteAll(m_contItems);
    m_contItems.clear();
    m_content.clear();
    m_cursorPos = 0;
}

bool MathEdit::hasDescendantFocus() const {
    if (hasFocus()) {
        return true;
    }
    for (MathEdit *c: m_contItems) {
        if (c->hasDescendantFocus()) return true;
    }
    return false;
}

bool MathEdit::isEmpty() const { // if this returns "true" the object is deleted on loosing focus
    return false;
}

MathVariable MathEdit::getValue() {
    return m_value;
}
void MathEdit::setMathWarning(QString s) {
    if (getParentPageMathItem())
        getParentPageMathItem()->addWarningMessage(s);
}

void MathEdit::compute(const MathVariable& boolMask) {
    // Special characters:
    QString smallerEqual = QString(smallerEqualChar);
    QString greaterEqual = QString(greaterEqualChar);
    QString longEqual    = QString(longEqualChar);
    QString notEqual     = QString(notEqualChar);
    
    m_result = QString("");
    
    QList<MathEdit*> tempMEList;
    
    try {
        m_mathError = m_mathWarning = QString();
        
        // make sure we don't have any loose inputs
        processAllContent();
        
        // let's see if this is the rootMathLeaf()
        bool isRoot(this == m_parentPageMathItem->rootMathLeaf());
        bool isValidRoot = false, isAssign = false, isResult = false; Q_UNUSED(isValidRoot);
        int32_t VariableID = -1; Q_UNUSED(VariableID);
        QList<int32_t> AssignIDs;
        QList<int32_t> ResultIDs;
        QList<MathEdit*> mathExpression;
        
        if (isRoot && !m_isUsedAsUnit) {
            // this is only allowed to be a new variable name, and a MathLeafEdit
            // containing the ":=" function, or an existing variable name and a MathLeafEdit
            // containing the "=" function.
            if (getContItems().length() >= 2) {
                if ((getContItems().at(0)->type() == MEVariable) &&
                    (getContItems().at(1)->type() == MEOperator) &&
                    (getContItems().at(1)->getContent() == ":=" ||
                     getContItems().at(1)->getContent() == "=")) {
                    isValidRoot = true;
                    VariableID = 0;
                    for (int32_t i = 1; i < getContItems().length(); i++) {
                        if (getContItems().at(i)->getContent() == ":=") { AssignIDs.append(i); isAssign = true; }
                        if (getContItems().at(i)->getContent() == "=") { ResultIDs.append(i); isResult = true; }
                    }
                    if (AssignIDs.length() >= 1 && ResultIDs.length() == 0) { // ":=" operator only
                        mathExpression = m_contItems.mid(AssignIDs.last() + 1);
                    }
                    if (ResultIDs.length() >= 1 && AssignIDs.length() >= 1) { // ":=" and "=" operators
                        if (AssignIDs.last() < ResultIDs.first()) {
                            mathExpression = m_contItems.mid(AssignIDs.last() + 1, ResultIDs.first() - (AssignIDs.last() + 1));
                        }
                    }
                    if (ResultIDs.length() >= 1 && AssignIDs.length() == 0) {// "=" operator only
                        mathExpression = m_contItems.mid(0, ResultIDs.first());
                    }
                }
            } else if (isRoot && m_isUsedAsUnit) { // this can happen if it is used in a diagram axis title for the unit
                isAssign = false;
                isResult = false;
            } else {
                m_mathError = tr("Root object has to contain at least 2 objects:\n"
                "    * A variable name\n"
                "    * followed by a ':=' or '=' function object.");
                
                throw std::invalid_argument("Invalid root object");
            }
        } else {
            mathExpression = m_contItems;
        }
        
        // update the value of all child expressions
        for (MathEdit *m: mathExpression) {
            m->compute(boolMask);
        }
        
        
        // Evaluate the mathExpression:
        //=============================
        
        // 1) calculate all exponentials first:
        if (mathExpression.length()>=2) {
            for (int64_t i = mathExpression.length()-1; i >= 0; i--) {
                if (mathExpression.at(i)->type() == MEExponent) {
                    if (i > 0) {
                        if (mathExpression.at(i-1)->type() != MEOperator) {
                            MathEdit *tempME = MathEditCreateNew::newMathEdit(QString("|"), MEConstant, getData(), getParentPageMathItem(), this);
                            tempMEList.append(tempME);
                            tempME->setValue(power(mathExpression.at(i-1)->getValue(),
                                                   mathExpression.at(i)->getValue(), boolMask));
                            mathExpression.removeAt(i); mathExpression.removeAt(i-1);
                            mathExpression.insert(i-1, tempME);
                        }
                    }
                }
            }
        }
        
        // 2) calculate all multiplications second:
        if (mathExpression.length()>=3) {
            for (int64_t i = mathExpression.length()-2; i >= 0; i--) {
                if (mathExpression.at(i)->type() == MEOperator &&
                    mathExpression.at(i)->getContent() == QString(cdotChar)) {
                    if (i > 0 && i < mathExpression.length()-1) {
                        if (mathExpression.at(i-1)->type() != MEOperator && mathExpression.at(i+1)->type() != MEOperator) {
                            MathEdit *tempME = MathEditCreateNew::newMathEdit(QString(""), MEConstant, getData(), getParentPageMathItem(), this);
                            tempMEList.append(tempME);
                            tempME->setValue(mul(mathExpression.at(i-1)->getValue(),
                                                 mathExpression.at(i+1)->getValue(), boolMask));
                            mathExpression.removeAt(i+1); mathExpression.removeAt(i); mathExpression.removeAt(i-1);
                            mathExpression.insert(i-1, tempME);
                        }
                    }
                }
            }
        }
        
        // 3) calculate all additions and subtractions third:
        if (mathExpression.length()>=3) {
            for (int64_t i = mathExpression.length()-2; i >= 0; i--) {
                if (mathExpression.at(i)->type() == MEOperator &&
                    (mathExpression.at(i)->getContent() == "+" || mathExpression.at(i)->getContent() == "-")) {
                    if (i > 0 && i < mathExpression.length()-1) {
                        if (mathExpression.at(i-1)->type() != MEOperator && mathExpression.at(i+1)->type() != MEOperator) {
                            MathEdit *tempME = MathEditCreateNew::newMathEdit(QString(""), MEConstant, getData(), getParentPageMathItem(), this);
                            tempMEList.append(tempME);
                            if (mathExpression.at(i)->getContent() == "+")
                                tempME->setValue(add(mathExpression.at(i-1)->getValue(),
                                                     mathExpression.at(i+1)->getValue(), boolMask));
                            if (mathExpression.at(i)->getContent() == "-")
                                tempME->setValue(sub(mathExpression.at(i-1)->getValue(),
                                                     mathExpression.at(i+1)->getValue(), boolMask));
                            mathExpression.removeAt(i+1); mathExpression.removeAt(i); mathExpression.removeAt(i-1);
                            mathExpression.insert(i-1, tempME);
                        }
                    }
                }
            }
        }
        
        // 4) calculate all comparisons fourth:
        if (mathExpression.length()>=3) {
            for (int64_t i = mathExpression.length()-2; i >= 0; i--) {
                QString opChar = mathExpression.at(i)->getContent();
                if (mathExpression.at(i)->type() == MEOperator &&
                    (opChar == "<" || opChar == smallerEqual ||
                     opChar == ">" || opChar == greaterEqual ||
                     opChar == longEqual || opChar == notEqual)) {
                     if (i > 0 && i < mathExpression.length()-1) {
                        if (mathExpression.at(i-1)->type() != MEOperator && mathExpression.at(i+1)->type() != MEOperator) {
                            MathEdit *tempME = MathEditCreateNew::newMathEdit(QString(""), MEConstant, getData(), getParentPageMathItem(), this);
                            tempMEList.append(tempME);
                            MathVariable a = mathExpression.at(i-1)->getValue();
                            MathVariable b = mathExpression.at(i+1)->getValue();
                            if (opChar == "<")          tempME->setValue(smallerThen(a, b, boolMask));
                            if (opChar == smallerEqual) tempME->setValue(smallerEqualThen(a, b, boolMask));
                            if (opChar == ">")          tempME->setValue(greaterThen(a, b, boolMask));
                            if (opChar == greaterEqual) tempME->setValue(greaterEqualThen(a, b, boolMask));
                            if (opChar == longEqual)    tempME->setValue(equal(a, b, boolMask));
                            if (opChar == notEqual)     tempME->setValue(notEq(a, b, boolMask));
                            mathExpression.removeAt(i+1); mathExpression.removeAt(i); mathExpression.removeAt(i-1);
                            mathExpression.insert(i-1, tempME);
                        }
                    }
                }
            }
        }
        
        
        // 5) calculate all boolean NOT fifth:
        if (mathExpression.length()>=2) {
            for (int64_t i = mathExpression.length()-2; i >= 0; i--) {
                QString opChar = mathExpression.at(i)->getContent();
                if (mathExpression.at(i)->type() == MEOperator &&
                    (opChar == "!")) {
                    if (i >= 0 && i < mathExpression.length()-1) {
                        if (mathExpression.at(i+1)->type() != MEOperator) {
                            MathEdit *tempME = MathEditCreateNew::newMathEdit(QString(""), MEConstant, getData(), getParentPageMathItem(), this);
                            tempMEList.append(tempME);
                            MathVariable a = mathExpression.at(i+1)->getValue();
                            if (opChar == "!") tempME->setValue(boolNot(a, boolMask));
                            mathExpression.removeAt(i+1); mathExpression.removeAt(i);
                            mathExpression.insert(i, tempME);
                        }
                    }
                }
            }
        }
        
        // 6) calculate all boolean AND sixth:
        if (mathExpression.length()>=3) {
            for (int64_t i = mathExpression.length()-2; i >= 0; i--) {
                QString opChar = mathExpression.at(i)->getContent();
                if (mathExpression.at(i)->type() == MEOperator &&
                    (opChar == "&")) {
                    if (i > 0 && i < mathExpression.length()-1) {
                        if (mathExpression.at(i-1)->type() != MEOperator && mathExpression.at(i+1)->type() != MEOperator) {
                            MathEdit *tempME = MathEditCreateNew::newMathEdit(QString(""), MEConstant, getData(), getParentPageMathItem(), this);
                            tempMEList.append(tempME);
                            MathVariable a = mathExpression.at(i-1)->getValue();
                            MathVariable b = mathExpression.at(i+1)->getValue();
                            if (opChar == "&") tempME->setValue(boolAnd(a, b, boolMask));
                            mathExpression.removeAt(i+1); mathExpression.removeAt(i); mathExpression.removeAt(i-1);
                            mathExpression.insert(i-1, tempME);
                        }
                    }
                }
            }
        }
        
        // 7) calculate all boolean OR seventh:
        if (mathExpression.length()>=3) {
            for (int64_t i = mathExpression.length()-2; i >= 0; i--) {
                QString opChar = mathExpression.at(i)->getContent();
                if (mathExpression.at(i)->type() == MEOperator &&
                    (opChar == "|")) {
                    if (i > 0 && i < mathExpression.length()-1) {
                        if (mathExpression.at(i-1)->type() != MEOperator && mathExpression.at(i+1)->type() != MEOperator) {
                            MathEdit *tempME = MathEditCreateNew::newMathEdit(QString(""), MEConstant, getData(), getParentPageMathItem(), this);
                            tempMEList.append(tempME);
                            MathVariable a = mathExpression.at(i-1)->getValue();
                            MathVariable b = mathExpression.at(i+1)->getValue();
                            if (opChar == "|") tempME->setValue(boolOr(a, b, boolMask));
                            mathExpression.removeAt(i+1); mathExpression.removeAt(i); mathExpression.removeAt(i-1);
                            mathExpression.insert(i-1, tempME);
                        }
                    }
                }
            }
        }
        
        //qDebug() << "MathEdit::compute: Lenght of expression after compute (1 expected): " << mathExpression.length();
        
        if (mathExpression.length() == 1) {
            MathVariable v = mathExpression.at(0)->getValue();
            if (v.isValidValue()) {
                setValue(v);
            } else if (!v.isValidValue() && m_isUsedAsUnit) {
                m_result = "";
            } else {
                setMathError(tr("ERROR: Undefined Variable"));
                setValue(MathVariable(QList<qreal> {std::numeric_limits<qreal>::max()}));
                getValue().clear();
                m_result = "UNDEFINED";
            }
        } else if (isUsedAsUnit() && mathExpression.length() == 0) {
            setValue(MathVariable(QList<qreal> {1.0}));
        } else if (m_content.isEmpty()) {
            setValue(MathVariable());
        } else {
            setMathError(tr("ERROR: Malformed mathematical expression"));
            setValue(MathVariable(QList<qreal> {std::numeric_limits<qreal>::max()}));
            getValue().clear();
            m_result = "UNDEFINED";
        }
        
                        //                           aIDs: -------0----2----4
                        //                           aIDs: -------v----v----v
        if (isAssign) { // could be a series of assignments like [a := b := c := 55]
            for (int64_t i = AssignIDs.length()-1; i>=0; i--) {
                int64_t aID = AssignIDs.at(i);
                if (getContItems().at(aID-1)->type() == MEVariable) {
                    getContItems().at(aID-1)->setValue(getValue());
                    // register the variable name with m_data:
                    if (getData()->contains(getContItems().at(aID-1)->getContent())) {
                        setMathWarning(tr("WARNING: Redefinition of existing variable '%1'.").arg(getContItems().at(aID-1)->getContent()));
                        qDebug() << tr("WARNING: Redefinition of existing variable '%1'.").arg(getContItems().at(aID-1)->getContent());
                    }
                    getData()->setValue(getContItems().at(aID-1)->getContent(), getValue());
                }
            }
        }
        
        if (isResult) {
            if (!m_unit) m_unit = new MathEdit(getData(), getParentPageMathItem(), this);
            m_unit->setIsUsedAsUnit(true);
            
            if (!(getValue().isEmpty() || m_result == "UNDEFINED")) {
                m_result = resultString(true);
            } else {
                m_mathError = tr("ERROR: Undefined variable '%1'.").arg(getContent());
                m_value.clear();
                m_result = "UNDEFINED";
            }
        } else {
            if (m_unit) {
                m_unit->deleteLater();
                m_unit = nullptr;
            }
        }
    }
    
    
    catch (const std::invalid_argument &e) {
        m_value.setErrorValue(std::numeric_limits<qreal>::max());
        qDebug() << "Caught exception:" << e.what();
        m_mathError = QString("ERROR: ") + QString(e.what());
        
    }
    catch (const QException &e) {
        m_value.setErrorValue(std::numeric_limits<qreal>::max());
        qDebug() << "Caught exception:" << e.what();
        
    }
    catch (const std::exception& e) {
        m_value.setErrorValue(std::numeric_limits<qreal>::max());
        qDebug() << "Caught exception:" << e.what();
        m_mathError = QString("ERROR: ") + QString(e.what());
        
    }
    
    // delete all temporary objects
    for (MathEdit *&m: tempMEList) {
        delete m; m = nullptr;
    }
    tempMEList.clear();
    
    // If any error or warning messages occured, let the PageMathItem know about it
    if (!m_mathError.isEmpty()) {
        m_parentPageMathItem->addErrorMessage(m_mathError);
    }
    if (!m_mathWarning.isEmpty()) {
        m_parentPageMathItem->addWarningMessage(m_mathWarning);
    }
    
    
    prepareGeometryChange();
    updateBoundingRect();
    notifyParentSizeChange();
    
}

void MathEdit::setCursorRightOf(MathEdit* child) {
    int32_t i = 0;
    for (MathEdit* ch: m_contItems) {
        if (ch == child) break;
        i++;
    }
    int32_t j = 0, k = 0;
    for (QChar c: m_content) {
        k++;
        if (c == '#') {
            if (i == j) break;
            j++;
        }
    }
    m_cursorPos = k;
}

void MathEdit::setCursorLeftOf(MathEdit* child) {
    int32_t i = 0;
    for (MathEdit* ch: m_contItems) {
        if (ch == child) break;
        i++;
    }
    int32_t j = 0, k = 0;
    for (QChar c: m_content) {
        if (c == '#') {
            if (i == j) break;
            j++;
        }
        k++;
    }
    m_cursorPos = k;
}

MathEdit* MathEdit::getChildAtPos(int64_t pos){
    int64_t id = getChildIndexAtPos(pos);
    if (id >=0) return m_contItems[id];
    else return nullptr;
}

int64_t MathEdit::getChildIndexAtPos(int64_t pos){ //abc#def#
    int64_t count = -1, i = 0;
    for (QChar c: m_content) {
        if (c == '#') {
            count++;
        }
        if (i >= pos) { break; }
        i++;
    }
    if (m_contItems.length() > 0) return count;
    else return -1;
}

std::pair<int64_t, int64_t> MathEdit::getChildRangeIDsAt(int64_t begin, int64_t length) { // begin, end like when selected
    int64_t idBegin, len;
    
    QString frontStr = m_content.mid(0,begin);
    QString selStr = m_content.mid(begin, length);
    
    idBegin = frontStr.count('#');
    len = selStr.count('#');
    
    return {idBegin, len};
}

void MathEdit::focusInEvent(QFocusEvent* event) {
    //m_parentPageMathItem->printStructure();
    m_cursorTimer.start(500);
    m_cursorVisible = true;
    //update(m_boundingRectangle);
    //m_parentPageMathItem->rootMathLeaf()->updateBoundingRectForWholeTree();
    emit gainedFocus();
    QGraphicsObject::focusInEvent(event);
}

void MathEdit::focusOutEvent(QFocusEvent* event) {
    bool processOK = processAllContent();
    if (!processOK) std::cout << "MathEdit::focusOutEvent: processing content failed! m_content = "
                              << m_content.toStdString() << std::endl;
    
    m_cursorTimer.stop();
    m_cursorVisible = false;
    m_selectBegin = m_selectEnd = m_selectAnchor = -1;
    //update(m_boundingRectangle);
    //m_parentPageMathItem->rootMathLeaf()->updateBoundingRectForWholeTree();
    emit lostFocus();
    QGraphicsObject::focusOutEvent(event);
}

void MathEdit::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void MathEdit::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void MathEdit::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
}

void MathEdit::keyPressEvent(QKeyEvent* event)
{
    const bool altPressed = event->modifiers() & Qt::AltModifier;
    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    const bool ctrlPressed = event->modifiers() & Qt::ControlModifier;
    
    QString charToInsert = event->text();
    int eventKey = event->key();
    
    // Special characters:
    QString smallerEqual = QString(smallerEqualChar);
    QString greaterEqual = QString(greaterEqualChar);
    QString longEqual    = QString(longEqualChar);
    QString notEqual     = QString(notEqualChar);
    if (eventKey == Qt::Key_AsciiCircum || eventKey == Qt::Key_Dead_Circumflex) charToInsert = "^";
    if (altPressed && charToInsert=="<") charToInsert = smallerEqual;
    if (altPressed && charToInsert==">") charToInsert = greaterEqual;
    if (altPressed && charToInsert=="=") charToInsert = longEqual;
    if (ctrlPressed && charToInsert=="=") charToInsert = notEqual;
    if (charToInsert=="*") charToInsert = QString(cdotChar);
    
    
    if (charToInsert.length() == 1 && !isCommandKey(eventKey)) { // to prevent ' ' chars to be added when the shift key is pressed for capital letters
        if (charToInsert=="#") {
            MathEdit *cI = MathEditCreateNew::newMathEdit("", MEConstant, m_data, m_parentPageMathItem, this);
            insertItem(cI);
            insertText(charToInsert);
        } else if ( charToInsert == "+" ||
                    charToInsert == cdotChar ||
                    charToInsert == ":" || charToInsert == "=" ||
                    charToInsert == "<" || charToInsert == smallerEqual ||
                    charToInsert == ">" || charToInsert == greaterEqual ||
                    charToInsert == longEqual || charToInsert == notEqual ||
                    charToInsert == "!" || charToInsert == "&" || charToInsert == "|" ) {
            if (m_cursorPos > 0 && charToInsert != "!") { // Cannot have a mathematical expression start with an operator
                bool isRoot = this == m_parentPageMathItem->rootMathLeaf();
                if (charToInsert == ":") charToInsert = ":="; // replace : with :=
                if ((charToInsert == ":=" || charToInsert == "=") && !isRoot) {
                    qWarning() << "The ':=' or '=' operator is only allowed in root object";
                } else {
                    MathEdit *child = getChildBeforeCursorPos(); // see what type the previous child is
                    bool childBeforeIsOperator = false;
                    if (child) if(child->type()==MEOperator) childBeforeIsOperator = true;
                    if (!childBeforeIsOperator || m_content.at(m_cursorPos-1) != '#') {
                        if (m_content.at(m_cursorPos-1) == '#') { // there is an item directly before, insert the operator
                            createNewItem(charToInsert, MEOperator);
                        } else { // there is text before, treat it as a variable and then insert the operator
                            createNewVariableAndItem(charToInsert, MEOperator);
                        }
                        emit getParentPageMathItem()->itemDataChanged();
                    }
                }
            } else if (m_cursorPos > -1 && charToInsert == "!") {
                if (m_cursorPos > 0) {
                    if (m_content.at(m_cursorPos-1) != '#') {
                        createNewVariableAndItem(charToInsert, MEOperator);
                    } else {
                        createNewItem(charToInsert, MEOperator);
                    }
                } else {
                    createNewItem(charToInsert, MEOperator);
                }
                emit getParentPageMathItem()->itemDataChanged();
            }
        } else if (charToInsert == "-") {
            if (cursorIsDirectlyAfterOperatorOrAtBegin(m_cursorPos)) { // The "-" will be treated as sign, not as operator
                insertText(charToInsert);
            } else if (cursorIsDirectlyAfterOther(m_cursorPos)) { // Now the "-" is an operator
                createNewItem(charToInsert, MEOperator);
            } else { // there is already some text before the minus, make that text a variable and the "-" an operator
                createNewVariableAndItem(charToInsert, MEOperator);
            }
            emit getParentPageMathItem()->itemDataChanged();
        } else if (charToInsert == "^") {
            if (!cursorIsDirectlyAfterOperatorOrAtBegin(m_cursorPos)) { // Cannot have a mathematical expression start with an operator
                MathEdit *newChild;
                if (m_content.at(m_cursorPos-1) == '#') { // there is an item directly before, insert the operator
                    newChild = createNewItem(charToInsert, MEExponent);
                } else { // there is text before, treat it as a variable and then insert the operator
                    newChild = createNewVariableAndItem(charToInsert, MEExponent);
                }
                newChild->cleanContent();
                newChild->setFocus();
                newChild->setCursorToEnd();
                newChild->updateBoundingRect();
                emit getParentPageMathItem()->itemDataChanged();
            }
        } else if (charToInsert == "/") {
            if (!cursorIsDirectlyAfterOperatorOrAtBegin(m_cursorPos) ||
               (m_selectEnd >= 0 && m_selectEnd != m_selectBegin)) { // Cannot have a mathematical expression start with an operator
                if (m_selectBegin < 0) { // nothing is selected, select the last chunk or item
                    MathEdit *childBefore = getChildBeforeCursorPos();
                    int32_t childType = 0;
                    if (childBefore) childType = getChildBeforeCursorPos()->type();
                    if (childType == MEExponent) { //select Not only the exponent but also the base
                        m_selectBegin = m_cursorPos - 2;
                    } else if (m_content.at(m_cursorPos-1) == '#') { 
                        m_selectBegin = m_cursorPos - 1;
                    } else {
                        m_selectBegin = getChunkStart(m_cursorPos);
                    }
                    if ( m_selectBegin < 0) m_selectBegin = 0;
                    m_selectEnd = m_cursorPos;
                    m_selectAnchor = m_cursorPos;
                }
                if (m_selectBegin >= 0) m_cursorPos = m_selectEnd; // set the cursor to end of selection for consistancy
                // get everything inside the selection to be an item
                bool processOK = processContent(m_selectBegin, m_selectEnd);
                Q_UNUSED(processOK);
                
                MathEdit *newChild;
                if (m_selectBegin >= 0) { //There is a selection active, put that into the numerator
                    QString selStr = m_content.mid(m_selectBegin, m_selectEnd - m_selectBegin);
                    auto[listBegin, listLen] = getChildRangeIDsAt(m_selectBegin, m_selectEnd - m_selectBegin);
                    QList<MathEdit*> selList = m_contItems.mid(listBegin, listLen);
                    m_content.remove(m_selectBegin, m_selectEnd - m_selectBegin);
                    m_contItems.remove(listBegin, listLen);
                    setCursorTo(getCursorPos() -1);
                    newChild = createNewItem("", MEFraction);
                    newChild->insertTextAt(0,selStr);
                    newChild->insertItemsAt(0,selList);
                    newChild->setFocusToChild2(); // set focus to denominator
                    newChild->setCursorToEnd();
                    newChild->updateBoundingRect();
                    emit getParentPageMathItem()->itemDataChanged();
                }
                
            }
        } else if (charToInsert == "(") { // could ba a parenthesis or a function or root or log
            insertText(charToInsert);
            
            QString chunk = getCurrentChunk();
            int64_t chunkStart = getChunkStart(m_cursorPos);
            int64_t chunkEnd = chunkStart + chunk.length();
            int chunkType = static_cast<int>(getChunkType(chunk));
            if (chunkType == MEFunction || chunkType == MERoot || chunkType == MELog) {
                int64_t startDel =0, endDel = 0; bool overlap = false;
                MathEdit *cI = MathEditCreateNew::newMathEdit(chunk, chunkType, m_data, m_parentPageMathItem, this);
                if (m_selectBegin >= 0 && m_selectBegin != m_selectEnd) {
                    m_selectBegin++; m_selectEnd++; m_selectAnchor++;
                    QString selStr = m_content.mid(m_selectBegin, m_selectEnd - m_selectBegin);
                    auto[listBegin, listLen] = getChildRangeIDsAt(m_selectBegin, m_selectEnd - m_selectBegin);
                    QList<MathEdit*> selList = m_contItems.mid(listBegin, listLen);
                    cI->insertTextAt(0,selStr);
                    cI->insertItemsAt(0,selList);
                    m_contItems.remove(listBegin, listLen);
                    // make sure that chunk and select do not overlap:
                    startDel = qMin(m_selectBegin, chunkStart);
                    endDel = qMax(m_selectEnd, chunkEnd);
                    if (m_selectEnd < chunkStart || chunkEnd < m_selectBegin) overlap = false; else overlap = true;
                } else { m_selectBegin = m_selectEnd = m_selectAnchor = -1; }
                insertItem(cI);
                if (overlap) {
                    m_content.remove(startDel, endDel - startDel);
                } else {
                    m_content.remove(chunkStart, chunk.length());
                    m_content.remove(m_selectBegin, m_selectEnd - m_selectBegin);
                }
                m_cursorPos = chunkStart;
                insertText("#");
                m_selectBegin = m_selectEnd = m_selectAnchor = -1;
                
                cI->setFocusToChild2();
                cI->setFocusToChild1();
                cI->setCursorToEnd();
                cI->updateBoundingRect();
                emit getParentPageMathItem()->itemDataChanged();
            }
        } else if (charToInsert == "{") { // Conditional function
            QString chunk = getCurrentChunk();
            int64_t chunkStart = getChunkStart(m_cursorPos);
            int chunkType = static_cast<int>(getChunkType(chunk));
            if (chunkType) {
                MathEdit *cI = MathEditCreateNew::newMathEdit(chunk, chunkType, m_data, m_parentPageMathItem, this);
                insertItem(cI);
                m_content.remove(chunkStart, chunk.length());
                m_cursorPos = chunkStart;
                insertText("#");
                cI->setFocus();
                cI->setCursorToEnd();
                cI->updateBoundingRect();
            }
            MathEdit *cO = MathEditCreateNew::newMathEdit(QString(""), MEConditionalFunc, m_data, m_parentPageMathItem, this);
            cO->initialize();
            insertItem(cO);
            insertText("#");
            cO->setFocusToChild1();
            cO->setCursorToEnd();
            cO->updateBoundingRect();
            emit getParentPageMathItem()->itemDataChanged();
            
        } else if (allowedCharacters.contains(charToInsert)) {
            insertText(charToInsert);
            QString chunk = getCurrentChunk();
            int64_t chunkStart = getChunkStart(m_cursorPos);
            int chunkType = static_cast<int>(getChunkType(chunk));
            if (chunkType) {
                MathEdit *cI = MathEditCreateNew::newMathEdit(chunk, chunkType, m_data, m_parentPageMathItem, this);
                
                insertItem(cI);
                m_content.remove(chunkStart, chunk.length());
                m_cursorPos = chunkStart;
                insertText("#");
                cI->setFocus();
                cI->setCursorToEnd();
                cI->updateBoundingRect();
                emit getParentPageMathItem()->itemDataChanged();
            }
        }
    }
    // The inserting has been done, now determine if any action is needed
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
        if (par) {
            par->setCashedChild(this);
            par->keyPressEvent(event);
            par->setCashedChild(nullptr);
            emit getParentPageMathItem()->itemDataChanged();
        }
    } else {
        if (!altPressed) {
            QGraphicsObject::keyPressEvent(event);
        }
    }
    
    if (charToInsert == "=") getParentPageMathItem()->compute();
    if (m_unit) m_unit->setFocus();
    
    cursorPosUpdate();
    updateBoundingRect();
    update(boundingRect()); // triggers paint()
    event->accept();
}

bool MathEdit::shiftDel() {
    MathEdit *par = qobject_cast<MathEdit*>(parentObject());
    if (!par) { return false; }
    par->setCashedChild(this);
    par->shiftDel();
    return true;
}

QString MathEdit::getChunkAt(int64_t pos) {
    const QString s = m_content;
    if (pos < 0 || pos > s.size()) return QString();
    
    // abc#def
    const qsizetype separatorBefore = s.lastIndexOf('#', pos - 1);
    const qsizetype separatorAfter  = s.indexOf('#', pos);
    
    const qsizetype start = separatorBefore + 1;
    const qsizetype end   = (separatorAfter == -1)? s.size() : separatorAfter;
    
    return s.mid(start, end - start);
}

QString MathEdit::getCurrentChunk() {
    return getChunkAt(m_cursorPos);
}

int64_t MathEdit::getChunkStart(int64_t pos) {
    const QString s = m_content;
    if (pos < 0 || pos > s.size()) return -1;
    
    const int64_t start = s.lastIndexOf('#', pos - 1) + 1;
    return start;
}

bool MathEdit::processAllContent() {
    return processContent(0, m_content.length());
}

bool MathEdit::processContent(int64_t selBegin, int64_t selEnd) {
    // get everything inside the selection to be an item, start at the last position
    bool isOK = true, isSelectionToBeProcessed = false;
    
    // check if we have something selected that we are processing here
    if (selBegin == m_selectBegin && selEnd == m_selectEnd) isSelectionToBeProcessed = true;
    int64_t i = selEnd - 1; // set counter to end of selection
    while (i >= selBegin) {
        if (m_content.at(i) == '#') {
            --i;
            continue;
        }
        // here we found the end of a chunk
        int64_t chunkEnd = i+1;
        // search for begin of the chunk
        while (i > selBegin && m_content.at(i-1) != '#') --i;
        int64_t chunkBegin = i;
        
        QString chunk = m_content.mid(chunkBegin,chunkEnd-chunkBegin);
        int chunkTyp = static_cast<int>(getChunkType(chunk));
        if (!chunkTyp) chunkTyp = MEVariable;
        
        int64_t selB = selBegin, selE = selEnd, selAnc = m_selectAnchor;
        //MathEdit *cME = createNewItem(chunk, MEVariable);
        MathEdit *cME = MathEditCreateNew::newMathEdit(chunk, chunkTyp, m_data, m_parentPageMathItem, this);
        if (cME) {
            insertItem(cME);
            cME->updateBoundingRect();
            insertTextAt(chunkBegin,"#");
            m_content.remove(chunkBegin+1, chunk.length());
            setCursorTo(getCursorPos() - chunk.length() + 1);
        } else {
            isOK = false;
            return isOK;
        }
        selBegin = selB; selEnd = selE;
        if (isSelectionToBeProcessed) { m_selectBegin = selB; m_selectEnd = selE; m_selectAnchor = selAnc; }
        selEnd -= chunk.length() - 1;
        if (i <= 0) break;
    }
    
    // Check if we messed up the selection (if one was set)
    if (m_selectBegin >= m_selectEnd) m_selectBegin = m_selectEnd = m_selectAnchor = -1;
    
    return isOK;
}

void MathEdit::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if(hasFocus()) {
        event->accept();
        grabMouse(); // "this" now gets all move events until release
        m_cursorPos = getCursorIndexForPosition(event->pos().x());
        m_selectBegin = m_selectEnd = m_selectAnchor = m_cursorPos;
        update(m_boundingRectangle);
    } else {
        QGraphicsObject::mousePressEvent(event);
    }
}

void MathEdit::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton) {
        int64_t mousePos = getCursorIndexForPosition(event->pos().x());
        m_selectBegin = std::min(m_selectAnchor, mousePos);
        m_selectEnd   = std::max(m_selectAnchor, mousePos);
        event->accept();
        update(m_boundingRectangle);
    }
    QGraphicsObject::mouseMoveEvent(event);
}

void MathEdit::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (scene() && scene()->mouseGrabberItem() == this) {
        ungrabMouse();
    }
    if (m_selectBegin == m_selectEnd) m_selectBegin = m_selectEnd = m_selectAnchor = -1; // If nothing was marked unset the selection
    QGraphicsObject::mouseReleaseEvent(event);
}

void MathEdit::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    Q_UNUSED(event);
}


/*******************************************************
private:
********************************************************/

void MathEdit::processInitText(QString text) {
    m_content = text;
}

void MathEdit::insertText(const QString& inText) {
    insertTextAt(m_cursorPos, inText);
}

void MathEdit::insertTextAt(int64_t pos, const QString& inText) {
    QString text = inText;
    if (pos > m_content.length()) pos = m_content.length();
    if(pos>0){
        if (m_content.at(pos-1) == QChar('\\')) {
            m_content.remove(pos-1, 1);
            m_cursorPos--; pos--;
            QString str = getGreekCharacter(inText);
            text.replace(0, 1, getGreekCharacter(inText));
        }
    } 
    m_content.insert(pos, text);
    m_cursorPos += text.length();
}


void MathEdit::insertItem(MathEdit *id) {
    insertItemAt(m_cursorPos, id);
}

void MathEdit::insertItems(QList<MathEdit*> list) {
    insertItemsAt(m_cursorPos, list);
}

void MathEdit::insertItemAt(int64_t pos, MathEdit *id) {
    // Look for all positions that have a '#' character
    QList<int64_t> positions;
    int64_t index = m_content.indexOf('#', 0);
    while (index != -1) {
        positions.append(index);
        // Advance by 1 to find overlapping matches, or by searchTerm.length() for non-overlapping
        index = m_content.indexOf('#', index + 1);
    }
    
    if (positions.isEmpty()) {
        m_contItems.append(id);
    }
    else {
        int64_t count = 0;
        for(int64_t p: positions) {
            if (p < pos) count++;
            else break;
        }
        m_contItems.insert(count, id);
    }
    
    id->setParentItem(this);
}

void MathEdit::insertItemsAt(int64_t pos, QList<MathEdit*> list) {
    // Look for all positions that have a '#' character
    QList<int64_t> positions;
    int64_t index = m_content.indexOf('#', 0);
    while (index != -1) {
        positions.append(index);
        // Advance by 1 to find overlapping matches, or by searchTerm.length() for non-overlapping
        index = m_content.indexOf('#', index + 1);
    }
    
    if (positions.isEmpty()) {
        for (int i = 0; i < list.size(); ++i) {
            m_contItems.insert(i, list.at(i));
        }
    }
    else {
        int64_t count = 0;
        for(int64_t p: positions) {
            if (p < pos) count++;
            else break;
        }
        for (int i = 0; i < list.size(); ++i) {
            m_contItems.insert(count + i, list.at(i));
        }
    }
    
    for (MathEdit* item : m_contItems) {
        item->setParentItem(this);
    }
    
}

MathEdit* MathEdit::createNewItem(QString s, int type) {
    MathEdit *cI = MathEditCreateNew::newMathEdit(s, type, m_data, m_parentPageMathItem, this);
    insertItem(cI);
    insertText("#");
    cI->initialize();
    setFocus();
    setCursorRightOf(cI);
    return cI;
}

MathEdit* MathEdit::createNewVariableAndItem(QString s, int type) {
    QString chunk = getCurrentChunk();
    int64_t chunkStart = getChunkStart(m_cursorPos);
    
    MathEdit *cI = MathEditCreateNew::newMathEdit(chunk, MEVariable, m_data, m_parentPageMathItem, this);
    insertItem(cI);
    insertText("#");

    MathEdit *cO = MathEditCreateNew::newMathEdit(s, type, m_data, m_parentPageMathItem, this);
    insertItem(cO);
    insertText("#");
    
    m_content.remove(chunkStart, chunk.length());
    m_cursorPos = chunkStart;
    cI->initialize(); cO->initialize();;
    setFocus();
    setCursorRightOf(cO);
    return cO;
}

MathEdit* MathEdit::createNewVariable() {
    QString chunk = getCurrentChunk();
    int64_t chunkStart = getChunkStart(m_cursorPos);
    MathEdit *cI = MathEditCreateNew::newMathEdit(chunk, MEVariable, m_data, m_parentPageMathItem, this);
    insertItem(cI);
    m_content.remove(chunkStart, chunk.length());
    m_cursorPos = chunkStart;
    insertText("#");
    cI->initialize();
    setFocus();
    setCursorRightOf(cI);
    return cI;
}

bool MathEdit::childBeforeIsOperator(int64_t pos){
    bool b = false;
    if (pos > 0) {
        MathEdit *child = getChildAtPos(pos-1); // see what type the previous child is
        if (child)
            if(child->type()==MEOperator)
                b = true;
    }
    return b;
}

bool MathEdit::cursorIsDirectlyAfterOperatorOrAtBegin(int64_t pos){
    bool b = false;
    if (pos>0) {
        if(m_content.at(pos-1) == '#' && childBeforeIsOperator(pos)) {
            b = true;
        }
    } else if (pos==0){
        b = true;
    }
    return b;
}

bool MathEdit::cursorIsDirectlyAfterOther(int64_t pos){
    bool b = false;
    if (m_cursorPos>0) {
        if(m_content.at(m_cursorPos-1) == '#' && !childBeforeIsOperator(pos)) {
            b = true;
        }
    }
    return b;
}

void MathEdit::setMathFontSize(int64_t size) {
    if (size<7) m_mathFontSize = 7;
    else m_mathFontSize = size;
    
    QFont font(m_fontName, static_cast<int>(m_mathFontSize));
    m_font = font;
    QFont subfont(m_fontName, qRound(static_cast<qreal>(m_mathFontSize)*m_subScriptScale));
    m_subfont = subfont;
}

void MathEdit::insertFnString(QKeyEvent* event) {
    keyPressEvent(event);
}

QString MathEdit::resultString(bool units) {
    Q_UNUSED(units);
    
    QString resStr = "";
    // Check if we are in a "display result" MathEdit
    int64_t resOperatorID = -1;
    for (int64_t i = getContItems().length()-1; i>0; i--) {
        MathEdit *m = getContItems().at(i);
        if (m->type() == MEOperator && m->getContent() == "=") {
            resOperatorID = i;
            break;
        }
    }
    
    if (resOperatorID >= 0) {
        //m_unit->setValue(MathVariable(QList<qreal>{1.0}));
        m_unit->compute();
        QList<qreal> manualUnits = m_unit->getValue().unit();
        qreal manualMultiplicator = 1.0 / m_unit->getValue().first();
        MathVariable value = getValue();
        QList<qreal> vunits = value.unit();    // unit exponents
        QList<qreal> unresolvedUnits = {vunits[0] - manualUnits[0],
                                        vunits[1] - manualUnits[1],
                                        vunits[2] - manualUnits[2],
                                        vunits[3] - manualUnits[3],
                                        vunits[4] - manualUnits[4],
                                        vunits[5] - manualUnits[5],
                                        vunits[6] - manualUnits[6]
                                       };
        
        QString valStr = QString();
        if (value.size() == 1) {
            valStr = tr("%1").arg(value.at(0)*manualMultiplicator);
        } else if (value.size() > 1 && value.size() <= 4) {
            valStr = tr("[%1").arg(value.at(0)*manualMultiplicator);
            for (int i=1; i<value.size(); i++) {
                valStr += tr(", %1").arg(value.at(i)*manualMultiplicator);
            }
            valStr += tr("]");
        } else if (value.size() > 4) {
            valStr = tr("[%1, ").arg(value.at(0)*manualMultiplicator);
            valStr += tr("%1, ").arg(value.at(1)*manualMultiplicator);
            valStr += tr("%1, ").arg(value.at(2)*manualMultiplicator);
            valStr += tr("... , %1]").arg(value.last()*manualMultiplicator);
        }
                                       
        QList<QString> unitStrs = {"m", "kg", "s", "A", "K", "mol", "cd"};
        QString unitStr = "";
        bool withMul = false;
        for (int32_t i = 0; i<7; i++) {
            qreal uExp = unresolvedUnits.at(i);
            if (uExp != 0.0) {
                if (withMul) unitStr.append(cdotChar);
                unitStr.append(unitStrs.at(i));
                if (uExp != 1.0) unitStr.append(QString("^") + tr("%1").arg(uExp));
                withMul = true;
            }
        }
        if (unitStr == QString("")) resStr = valStr;
        else resStr = valStr + QString(" ") + unitStr;
    }
    
    
    
    return resStr;
}

void MathEdit::appendSIUnit(QString unitStr, qreal exponent, bool withMultiplication) {
    if (withMultiplication) {
        m_contItems.append(MathEditCreateNew::newMathEdit(QString(cdotChar), MEOperator, getData(), getParentPageMathItem(), this));
        m_content.append("#");
    }
    m_contItems.append(MathEditCreateNew::newMathEdit(unitStr, MEVariable, getData(), getParentPageMathItem(), this));
    m_contItems.append(MathEditCreateNew::newMathEdit(tr("%1").arg(exponent), MEExponent, getData(), getParentPageMathItem(), this));
    m_content.append("##");
}

void MathEdit::toggleCursor() {
    m_cursorVisible = !m_cursorVisible;
    QRectF cursorRect = getCursorRect(); // in item coordinates
    update(cursorRect);
//qDebug() << "m_content = " << m_content << "  ;  Type = " << typeStr(type());
    
}

void MathEdit::cursorPosUpdate() {
    QString strBeforeCursor(m_content); strBeforeCursor.truncate(m_cursorPos);
    QFontMetricsF fm(m_font), subFm(m_subfont);
    m_centerHeight = baseline();
    
    int isString = 0, isChild = 1;
       
    QList<int> typeOfItem; // List of which chunks are occuring in strBeforeCursor
    QChar lastItem = '#';
    for (QChar item: strBeforeCursor) {
        if(item == '#') typeOfItem.append(isChild);
        if(lastItem == '#' && item != '#') typeOfItem.append(isString);
        lastItem = item;
    }
    
    int64_t childNum = 0, strNum = 0;
    QStringList contList = strBeforeCursor.split('#', Qt::SkipEmptyParts);
    qreal w = 0.0;
    bool inSubStr = false;
    if (!typeOfItem.isEmpty()) {
        for(int t: typeOfItem) {
            if (t == isString) {
                QString contStr = contList[strNum]; strNum++;
                inSubStr = false;
                QString normStr = contStr.section('_', 0, 0);
                QString subStr = contStr.section('_', 1, -1);
                if(m_cursorPos>=contStr.indexOf('_') && contStr.indexOf('_')>0) inSubStr = true;
                w += textWidth(normStr) + subtextWidth(subStr);
            }
            if (t == isChild) {
                inSubStr = false;
                MathEdit *child = m_contItems[childNum]; childNum++;
                if (child->type() == MEOperator && child->getContent() == QString(cdotChar)) {
                    w += child->boundingRect().width()+0.0*m_paddingH;
                } else {
                    w += child->boundingRect().width()+1.0*m_paddingH;
                }
            }
        }
    }
    m_cursorX = w-1;
    m_cursorW = 2;
    if (!inSubStr) {
        m_cursorY = boundingRect().top();
        m_cursorH = boundingRect().bottom();
    }
    else {
        m_cursorY = boundingRect().top() +subFm.ascent() - fm.descent();
        m_cursorH = boundingRect().bottom() + subFm.descent();
    }
}

int64_t MathEdit::getCursorIndexForPosition(qreal x) {
    
    QFontMetricsF fm(m_font);
    QFontMetricsF subFm(m_subfont);
    
    for (int64_t i = 0; i < m_content.size(); ++i) {
//        if (m_content.at(i) == '#') { i++; if (i==m_content.size()) break; }
        QString strBeforeCursor(m_content); strBeforeCursor.truncate(i); // take only the part before position i
        
        int64_t isString = 0, isChild = 1;
        
        QList<int64_t> typeOfItem; // List of which chunks are occuring in strBeforeCursor
        QChar lastItem = '#';
        for (QChar item: strBeforeCursor) {
            if(item == '#') typeOfItem.append(isChild);
            if(lastItem == '#' && item != '#') typeOfItem.append(isString);
            lastItem = item;
        }
        
        int64_t childNum = 0, strNum = 0;
        QStringList contList = strBeforeCursor.split('#', Qt::SkipEmptyParts);
        qreal itemW1 = fm.horizontalAdvance(m_content.at(0));
        qreal itemW2 = fm.horizontalAdvance(m_content.at(0));
        qreal w = 0.0;
        bool inSubStr;
        if(!typeOfItem.isEmpty()) {
            for(int64_t t: typeOfItem) {
                if (t == isString) {
                    QString contStr = contList[strNum]; strNum++;
                    inSubStr = false;
                    QString normStr = contStr.section('_', 0, 0);
                    QString subStr = contStr.section('_', 1, -1);
                    if(m_cursorPos>=contStr.indexOf('_') && contStr.indexOf('_')>0) inSubStr = true;
                    w += textWidth(normStr) + subtextWidth(subStr);
                    if (inSubStr) itemW1 = subFm.horizontalAdvance(m_content.at(i-1));
                        else itemW1 = fm.horizontalAdvance(m_content.at(i-1));
                    if (inSubStr) itemW2 = subFm.horizontalAdvance(m_content.at(i));
                        else itemW2 = fm.horizontalAdvance(m_content.at(i));
                }
                if (t == isChild) {
                    inSubStr = false;
                    MathEdit *child = m_contItems[childNum]; childNum++;
                    if (child->type() == MEOperator && child->getContent() == QString(cdotChar)) {
                        w += child->boundingRect().width()+0.0*m_paddingH;
                    } else {
                        w += child->boundingRect().width()+1.0*m_paddingH;
                    }
                }
            }
        }
        
        // If the click is within this item's horizontal bounds, set cursor here
        if (m_content.at(i) == '_') {
            if (x >= w-0.6*itemW1 && x <= w) return i;
            if (x >= w && x <= w + 0.6*itemW2) return i+1;
        }
        else {
            if (x >= w-0.6*itemW1 && x <= w + 0.6*itemW2) return i;
        }
    }
    
    // If click is past all content, place cursor at the end
    return m_content.size();
}

qreal MathEdit::getPositionForIndex(int64_t id){
    QString strBeforeID(m_content); strBeforeID.truncate(id);
    QFontMetricsF fm(m_font), subFm(m_subfont);
    m_centerHeight = baseline();
    
    int64_t isString = 0, isChild = 1;
    
    QList<int64_t> typeOfItem; // List of which chunks are occuring in strBeforeID
    QChar lastItem = '#';
    for (QChar item: strBeforeID) {
        if(item == '#') typeOfItem.append(isChild);
        if(lastItem == '#' && item != '#') typeOfItem.append(isString);
        lastItem = item;
    }
    
    int64_t childNum = 0, strNum = 0;
    QStringList contList = strBeforeID.split('#', Qt::SkipEmptyParts);
    qreal pos = 0.0;
    if(!typeOfItem.isEmpty()) {
        for(int64_t t: typeOfItem) {
            if (t == isString) {
                QString contStr = contList[strNum]; strNum++;
                QString normStr = contStr.section('_', 0, 0);
                QString subStr = contStr.section('_', 1, -1);
                pos += textWidth(normStr) + subtextWidth(subStr);
            }
            if (t == isChild) {
                MathEdit *child = m_contItems[childNum]; childNum++;
                if (child->type() == MEOperator && child->getContent() == QString(cdotChar)) {
                    pos += child->boundingRect().width()+0.0*m_paddingH;
                } else {
                    pos += child->boundingRect().width()+1.0*m_paddingH;
                }
            }
        }
    }
    return pos;
}

void MathEdit::backspace() {
    if(m_cursorPos>0) {
        if (m_content.at(m_cursorPos-1) == '#') { // delete the child MathEdit at this position
            int64_t childIndex = getChildIndexBeforeCursorPos();
            MathEdit* child = m_contItems.at(childIndex);
            m_contItems.removeAt(childIndex);
            delete child;
        }
        m_cursorPos--;
        m_content.remove(m_cursorPos,1);
    }
}
void MathEdit::del() {
    if(m_cursorPos<m_content.length()) {
        if (m_content.at(m_cursorPos) == '#') { // delete the child MathEdit at this position
            int64_t childIndex = getChildIndexAtCursorPos();
            MathEdit* child = m_contItems.at(childIndex);
            m_contItems.removeAt(childIndex);
            delete child;
        }
        m_content.remove(m_cursorPos,1);
    }
}
void MathEdit::leftArrow() {
    m_leftArrowPressed = true;
    m_selectBegin = m_selectEnd = m_selectAnchor = -1;
    if(m_cursorPos>0) {
        if (m_content.at(m_cursorPos-1) == '#') {
            MathEdit *child = getChildBeforeCursorPos();
            if (child) {
                child->setFocus();
                child->setCursorToEnd();
            }
        }
        m_cursorPos -= 1;
    } else {
        if(parentObject() != m_parentPageMathItem) { // only step out of the MathEdit if there is a MathEdit parent
            MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
            pa->setCashedChild(this);
            pa->setFocus();
            pa->setCursorLeftOf(this);
            pa->setCashedChild(nullptr);
        }
    }
    m_leftArrowPressed = false;
}
void MathEdit::shiftLeftArrow() {
    processAllContent();
    if(m_selectBegin == -1 && m_selectEnd == -1 && m_selectAnchor == -1) 
        m_selectAnchor = m_cursorPos;
    if(m_cursorPos>0) m_cursorPos -= 1;
    m_selectBegin = std::min(m_selectAnchor, m_cursorPos);
    m_selectEnd   = std::max(m_selectAnchor, m_cursorPos);
    
}
void MathEdit::rightArrow() {
    m_rightArrowPressed = true;
    m_selectBegin = m_selectEnd = m_selectAnchor = -1;
    if(m_cursorPos<m_content.length()) {
        if (m_content.at(m_cursorPos) == '#') {
            MathEdit *child = getChildAtCursorPos();
            if (child) {
                child->setFocus();
                child->setCursorToBegin();
            }
        }
        m_cursorPos += 1;
    } else {
        if(parentObject() != m_parentPageMathItem) { // only step out of the MathEdit if there is a MathEdit parent
            MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
            pa->setCashedChild(this);
            pa->setFocus();
            pa->setCursorRightOf(this);
            pa->setCashedChild(nullptr);
        }
    }
    m_rightArrowPressed = false;
}
void MathEdit::shiftRightArrow() {
    processAllContent();
    if(m_selectBegin == -1 && m_selectEnd == -1 && m_selectAnchor == -1) 
        m_selectAnchor = m_cursorPos;
    if(m_cursorPos<m_content.length()) m_cursorPos += 1;
    m_selectBegin = std::min(m_selectAnchor, m_cursorPos);
    m_selectEnd   = std::max(m_selectAnchor, m_cursorPos);
    
}
void MathEdit::upArrow() {
    
}
void MathEdit::downArrow() {
    
}

bool MathEdit::isCommandKey(const int key) {
    bool ret = false;
    if (key == Qt::Key_Backspace) { ret = true; }
    else if (key == Qt::Key_Delete) { ret = true; }
    else if (key == Qt::Key_Left) { ret = true; }
    else if (key == Qt::Key_Right) { ret = true; }
    else if (key == Qt::Key_Up) { ret = true; }
    else if (key == Qt::Key_Down) { ret = true; }
    else if (key == Qt::Key_Enter) { ret = true; }
    else if (key == Qt::Key_Return) { ret = true; }
    return ret;
}

int MathEdit::getChunkType(QString chunk) {
    int couldBeOperator = 0;
    if (chunk == "-" || chunk == "+" ||
        chunk == cdotChar || chunk == "/" ||
        chunk == "^" ||
        chunk == "<" || chunk == QString(smallerEqualChar) ||
        chunk == ">" || chunk == QString(greaterEqualChar) ||
        chunk == QString(longEqualChar) || chunk == QString(notEqualChar))
    {
        couldBeOperator = 1;
    }
    
    int couldBeVariable = 0;
    QStringList validVariableStartCharacters;
    for (QChar c: QString("ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz")) {
        validVariableStartCharacters.append(QString(c));
        validVariableStartCharacters.append(QString("-") + QString(c));
    }
    for (QChar c: latinToGreek.values()) {
        validVariableStartCharacters.append(QString(c));
        validVariableStartCharacters.append(QString("-") + QString(c));
    }
    if (chunk.contains("("))
        couldBeVariable = 0;
    else if (validVariableStartCharacters.contains(QString(chunk.left(1))) ||
             validVariableStartCharacters.contains(chunk.left(2))) {
        couldBeVariable = 1;
    }
    
    int couldBeConstant = 0;
    QDoubleValidator dValidator;
    dValidator.setLocale(QLocale::C);
    dValidator.setNotation(QDoubleValidator::ScientificNotation);
    dValidator.setDecimals(-1);
    int pos = static_cast<int>(chunk.length());
    QValidator::State dState = dValidator.validate(chunk,pos);
//qDebug() << "string:" << chunk << "state:" << dState << "pos:" << pos;
    if (dState == QValidator::Acceptable || dState == QValidator::Intermediate) {
        couldBeConstant = 1;
    }
    
    int couldBeFunction = 0;
    QList<QString> fnList = getValidFunctions();
    fnList.removeAll("root(");
    fnList.removeAll("log(");
    QString tmpStr = chunk;
    if (tmpStr.startsWith('-')) tmpStr.remove(0, 1);
    for (QString fn: fnList) {
        if (fn.startsWith(chunk) || fn.startsWith(tmpStr)) {
            couldBeFunction = 1; break;
        }
    }
    
    int couldBeFraction = 0;
    if (chunk.contains('/')) couldBeFraction = 1;
    
    int couldBeExponent = 0;
    if (chunk.contains('^')) couldBeExponent = 1;
    
    int couldBeRoot = 0;
    QString rootStr = "root(";
    QString tmpString = chunk;
    if (tmpString.startsWith('-')) tmpString.remove(0, 1);
    if (rootStr.startsWith(chunk) || rootStr.startsWith(tmpString)) couldBeRoot = 1;
    
    int couldBeLog = 0;
    QString logStr = "log(";
    QString tempString = chunk;
    if (tempString.startsWith('-')) tempString.remove(0, 1);
    if (logStr.startsWith(chunk) || logStr.startsWith(tempString)) couldBeLog = 1;
    
    int sum = couldBeOperator + couldBeVariable + couldBeConstant + couldBeFunction +
              couldBeFraction + couldBeExponent + couldBeRoot + couldBeLog;
    
    int type = 0;
    if (sum == 1)
        type = couldBeOperator*MEOperator + couldBeVariable*MEVariable +
               couldBeConstant*MEConstant + couldBeFunction*MEFunction +
               couldBeFraction*MEFraction + couldBeExponent*MEExponent +
               couldBeRoot*MERoot + couldBeLog*MELog;
    
    return type;
}

void MathEdit::printContent() {
    qDebug() << "MathEdit::printContent: m_content = " << m_content;
}

qreal MathEdit::textWidth(const QString &text) {
    // 1. Create a layout for the text with the exact font you will draw with.
    QTextLayout layout(text, m_font);
    // 2. Tell Qt to perform the full layout (including kerning, ligatures, etc.).
    layout.beginLayout();
    QTextLine line = layout.createLine();   // single line – we do not support multiline yet
    line.setLineWidth(1e9);                // huge width so the line never wraps
    layout.endLayout();
    // 3. The natural width of the line is the visual width we need.
    return line.naturalTextWidth();         // already includes over‑hangs & kerning
}
qreal MathEdit::subtextWidth(const QString &text) {
    // 1. Create a layout for the text with the exact font you will draw with.
    QTextLayout layout(text, m_subfont);
    // 2. Tell Qt to perform the full layout (including kerning, ligatures, etc.).
    layout.beginLayout();
    QTextLine line = layout.createLine();   // single line – we do not support multiline yet
    line.setLineWidth(1e9);                // huge width so the line never wraps
    layout.endLayout();
    // 3. The natural width of the line is the visual width we need.
    return line.naturalTextWidth();         // already includes over‑hangs & kerning
}

QString MathEdit::typeStr(int type) {
    if (type == MEBase)     return QString("MEBase");
    if (type == MEOperator) return QString("MEOperator");
    if (type == MEVariable) return QString("MEVariable");
    if (type == MEConstant) return QString("MEConstant");
    if (type == MEFunction) return QString("MEFunction");
    if (type == MEFraction) return QString("MEFraction");
    if (type == MEExponent) return QString("MEExponent");
    if (type == MERoot)     return QString("MERoot");
    if (type == MELog)     return QString("MELog");
    return QString();
}

void MathEdit::printStructure() {
    qDebug() << "MathEdit:" << m_content;
    for (MathEdit* me: m_contItems) me->printStructure();
}

void MathEdit::printMathExpression(QList<MathEdit*> &MEList) {
    // Evaluate the mathExpression:
    QString mStr = "";
    for (MathEdit *m: MEList) {
        mStr.append(m->getContent());
    }
    qDebug() << "mathExpression = " << mStr;
}

void MathEdit::setFocus(Qt::FocusReason focusReason) {
    QGraphicsObject::setFocus(focusReason);
}

// Stubs for situations when focus is to be handed through to a child,
// will be overridden by derived classes like MathEditFraction, MathEditRoot,
// MathEditLog (functions that need two inputs):
void MathEdit::setFocusToChild1(Qt::FocusReason focusReason) {
    QGraphicsObject::setFocus(focusReason);
}
void MathEdit::setFocusToChild2(Qt::FocusReason focusReason) {
    QGraphicsObject::setFocus(focusReason);
}
