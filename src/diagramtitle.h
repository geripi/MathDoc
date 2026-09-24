#ifndef DIAGRAMTITLE_H
#define DIAGRAMTITLE_H

#include "pagediagramitem.h"
#include "pagemathitem.h"
#include "data.h"

#include <QGraphicsObject>
#include <QString>
#include <QJsonObject>

class DiagramTitle : public QGraphicsObject
{
    Q_OBJECT
    
public:
    explicit DiagramTitle(Data *d, PageDiagramItem *parent);
    explicit DiagramTitle(const QString &text, Data *d, PageDiagramItem *parent);
    
    ~DiagramTitle() override = default;
    void initUnit(PageMathItem *u = nullptr);
    
    QString text() const{ return m_text; }
    void setText(const QString &text);
    
    // Serialization functions
    QJsonObject toJson() const; //
    void fromJson(const QJsonObject& object, Data *data, QGraphicsItem *parent); //
    
    PageMathItem *getUnit() const{ return m_unit; }
    
    PageDiagramItem *getParent() const{ return m_parent; }
    
    QFont font() const{ return m_font; }
    void setFont(const QFont &f) { m_font = f; }
    QFontMetricsF fontMetrics() const{ return m_fontMetrics; }
    void setFontMetrics(const QFontMetricsF &fm) { m_fontMetrics = fm; }
    QFont subFont() const{ return m_subFont; }
    void setSubFont(const QFont &f) { m_subFont = f; }
    QFontMetricsF subFontMetrics() const{ return m_subFontMetrics; }
    void setSubFontMetrics(const QFontMetricsF &fm) { m_subFontMetrics = fm; }
    qreal mathFontSize() const{ return m_mathFontSize; }
    void setMathFontSize(qreal s);
    
    QRectF boundingRect() const override { return m_boundingRect; }
    QRectF getBoundingRectangle() { return boundingRect(); }
    void updateBoundingRect();
    
    void compute();
    
    qreal x() const{ return boundingRect().x(); }
    qreal y() const{ return boundingRect().y(); }
    qreal width() const{ return boundingRect().width(); }
    qreal height() const{ return boundingRect().height(); }
    qreal bottom() const{ return height() + y(); }
    
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    
    void keyPressEvent(QKeyEvent* event) override;
    void insertText(const QString& inText);
    void insertTextAt(int64_t pos, const QString& inText);
    
    bool processAllContent();
    
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
    void backspace();
    void del();
    virtual void leftArrow();
    void shiftLeftArrow();
    virtual void rightArrow();
    void shiftRightArrow();
    virtual void upArrow();
    virtual void downArrow();
    void cursorPosUpdate();
    virtual bool processContent(int64_t selBegin, int64_t selEnd);
    
    qreal textWidth(const QString &text);
    qreal subtextWidth(const QString &text);
    
    int64_t getCursorIndexForPosition(qreal x);
    qreal getPositionForIndex(int64_t id);
    
    
private:
    QString m_text, m_autoUnit;
    PageMathItem *m_unit;
    
    Data *m_data;
    PageDiagramItem *m_parent;
    
    int64_t m_cursorPos = 0.0;
    QTimer m_cursorTimer;
    bool m_cursorVisible = false;
    qreal m_cursorX, m_cursorY, m_cursorW, m_cursorH;
    int64_t m_selectBegin = -1, m_selectEnd = -1, m_selectAnchor = -1;
    QRectF getCursorRect() const { return QRectF(m_cursorX, m_cursorY, m_cursorW, m_cursorH);}
    
    
    QString m_fontName = "Liberation Sans";
    qreal m_mathFontSize = 14;
    static constexpr qreal m_subScriptScale = 0.75;
    QFont m_font, m_subFont;
    QFontMetricsF m_fontMetrics, m_subFontMetrics;
    const qreal m_paddingH = 2.0, m_paddingV = 2.0;
    
    QRectF m_boundingRect;
    
private slots:
    void toggleCursor();
    
};

#endif // DIAGRAMTITLE_H

