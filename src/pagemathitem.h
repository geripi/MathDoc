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

// pagemathitem.h

#ifndef PAGEMATHITEM_H
#define PAGEMATHITEM_H

#include <QGraphicsObject>
#include <QGraphicsSceneHoverEvent>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
//#include "mathleafedit.h"
#include "mathedit.h"
#include "data.h"

class PageMathItem : public QGraphicsObject
{
    Q_OBJECT 
    
public:
    enum { Type = UserType + 3 }; // unique type ID per subclass
    int type() const override { return Type; }
    static constexpr qreal BORDER_SIZE = 3.0; // User-requested border width
    
    explicit PageMathItem(Data *d, QGraphicsItem* parent = nullptr);
    
    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    // Add child items to the frame
    void addItem(QGraphicsItem* item);
    // Add a child MathLeafEdit
    //void setMathLeaf(MathLeafEdit* leaf);
    void setMathLeaf(MathEdit* leaf);
    //MathLeafEdit* getMathLeaf() const { return m_mathLeaf; }
    //MathLeafEdit* rootMathLeaf() const { return m_mathLeaf; }
    MathEdit* getMathLeaf() const { return m_mathLeaf; }
    MathEdit* rootMathLeaf() const { return m_mathLeaf; }
    void compute();
    
    bool isEditing() const { return m_isEditing; }
    bool showStructure() const { return m_showStructure; }
    bool getIsInDiagramTitle() const { return m_isInDiagramTitle; }
    void setIsInDiagramTitle (bool b) { m_isInDiagramTitle = b; }
    
    QJsonObject toJson() const;
    static PageMathItem* fromJson(const QJsonObject& json, Data* d, QGraphicsItem* parent = nullptr);
    void addErrorMessage(const QString &m);
    void addWarningMessage(const QString &m);
    void setFont(QFont font) { m_font = font; }
    
    bool hasErrorMessages() {
        if (true) {
            bool b = (m_ErrorMessageList.length() > 0);
            return b;
        } else {return false;}
        }
    bool hasWarningMessages() { bool b = (m_WarningMessageList.length() > 0); return b; }
    
    void checkSanity(QString msg);
    
    void printStructure();
    
    void blockCompute() { m_isComputeBlocked = true; }
    void releaseCompute() { m_isComputeBlocked = false; }
    
signals:
    void editingStateChanged(bool isEditing);
    // Signal emitted when the content or geometry of this item changes.
    void itemDataChanged();
    // Signals to inform about editing state changes
    void editingStarted(); // Emitted when text editing is enabled
    void editingFinished(); // Emitted when text editing is disabled
    void itemSelected(); // Emitted when this is selected (either click on border or click inside)
    void computeWholeDocument(); // Emitted when the whole document should be computed
    
    
public slots:
    void onChildFocusIn();
    void onChildFocusOut();
    // Slot to be connected to child signals to trigger a layout update
    void updateLayout();
    
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    // Overrides for cursor changes on hover
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    // Override itemChange to emit itemDataChanged when position changes
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    
private:
    QPointF m_lastMousePos;    // Store the last mouse press position for drag operations.
    qreal m_padding = 5.0;
    bool m_isEditing = false;
    bool m_isComputeBlocked = false;
    bool m_isComputing = false;
    bool m_showStructure = false;
    bool m_isInDiagramTitle = false;
    void updateHoverCursor(const QPointF &pos);    // Helper to determine and set the appropriate cursor based on mouse position
//    MathLeafEdit* m_mathLeaf = nullptr;
    MathEdit* m_mathLeaf = nullptr;
    QPointF m_dragStartPos;
    Data *m_data;
    QTimer m_focusCheckTimer;
    QStringList m_ErrorMessageList, m_WarningMessageList;
    QFont m_font;
    QString m_fontName = "Liberation Serif";
    int m_mathFontSize = 12;
    
    void paintMesssageList(const QStringList &list, QPainter* painter);
    
    
private slots:
    void checkFocusAndEditingState();
};

#endif // PAGEMATHITEM_H
