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

#include "matheditvariable.h"
#include "pagemathitem.h"
#include "helpers.h"

#include <QDebug>

MathEditVariable::MathEditVariable(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEdit(d, parentPageMathItem, parent) {
    
}
MathEditVariable::MathEditVariable(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEditVariable(d, parentPageMathItem, parent) {
    processInitText(text);
}

void MathEditVariable::init() {
    updateBoundingRect();
}

void MathEditVariable::keyPressEvent(QKeyEvent* event)
{
    //printContent();
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
        QString allowedChars = QString("ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789§$€?!.;\\abcdefghijklmnopqrstuvwxyz");
        for (QChar c: latinToGreek.values()) { allowedChars.append(c); }
        QString numCharacters = QString("0123456789.");
        
        if (allowedChars.contains(charToInsert) && !(numCharacters.contains(charToInsert) && m_cursorPos == 0)) {
            insertText(charToInsert);
            emit getParentPageMathItem()->itemDataChanged();
        } else if (charToInsert == "-" && m_cursorPos == 0) {
            insertText(charToInsert);
            emit getParentPageMathItem()->itemDataChanged();
        } else {
            if ((charToInsert == "-" || charToInsert == "+" ||
                 charToInsert == cdotChar || charToInsert == "/" ||
                 charToInsert == "^" ||
                 charToInsert == ":" || charToInsert == "=" ||
                 charToInsert == "<" || charToInsert == smallerEqual ||
                 charToInsert == ">" || charToInsert == greaterEqual ||
                 charToInsert == longEqual || charToInsert == notEqual
                ) && m_cursorPos == m_content.length()) {
                MathEdit* pa = qobject_cast<MathEdit*>(parentObject());
                pa->setFocus();
                pa->setCursorRightOf(this);
                pa->insertFnString(event);
                emit getParentPageMathItem()->itemDataChanged();
            }
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
    
    cursorPosUpdate();
    updateBoundingRect();
    update(boundingRect());
    event->accept();
}

QJsonObject MathEditVariable::toJson() const
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

void MathEditVariable::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathEdit::MEVariable) {
        m_content = object["content"].toString();
        setMathFontSize(object["mathFontSize"].toInt());
        
    } else {
        qDebug() << "MathLeafEdit::fromJson: Error on loading object!";
    }
}

void MathEditVariable::setValue(MathVariable m) {
    MathEdit::setValue(m);
}

void MathEditVariable::compute(const MathVariable& boolMask) {
    Q_UNUSED(boolMask);
    
    QString tmpStr = getContent();
    qreal sign = 1;
    if (tmpStr.at(0) == QChar('-')) {
        tmpStr.remove(0,1);
        sign = -1;
    }
    
    setValue(mul(sign, getData()->getValue(tmpStr)));
    if (std::numeric_limits<qreal>::max() == getValue().first()) {
        
    }
}
