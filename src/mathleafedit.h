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

// mathleafedit.h
#ifndef MATHLEAFEDIT_H
#define MATHLEAFEDIT_H

#include <QCoreApplication>
#include <QGraphicsObject>
#include <QVariantList>
#include <QKeyEvent>
#include <QPainter>
#include <QFont>
#include <QTimer>
#include <QVariant>
#include <QFocusEvent>
#include <QJsonArray>
#include <QJsonObject>
#include "data.h"
#include "mathvariable.h"


class PageMathItem;

class MathLeafEdit : public QGraphicsObject
{
    Q_OBJECT
public:
    enum { Type = UserType + 4 }; // unique per subclass
    int type() const override { return Type; }
    
    explicit MathLeafEdit(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent = nullptr);
    explicit MathLeafEdit(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent = nullptr);
    ~MathLeafEdit() override;
    
    QRectF boundingRect() const override { return m_boundingRectangle; }
    QRectF boundingRectangle() { return m_boundingRectangle; }
    QRectF updateBoundingRect();
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    void paintDivision(QPainter* painter);
    void paintRoot(QPainter* painter);
    void paintLog(QPainter* painter);
    void paintPiecewiseFunction(QPainter* painter);
    void insertText(const QString& text, bool force = false); // force == true inserts the string even if it is a function string
    void insertText(QKeyEvent* event);
    void insertMathLeaf(MathLeafEdit* newLeaf);
    void backspace(bool childItemsAreMoved = false);
    void del();
    void reconnectChildLeaf(MathLeafEdit *child, MathLeafEdit *dest);
    void setFont(QFont font) { m_font = font; updateBoundingRectAndLayout(); }
    void setCursorToLast() { m_cursorPos = m_content.count(); setFocus(); }
    void setCursorToFirst() { m_cursorPos = 0; setFocus(); }
    int mathFontSize() {return m_mathFontSize; }
    QString fontName() { return m_fontName; }
    void setFontSize(int s) { m_mathFontSize = s; m_font.setPointSize(s); }// QFont font(m_fontName, m_mathFontSize); setFont(font);}
    QString currentTokenBackwards();
    QString currentTokenAll();
    QString tokenAllAt(int pos);
    QVariantList content() { return m_content; }
    
    // Serialization functions
    QJsonObject toJson() const; //
    void fromJson(const QJsonObject& object); //
    
    MathVariable getValue();
    void setValue(MathVariable v) { m_value = v; }
    QString getName() { return m_name; }
    qreal centerHeight() { return m_centerHeight; }
    
    bool isEmpty() const;
    bool isFunctionIsSet() { return m_isFunction; }
    bool isValidFunctionIsSet() { return m_isValidFunction; }
    bool isPiecewiseFunction() { return m_isPiecewiseFunction; }
    bool isExpression();
    bool isBinaryFunction();
    bool isValidValue(MathVariable L);
    bool isBasicOperator();
    bool isUnaryFunction();
    bool isAssignmentFunction();
    bool isResultFunction();
    bool showParenthesis() { return m_showParenthesis; }
    bool hasDescendantFocus() const;
    
    void setHighlighted(bool highlighted);
    void cleanContent();
    QString contStr() { return m_contStr; }
    
    void compute();
    
signals:
    void itemSizeChanged();
    void functionEntered(const QString& text);
    void gainedFocus();
    void lostFocus();
    void deleteMe(MathLeafEdit *me);
    void leftArrowAtStart(MathLeafEdit *me);
    void rightArrowAtEnd(MathLeafEdit *me);
    void simulateKeyPress(int key, const QString& text);
    void transferContentTo(MathLeafEdit *me);
    
public slots:
    void onTransferContentTo(MathLeafEdit *me);
    
protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    // Overrides for cursor changes on hover
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    
private:
    void updateBoundingRectAndLayout();
    void maybeFunction(QString text);
    void checkForValidFunction();
    void checkForParenthesis();
    void updateHoverCursor(const QPointF &pos);    // Helper to determine and set the appropriate cursor based on mouse position
    int getCursorIndexForPosition(qreal x);
    void clearSelection();
    void leftArrow();
    void rightArrow();
    void upArrow(MathLeafEdit *m);
    void downArrow(MathLeafEdit *m);
    
    bool isStringAtContent(int i);
    bool isString(const QVariant &v);
    bool isMathLeafEditAtContent(int i);
    bool isMathLeafEdit(const QVariant &v);
    
    bool isStringAtContent(int i) const;
    bool isString(const QVariant &v) const;
    bool isMathLeafEditAtContent(int i) const;
    bool isMathLeafEdit(const QVariant &v) const;
    
    QString getStringAtContent(int i);
    QString getString(const QVariant &v);
    MathLeafEdit* getMathLeafEditAtContent(int i);
    MathLeafEdit* getMathLeafEdit(const QVariant &v);
    
    QString getStringAtContent(int i) const;
    QString getString(const QVariant &v) const;
    MathLeafEdit* getMathLeafEditAtContent(int i) const;
    MathLeafEdit* getMathLeafEdit(const QVariant &v) const;
    
    void removeComputedOperatorOperands(int ID, QList<QString> &operatorList, QList<MathVariable> &operandList);
    
    void updateParenthesisPath(const QPointF& topLeft, qreal totalHeight, bool isLeftParenthesis);
    void updateBracesPath(const QPointF& topLeft, qreal totalHeight, bool isLeftBrace);
    QRectF getCursorRect() const { return QRectF(m_cursorX, m_cursorY, m_cursorW, m_cursorH);}
    
    Data *m_data;
    QString m_name; // if this represents a variable, this is the variable name
    MathVariable m_value; // Value or list of values that are represented by the name
    PageMathItem *m_parentPageMathItem;
    
    qreal m_paddingH = 2.0;
    qreal m_paddingV = 1.0;
    mutable qreal m_centerHeight = 0.0;
    void checkChildrenMaxCenterHeight();
    
    QVariantList m_content;
    int m_cursorPos = 0;
    QRectF m_textBoundingRect;
    mutable QRectF m_boundingRectangle;
    
    
    bool m_cursorVisible = false;
    bool m_isFunction = false;
    bool m_isValidFunction = false;
    bool m_isPiecewiseFunction = false;
    bool m_showParenthesis = false;
    bool m_showBraces = false;
    QPainterPath m_leftParenthesisPath, m_rightParenthesisPath;
    QGraphicsPathItem *m_leftParenthesis, *m_rightParenthesis;
    QPixmap m_leftParenthesisPixmap, m_rightParenthesisPixmap;
    QPainterPath m_leftBracesPath, m_rightBracesPath;
    QGraphicsPathItem *m_leftBraces, *m_rightBraces;
    QPixmap m_leftBracesPixmap, m_rightBracesPixmap;
    bool m_paintParenthesisFast = false;
    bool m_paintBracesFast = false;
    QTimer m_cursorTimer;
    QFont m_font;
    QString m_fontName = "Liberation Serif";
    int m_mathFontSize = 12;
    qreal m_subScriptScale = 0.75;
    qreal m_cursorX, m_cursorY, m_cursorW, m_cursorH;
    
    bool m_isHighlighted = false;
    int m_selectionStart = -1;
    int m_selectionEnd = -1;
    int m_selectionAnchor = -1;
    int m_persistSelectionStart = -1;
    int m_persistSelectionEnd = -1;
    
    void printContent();
    void updateContStr() const;
    mutable QString m_contStr;
    long int m_count = 0;
    
    QString m_mathError = "", m_mathWarning = "";
    QString resultString(bool units=false);
    
private slots:
    void toggleCursor();
    void updateChildPositions();
    void removeChild(MathLeafEdit *child);
    void setCursorBeforeChild(MathLeafEdit *child);
    void setCursorAfterChild(MathLeafEdit *child);
    void handleSimulatedKeyPress(int key, const QString& text);
    void refreshLayout();
};

#endif // MATHLEAFEDIT_H
