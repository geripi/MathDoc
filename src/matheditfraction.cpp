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

#include "matheditfraction.h"
#include "helpers.h"
#include "matheditfactory.h"

#include <QDebug>
#include <QRandomGenerator>

MathEditFraction::MathEditFraction(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEdit(d, parentPageMathItem, parent) {
    m_numerator = new MathEdit(d, parentPageMathItem, this);
    m_denominator = new MathEdit(d, parentPageMathItem, this);
}

MathEditFraction::MathEditFraction(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEditFraction(d, parentPageMathItem, parent) {
    if (text != "#")
        m_numerator->insertTextAt(0, text);
}

void MathEditFraction::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
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
        if(m_numerator->getContent().isEmpty() || m_denominator->getContent().isEmpty()) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 223, 223, 127));
            painter->drawRect(getBoundingRectangle());
        } else {
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(getBoundingRectangle());
        }
    }
    
    QColor textColor = Qt::black;
    painter->setPen(QPen(textColor, 1, Qt::SolidLine));
    painter->drawLine(0.0, 0.0, getBoundingRectangle().width(), 0.0);
    
}

void MathEditFraction::updateBoundingRect() {
    //m_numerator->updateBoundingRect();
    //m_denominator->updateBoundingRect();
    
    QRectF bRectNumer = m_numerator->getBoundingRectangle();
    QRectF bRectDenom = m_denominator->getBoundingRectangle();
    
    qreal numeratorY = -getPaddingV() - bRectNumer.bottom();
    qreal denominatorY = getPaddingV() - bRectDenom.top();
    
    qreal maxLength = qMax(bRectNumer.width(), bRectDenom.width()) + 2.0*getPaddingH();
    qreal numeratorX = 0.5*(maxLength - bRectNumer.width());
    qreal denominatorX = 0.5*(maxLength - bRectDenom.width());
    
    m_numerator->setPos(numeratorX, numeratorY);
    m_denominator->setPos(denominatorX, denominatorY);
    
    qreal top = qMin( numeratorY + bRectNumer.top(), denominatorY + bRectDenom.top());
    qreal bottom = qMax(numeratorY + bRectNumer.bottom(), denominatorY + bRectDenom.bottom());
    
    QRectF rect(0.0, top, maxLength, bottom - top);
    
    prepareGeometryChange();
    setBoundingRectangle(rect);
    
    notifyParentSizeChange();
}

qreal MathEditFraction::baseline() const {
    //return m_numerator->getBoundingRectangle().height() + getPaddingV();
    return 0.0;
}

void MathEditFraction::compute(const MathVariable& boolMask) {
    m_numerator->compute(boolMask);
    m_denominator->compute(boolMask);
    setValue(div(m_numerator->getValue(), m_denominator->getValue(), boolMask));
}

void MathEditFraction::insertTextAt(int32_t pos, const QString& inText) {
    m_numerator->insertTextAt(pos, inText);
}

void MathEditFraction::insertItemAt(int32_t pos, MathEdit *id) {
    m_numerator->insertItemAt(pos,id);
}

void MathEditFraction::insertItemsAt(int32_t pos, QList<MathEdit*> list) {
    m_numerator->insertItemsAt(pos,list);
}

void MathEditFraction::appendToNumerator(QString cont, QList<MathEdit*> meList) {
    m_numerator->setCursorToEnd();
    m_numerator->insertText(cont);
    for (MathEdit* item: meList) {
        m_numerator->addContItem(item);
    }
}

void MathEditFraction::appendToDenominator(QString cont, QList<MathEdit*> meList) {
    m_denominator->setCursorToEnd();
    m_denominator->insertText(cont);
    for (MathEdit* item: meList) {
        m_denominator->addContItem(item);
    }
}

void MathEditFraction::setCursorToBegin(){
    if (hasFocus()) {
        m_numerator->setFocus();
        m_numerator->setCursorToBegin();
    }
}

void MathEditFraction::setCursorToEnd(){
    if (hasFocus()) {
        m_denominator->setFocus();
        m_denominator->setCursorToEnd();
    }
}

void MathEditFraction::setFocus(Qt::FocusReason focusReason) {
    MathEdit *cashedChild = getCashedChild();
    if (cashedChild == m_denominator && cashedChild->getRightArrowPressed()) { // leave this towards the right
        rightArrow();
    } else if (cashedChild == m_denominator && cashedChild->getLeftArrowPressed()) { // leave this towards the right
        m_numerator->setFocus();
        m_numerator->setCursorToEnd();
    } else if (cashedChild == m_numerator && cashedChild->getRightArrowPressed()) {
        m_denominator->setFocus();
        m_denominator->setCursorToBegin();
    } else if (cashedChild == m_numerator && cashedChild->getLeftArrowPressed()) {
        leftArrow();
    } else if (!cashedChild) {
        MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
        if (pa->getRightArrowPressed()) m_numerator->setFocus();
        else if (pa->getLeftArrowPressed()) m_denominator->setFocus();
        
    } else {
        m_denominator->setFocus();
    }
    setCashedChild(nullptr);
}
void MathEditFraction::setFocusToChild1(Qt::FocusReason focusReason) {
    m_numerator->setFocus();
}
void MathEditFraction::setFocusToChild2(Qt::FocusReason focusReason) {
    m_denominator->setFocus();
}
bool MathEditFraction::hasDescendantFocus() const {
    if (hasFocus()) { return true; }
    if (m_numerator->hasDescendantFocus()) { return true; }
    if (m_denominator->hasDescendantFocus()) { return true; }
    
    return false;
}

QJsonObject MathEditFraction::toJson() const {
    QJsonObject object;
    object["type"] = type();
    object["content"] = m_content;
    object["numerator"] = m_numerator->toJson();
    object["denominator"] = m_denominator->toJson();
    object["mathFontSize"] = getMathFontSize();
    
    QJsonArray childItemsArray;
    for(MathEdit *child:getContItems()) {
        childItemsArray.append(child->toJson());
    }
    object["childItems"] = childItemsArray;
    
    return object;
}

void MathEditFraction::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathEdit::MEFraction) {
        m_content = object["content"].toString();
        setMathFontSize(object["mathFontSize"].toInt());
        
        const QJsonObject jsonNumerator = object["numerator"].toObject();
        const QJsonObject jsonDenominator = object["denominator"].toObject();
        
        MathEditFactory::createChildTree(jsonNumerator, getData(), getParentPageMathItem(), m_numerator);
        MathEditFactory::createChildTree(jsonDenominator, getData(), getParentPageMathItem(), m_denominator);
        
    } else {
        qDebug() << "MathLeafEdit::fromJson: Error on loading object!";
    }
}

void MathEditFraction::printStructure() {
    qDebug() << "MathEditFraction:" << m_content;
    for (MathEdit* me: getContItems()) me->printStructure();
    m_numerator->printStructure();
    m_denominator->printStructure();
}
