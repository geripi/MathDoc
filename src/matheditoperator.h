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

#include <QValidator>

class MathEditOperator: public MathEdit
{
    Q_OBJECT
    
public:
    static constexpr int Type = MEOperator;
    int type() const override { return Type; }
    
    explicit MathEditOperator(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent);
    explicit MathEditOperator(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent);
    
    // Serialization functions
    QJsonObject toJson() const override; //
    void fromJson(const QJsonObject& object) override; //
    
    void compute(const MathVariable& boolMask = MathVariable()) override { Q_UNUSED(boolMask); }
    
protected:
    void keyPressEvent(QKeyEvent* event) override;
    bool processContent(int64_t selBegin, int64_t selEnd) override { Q_UNUSED(selBegin); Q_UNUSED(selEnd); return true; }
    
    
private:
    
    
};
