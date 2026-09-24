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


class MathEditConditionalFunc: public MathEdit
{
    Q_OBJECT
    
public:
    static constexpr int Type = MEConditionalFunc;
    int type() const override { return Type; }
    
    explicit MathEditConditionalFunc(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent);
    explicit MathEditConditionalFunc(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent);
    
    void initialize() override;
    
    void setFocus(Qt::FocusReason focusReason = Qt::OtherFocusReason) override;
    void setFocusToBoolItem(int32_t ID, Qt::FocusReason focusReason = Qt::OtherFocusReason);
    void setFocusToExprItem(int32_t ID, Qt::FocusReason focusReason = Qt::OtherFocusReason);
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    void updateBoundingRect() override;
    
    void compute(const MathVariable& boolMask = MathVariable()) override;
    
    void setFocusToChild1(Qt::FocusReason focusReason = Qt::OtherFocusReason) override;
    void setFocusToChild2(Qt::FocusReason focusReason = Qt::OtherFocusReason) override;
    
    bool hasDescendantFocus() const override;
    
    QList<MathEdit*> getBoolList() const { return m_boolList; }
    QList<MathEdit*> getExprList() const { return m_exprList; }
    
    // Serialization functions
    QJsonObject toJson() const override; //
    void fromJson(const QJsonObject& object) override;
    
    bool shiftDel() override;
    void enter();
    
protected:
    void init();
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    bool processContent(int32_t selBegin, int32_t selEnd) override { return true; }
    
    bool isPartOfFuncStr(QString testStr);
    
private:
    void initializeBraces();
    void updateBracesPath(const QPointF& topLeft, qreal totalHeight, bool isLeftBrace);
    QPainterPath m_leftBracesPath, m_rightBracesPath;
    QGraphicsPathItem *m_leftBraces, *m_rightBraces;
    QPixmap m_leftBracesPixmap, m_rightBracesPixmap;
    bool m_paintBracesFast = false;
    
    QList<MathEdit*> m_boolList, m_exprList;
    
};
