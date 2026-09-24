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

#pragma once

#include <mathedit.h>

class MathEditFraction: public MathEdit
{
    Q_OBJECT
    
public:
    enum {
        posNumerator,
        posDenominator
    };
    static constexpr int Type = MEFraction;
    int type() const override { return Type; }
    
    explicit MathEditFraction(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent);
    explicit MathEditFraction(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent);
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    void updateBoundingRect() override;
    qreal baseline() const override;
    
    void compute(const MathVariable& boolMask = MathVariable()) override;
    
    MathEdit *getNumerator() { return m_numerator; }
    MathEdit *getDenominator() { return m_denominator; }
    
    void insertTextAt(int32_t pos, const QString& inText) override;
    void insertItemAt(int32_t pos, MathEdit *id) override;
    void insertItemsAt(int32_t pos, QList<MathEdit*> list) override;
    
    void appendToNumerator(QString cont, QList<MathEdit*> meList);
    void appendToDenominator(QString cont, QList<MathEdit*> meList);
    
    void setCursorToBegin() override;
    void setCursorToEnd() override;
    
    void setFocus(Qt::FocusReason focusReason = Qt::OtherFocusReason) override;
    void setFocusToChild1(Qt::FocusReason focusReason = Qt::OtherFocusReason) override;
    void setFocusToChild2(Qt::FocusReason focusReason = Qt::OtherFocusReason) override;
    bool hasDescendantFocus() const override;
    
    // Serialization functions
    QJsonObject toJson() const override; //
    void fromJson(const QJsonObject& object) override; //
    
protected:
    //void keyPressEvent(QKeyEvent* event) override;
    bool processContent(int32_t selBegin, int32_t selEnd) override { return true; }
    
    
private:
    MathEdit *m_numerator, *m_denominator;
    
    void printStructure() override;
    
};
