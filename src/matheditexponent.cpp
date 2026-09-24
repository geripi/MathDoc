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

#include "matheditexponent.h"
#include "helpers.h"

#include <QDebug>

MathEditExponent::MathEditExponent(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEdit(d, parentPageMathItem, parent) {
    setMathFontSize(qRound(static_cast<qreal>(getMathFontSize())*0.8333333333333333333333333333333333333333333333333333333333));
    
    QFont font(getFontName(), static_cast<int>(getMathFontSize()));
    setFont(font);
    QFont subfont(getFontName(), qRound(static_cast<qreal>(getMathFontSize())*getSubScriptScale()));
    setSubFont(subfont);
    
}
MathEditExponent::MathEditExponent(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEditExponent(d, parentPageMathItem, parent) {
    processInitText(text);
}


void MathEditExponent::updateBoundingRect() {
    MathEdit::updateBoundingRect();
}

qreal MathEditExponent::baseline() const {
    //return m_numerator->getBoundingRectangle().height() + getPaddingV();
    MathEdit* p = qobject_cast<MathEdit*>(parentObject());
    if (!p)
        return 0.0;
    
    QFontMetricsF fm(getFont());
    QFontMetricsF parFm(p->getFont());
    
    
    
    //return parFm.ascent() + fm.ascent();
    return 0.0;
}

qreal MathEditExponent::verticalOffset(MathEdit *referenceItem) const
{ // negative offset means it moves UP
    qreal offs = 0.0;
    MathEdit* p = qobject_cast<MathEdit*>(parentObject());
    if (!p)
        return - getBoundingRectangle().height();
    QFontMetricsF parFm(p->getFont());
    
    if (referenceItem) {
        offs = referenceItem->getBoundingRectangle().top()
               - getBoundingRectangle().bottom()
               + 0.4*parFm.height();
    } else {
        offs = 0.0 * parFm.height() - getBoundingRectangle().bottom();
    }
    
    return offs;
}

QJsonObject MathEditExponent::toJson() const
{
    QJsonObject object;
    object["type"] = type();
    object["content"] = m_content;
    object["mathFontSize"] = static_cast<int>(getMathFontSize());
    
    QJsonArray childItemsArray;
    for(MathEdit *child: getContItems()) {
        childItemsArray.append(child->toJson());
    }
    object["childItems"] = childItemsArray;
    
    return object;
}

void MathEditExponent::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathEdit::MEExponent) {
        m_content = object["content"].toString();
        setMathFontSize(object["mathFontSize"].toInt());
        
    } else {
        qDebug() << "MathLeafEdit::fromJson: Error on loading object!";
    }
}
