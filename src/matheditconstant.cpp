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

#include "matheditconstant.h"
#include "pagemathitem.h"
#include "helpers.h"

#include <QDebug>

MathEditConstant::MathEditConstant(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEdit(d, parentPageMathItem, parent) {
    init();
}
MathEditConstant::MathEditConstant(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent)
:MathEditConstant(d, parentPageMathItem, parent) {
    processInitText(text);
}

void MathEditConstant::init() {
    updateBoundingRect();
}

void MathEditConstant::keyPressEvent(QKeyEvent* event)
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
        //insertText(charToInsert);
        QString newContent = m_content;
        newContent.insert(m_cursorPos, charToInsert);
        
        int pos = m_cursorPos;
        QValidator::State state = m_validator.validate(newContent, pos);
        
        if (state != QValidator::Invalid) {
            m_content = newContent;
            ++m_cursorPos;
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

QJsonObject MathEditConstant::toJson() const {
    QJsonObject object;
    object["type"] = type();
    object["content"] = m_content;
    object["mathFontSize"] = getMathFontSize();
    
    QJsonArray childItemsArray;
    for(MathEdit *child: getContItems()) {
        childItemsArray.append(child->toJson());
    }
    object["childItems"] = childItemsArray;
    
    return object;
}

void MathEditConstant::fromJson(const QJsonObject& object) {
    m_content.clear();
    int type = object["type"].toInt();
    if (type == MathEdit::MEConstant) {
        m_content = object["content"].toString();
        setMathFontSize(object["mathFontSize"].toInt());
    } else {
        qDebug() << "MathEditConstant::fromJson: Error on loading object!";
    }
}

void MathEditConstant::compute(const MathVariable& boolMask) {
    if (canConvertToMathVariable(getContent())) {
        setValue(convertToMathVariable(getContent()));
    } else {
        setValue(QList<qreal>{std::numeric_limits<qreal>::max()});
        setMathError(tr("ERROR: Input must be one number or\n"
                        "       a list of 3 numbers (start, end, count)"));
        qDebug() << getMathError();
    }
}
