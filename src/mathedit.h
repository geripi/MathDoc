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

#ifndef MATHEDIT_H
#define MATHEDIT_H

#include <QGraphicsItem>
#include <QKeyEvent>
#include <QPainter>
#include <QFont>
#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>

#include "data.h"
#include "mathvariable.h"

class PageMathItem;

/**
 * Base class for all the MathEdit classes that can occur when a formmula is required
 */
class MathEdit : public QGraphicsObject
{
    Q_OBJECT

public:
    enum {
        MEBase = QGraphicsItem::UserType + 100,
        MEOperator,
        MEVariable,
        MEConstant,
        MEFunction,
        MEFraction,
        MEExponent,
        MERoot,
        MELog,
        MEConditionalFunc
    };
    
    
    static constexpr int Type = MEBase;
    int type() const override { return Type; }
    static QString typeStr(int type);
    
    explicit MathEdit(Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent = nullptr);
    explicit MathEdit(QString text, Data *d, PageMathItem *parentPageMathItem, QGraphicsObject* parent = nullptr);
    
    virtual void initialize() { setFocus(); updateBoundingRect(); }
    
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    
    QRectF boundingRect() const override { return m_boundingRectangle; }
    virtual void updateBoundingRect();
    void updateBoundingRectForWholeTree();
    void notifyParentSizeChange();
    virtual qreal baseline() const;
    virtual qreal verticalOffset(MathEdit *referenceItem = nullptr) const;
    
    // Serialization functions
    virtual QJsonObject toJson() const; //
    virtual void fromJson(const QJsonObject& object); //
    virtual void addContItem(MathEdit *me) { m_contItems.append(me); }
    
    virtual bool hasDescendantFocus() const;
    bool isEmpty() const;
    QString getContent() const { return m_content; }
    void setContent(QString s) { m_content = s; }
    MathVariable getValue();
    virtual void setValue(MathVariable m) { m_value = m; }
    QString getMathError() { return m_mathError; }
    void setMathError(QString s) { m_mathError = s; }
    QString getMathWarning() { return m_mathWarning; }
    void setMathWarning(QString s);
    bool isUsedAsUnit() { return m_isUsedAsUnit; }
    void setIsUsedAsUnit(bool b) { m_isUsedAsUnit = b; }
    
    virtual void compute(const MathVariable& boolMask = MathVariable());
    
    void cleanContent();
    
    PageMathItem *getParentPageMathItem() { return m_parentPageMathItem; }
    virtual void setCursorToBegin() { m_cursorPos = 0; }
    virtual void setCursorToEnd() { m_cursorPos = m_content.length(); }
    int32_t getCursorPos() const { return m_cursorPos; }
    void setCursorTo(int32_t p) { if (p>m_content.length()) setCursorToEnd(); else if (p<0) setCursorToBegin(); else m_cursorPos = p; }
    virtual void setCursorRightOf(MathEdit* child);
    virtual void setCursorLeftOf(MathEdit* child);
    bool isCursorVisible() { return m_cursorVisible; }
    qreal getCursorX() { return m_cursorX; }
    qreal getCursorY() { return m_cursorY; }
    qreal getCursorW() { return m_cursorW; }
    qreal getCursorH() { return m_cursorH; }
    int32_t getSelectBegin() { return m_selectBegin; }
    void setSelectBegin(int32_t i) { m_selectBegin = i; }
    int32_t getSelectEnd() { return m_selectEnd; }
    void setSelectEnd(int32_t i) { m_selectEnd = i; }
    int32_t getSelectAnchor() { return m_selectAnchor; }
    void setSelectAnchor(int32_t i) { m_selectAnchor = i; }
    
    MathEdit* getChildAtPos(int32_t pos);
    MathEdit* getChildAtCursorPos() { return getChildAtPos(m_cursorPos); } //abc#def
    MathEdit* getChildBeforeCursorPos() { return getChildAtPos(m_cursorPos-1); }
    int getChildIndexAtPos(int32_t pos);
    int getChildIndexAtCursorPos() { return getChildIndexAtPos(m_cursorPos); }
    int getChildIndexBeforeCursorPos() { return getChildIndexAtPos(m_cursorPos-1); }
    std::pair<int32_t, int32_t> getChildRangeIDsAt(int32_t begin, int32_t length);
    MathEdit* getUnit() { return m_unit; }
    void setUnit(MathEdit *m) { m_unit = m; }
    
    void processInitText(QString text); // TODO: Implement what should happen when initial string is given to c'tor
    virtual void insertText(const QString& inText);
    virtual void insertTextAt(int32_t pos, const QString& inText);
    virtual void insertItem(MathEdit *id);
    virtual void insertItems(QList<MathEdit*> list);
    virtual void insertItemAt(int32_t pos, MathEdit *id);
    virtual void insertItemsAt(int32_t pos, QList<MathEdit*> list);
    void insertFnString(QKeyEvent* event);
    
    QString resultString(bool units);
    void appendSIUnit(QString unitStr, qreal exponent, bool withMultiplication);
    
    MathEdit* createNewItem(QString s, int Type);
    MathEdit* createNewVariableAndItem(QString s, int Type);
    MathEdit* createNewVariable();
    Data *getData() { return m_data; }
    QString getResult() { return m_result; }
    void setResult(QString s) { m_result = s; }
    
    bool childBeforeIsOperator(int32_t pos);
    bool cursorIsDirectlyAfterOperatorOrAtBegin(int32_t pos);
    bool cursorIsDirectlyAfterOther(int32_t pos);
    
    int getMathFontSize() const { return m_mathFontSize; }
    void setMathFontSize(int size);
    QString getFontName() const { return m_fontName; }
    void setFontName(QString s) { m_fontName = s; }
    qreal getSubScriptScale() const {return m_subScriptScale; }
    void setSubScriptScale(qreal r) { m_subScriptScale = r; }
    QFont getFont() const { return m_font; }
    void setFont(QFont f) { m_font = f; }
    QFont getSubFont() const { return m_subfont; }
    void setSubFont(QFont f) { m_subfont = f; }
    
    QRectF getBoundingRectangle() const { return m_boundingRectangle; }
    void setBoundingRectangle(QRectF r) { m_boundingRectangle = r; }
    
    MathEdit* getCashedChild() { return m_cashedChild; }
    void setCashedChild(MathEdit* c) { m_cashedChild= c; }
    bool getRightArrowPressed() { return m_rightArrowPressed; }
    void setRightArrowPressed(bool b) {m_rightArrowPressed = b; }
    bool getLeftArrowPressed() { return m_leftArrowPressed; }
    void setLeftArrowPressed(bool b) {m_leftArrowPressed = b; }
    
    bool processAllContent();
    
    virtual void setFocus(Qt::FocusReason focusReason = Qt::OtherFocusReason);
    virtual void setFocusToChild1(Qt::FocusReason focusReason = Qt::OtherFocusReason);
    virtual void setFocusToChild2(Qt::FocusReason focusReason = Qt::OtherFocusReason);
    
    virtual void printStructure();
    virtual void printMathExpression(QList<MathEdit*> &MEList);
    
    void keyPressEvent(QKeyEvent* event) override;
    virtual bool shiftDel();
    
signals:
    void itemSizeChanged();
    void gainedFocus();
    void lostFocus();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    bool isCommandKey(const int key);
    int getChunkType(QString chunk);
    int32_t getChunkStart(int32_t pos);
    void backspace();
    void del();
    virtual void leftArrow();
    void shiftLeftArrow();
    virtual void rightArrow();
    void shiftRightArrow();
    virtual void upArrow();
    virtual void downArrow();
    void cursorPosUpdate();
    virtual bool processContent(int32_t selBegin, int32_t selEnd);
    
    QString m_content;
    int32_t m_cursorPos = 0;
    
    qreal textWidth(const QString &text);
    qreal subtextWidth(const QString &text);
    
    int getCursorIndexForPosition(qreal x);
    qreal getPositionForIndex(int id);
    
    qreal getCenterHeight() { return m_centerHeight; }
    void setCenterHeight(qreal r) { m_centerHeight = r; }
    
    QList<MathEdit*> getContItems() const { return m_contItems; }
    
    const qreal getPaddingH() const { return m_paddingH;}
    const qreal getPaddingV() const { return m_paddingV;}
    
    
private:
    QList<MathEdit*> m_contItems;
    
    MathVariable m_value; // Value or list of values that are represented by the name
    QString m_result = QString("");
    MathEdit *m_unit = nullptr;
    bool m_isUsedAsUnit = false;
    
    MathEdit *m_cashedChild = nullptr;
    
    bool m_rightArrowPressed = false, m_leftArrowPressed = false;
    
    Data *m_data;
    PageMathItem *m_parentPageMathItem;
    
    QRectF m_boundingRectangle;
    mutable qreal m_centerHeight = 0.0;
    
    QFont m_font, m_subfont;
    QString m_fontName = "Liberation Serif";
    int m_mathFontSize;
    qreal m_subScriptScale = 0.75;
    const qreal m_paddingH = 2.0, m_paddingV = 2.0;
    
    QTimer m_cursorTimer;
    bool m_cursorVisible = false;
    qreal m_cursorX, m_cursorY, m_cursorW, m_cursorH;
    int32_t m_selectBegin = -1, m_selectEnd = -1, m_selectAnchor = -1;
    QRectF getCursorRect() const { return QRectF(m_cursorX, m_cursorY, m_cursorW, m_cursorH);}
    
    QString getCurrentChunk();
    QString getChunkAt(int32_t pos);
    
    virtual void printContent();
    
    QString m_mathError, m_mathWarning;
    
private slots:
    void toggleCursor();
    void refreshLayout();
    

};

#endif // MATHEDIT_H
