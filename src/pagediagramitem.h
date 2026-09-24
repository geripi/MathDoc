#ifndef PAGEDIAGRAMITEM_H
#define PAGEDIAGRAMITEM_H

#include <QGraphicsObject>
#include <QGraphicsProxyWidget>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QJsonObject>
#include <QJsonArray>
#include <QFont>
#include <QFontMetricsF>
#include <QPen>
#include <QBrush>
#include <QPointer>

#include "data.h"
#include "matheditvariable.h"

class DiagramTitle;

class PageDiagramItem : public QGraphicsObject
{
    Q_OBJECT
    
public:
    enum { Type = UserType + 4 };
    int type() const override { return Type; }
    
    PageDiagramItem(Data* d = nullptr, QGraphicsItem* parent = nullptr);
    ~PageDiagramItem() override = default;
    
    void initTitles();
    // ------------------------------------------------------------
    // Geometry
    QSizeF diagramSize() const;
    void setDiagramSize(const QSizeF &size);
    static constexpr qreal DEFAULT_WIDTH  = 600.0;
    static constexpr qreal DEFAULT_HEIGHT = 400.0;
    
    // ------------------------------------------------------------
    // Data
    void setData(Data *data);
    Data *data() const { return m_data; }
    QGraphicsItem *getParent() const { return m_parent; }
    void setXUnitMultiplier(qreal m) { m_xUnitMultiplier = m; }
    void setYUnitMultiplier(qreal m) { m_yUnitMultiplier = m; }
    
    void refresh();
    void compute();
    
    // ------------------------------------------------------------
    // Variable names
    QString xVariable() const, yVariable() const, title() const;
    void setXVariable(const QString &name);
    void setYVariable(const QString &name);
    void setTitle(const QString &title);
    
    DiagramTitle *getXEdit() { return m_xEdit; }
    void setXEdit(const QJsonObject &object);
    DiagramTitle *getYEdit() { return m_yEdit; }
    void setYEdit(const QJsonObject &object);
    
    // ------------------------------------------------------------
    // Automatic/manual scaling
    bool autoScaleX() const { return m_autoScaleX; }
    bool autoScaleY() const { return m_autoScaleY; }
    
    void setAutoScaleX(bool enabled);
    void setAutoScaleY(bool enabled);
    
    void setXRange(qreal minimum, qreal maximum);
    void setYRange(qreal minimum, qreal maximum);
    
    qreal xMinimum() const { return m_xMin; }
    qreal xMaximum() const { return m_xMax; }
    qreal yMinimum() const { return m_yMin; }
    qreal yMaximum() const { return m_yMax; }
    
    // ------------------------------------------------------------
    // JSON
    QJsonObject toJson() const;
    static PageDiagramItem *fromJson(const QJsonObject &object,
                                     Data *data,
                                     QGraphicsItem *parent = nullptr);
    
signals:
    void itemDataChanged();
    void itemSelected();
    
public slots:
    void onChildFocusIn();
    void onChildFocusOut();
    // Slot to be connected to child signals to trigger a layout update
    void updateLayout();
    
protected:
    QRectF boundingRect() const override;
    
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
   
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
                                   
private slots:
    void variableTextChanged();
    void titleTextChanged();
    void editingFinished();
    
private:
    // ------------------------------------------------------------
    // Internal structures
    struct PlotData {
        QVector<qreal> x;
        QVector<qreal> y;
        bool valid = false;
    };
    
    // ------------------------------------------------------------
    // Drawing
    QRectF plotRect() const;
    
    void drawBackground(QPainter *painter);
    void drawGrid(QPainter *painter);
    void drawAxes(QPainter *painter);
    void drawTicks(QPainter *painter);
    void drawCurve(QPainter *painter);
    void drawSelection(QPainter *painter, const QStyleOptionGraphicsItem *option);
    
    // ------------------------------------------------------------
    // Scaling
    void calculateAutomaticRanges();
    void calculateAutomaticXRange();
    void calculateAutomaticYRange();
    
    void ensureValidRanges();
    
    // ------------------------------------------------------------
    // Plot conversion
    QPointF dataToScene(qreal x, qreal y) const;
    qreal niceNumber(qreal value, bool round) const;
    QVector<qreal> generateTicks(qreal minimum, qreal maximum, int targetCount = 10) const;
    QVector<qreal> generateSubTicks(const QVector<qreal> &ticks, bool isXAxis, int targetCount = 10) const;
    QString formatXTick(qreal value) const;
    QString formatYTick(qreal value) const;
    
    // ------------------------------------------------------------
    // Data
    PlotData getPlotData() const;
    QVector<qreal> mathVariableValues(const MathVariable &variable) const;
    qreal m_xUnitMultiplier = 1.0, m_yUnitMultiplier = 1.0;
    
    // ------------------------------------------------------------
    // Widgets
    QGraphicsProxyWidget *m_titleEditProxy = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    DiagramTitle *m_xEdit = nullptr, *m_yEdit = nullptr;
    
    // ------------------------------------------------------------
    // Data
    Data *m_data;
    QGraphicsItem *m_parent;
    
    // ------------------------------------------------------------
    // Geometry
    QSizeF m_size;
    qreal m_yAxisTicsWidth;
    qreal axisTextWidth(const QString &text);
    void updateMargins();
    
    // ------------------------------------------------------------
    // Axis state
    bool m_autoScaleX = true, m_autoScaleY = true;
    qreal m_xMin = -1.0, m_xMax =  1.0, m_yMin = -1.0, m_yMax =  1.0;
    
    // ------------------------------------------------------------
    // Plot appearance
    QFont m_axisFont, m_titleFont;
    QFontMetricsF m_axisFm, m_titleFm;
    
    QColor m_backgroundColor = Qt::white;
    QColor m_gridColor = QColor(127, 127, 127);
    QColor m_subGridColor = QColor(192, 192, 192);
    QColor m_axisColor = Qt::black;
    QColor m_axisBGColor = QColor(247, 247, 247);
    QColor m_curveColor = QColor(30, 100, 220);
    
    qreal m_gridPenWidth = 0.7;
    qreal m_curvePenWidth = 2.0;
    
    // Margins around the actual plotting area
    qreal m_leftMargin   = 65.0, m_rightMargin  = 20.0, m_topMargin = 65.0, m_bottomMargin = 45.0;
};

#endif // PAGEDIAGRAMITEM_H
