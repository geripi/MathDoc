#include "pagediagramitem.h"
#include "diagramtitle.h"
#include "helpers.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QTextList>
#include <QDebug>
#include <QtMath>
#include <cmath>
#include <algorithm>

PageDiagramItem::PageDiagramItem(Data *d, QGraphicsItem *parent)
: QGraphicsObject(parent),
  m_data(d),
  m_parent(parent),
  m_size(DEFAULT_WIDTH, DEFAULT_HEIGHT),
  m_axisFont("Liberation Sans", 12),
  m_titleFont("Liberation Sans", 16, QFont::Bold),
  m_axisFm(m_axisFont),
  m_titleFm(m_titleFont)
{
    setFlags(ItemIsMovable |
    ItemIsSelectable |
    ItemSendsGeometryChanges |
    ItemIsFocusable);
    
    setAcceptHoverEvents(true);
    setZValue(1);
    
    // ------------------------------------------------------------
    // Title editor
    m_titleEdit = new QLineEdit();
    m_titleEdit->setPlaceholderText("Diagram title");
    m_titleEdit->setAlignment(Qt::AlignCenter);
    m_titleEdit->setFont(m_titleFont);
    //m_titleEdit->resize(m_size.width() * 0.8, m_titleFm.height());
    
    m_titleEditProxy = new QGraphicsProxyWidget(this);
    m_titleEditProxy->setWidget(m_titleEdit);
    m_titleEditProxy->setMinimumHeight(0);
    //m_titleEditProxy->setPos(m_size.width() * 0.1, 5.0);
    
    // ------------------------------------------------------------
    // Signals
    connect(m_titleEdit, &QLineEdit::textChanged, this, &PageDiagramItem::titleTextChanged);
    connect(m_titleEdit, &QLineEdit::editingFinished, this, &PageDiagramItem::editingFinished);
}

void PageDiagramItem::initTitles() {
    m_xEdit = new DiagramTitle("x", data(), this);
    m_xEdit->initUnit();
    m_xEdit->updateBoundingRect();
    
    m_yEdit = new DiagramTitle("y", data(), this);
    m_yEdit->initUnit();
    m_yEdit->updateBoundingRect();
    
    connect(m_xEdit, &DiagramTitle::itemSizeChanged, this, &PageDiagramItem::updateLayout);
    connect(m_xEdit, &DiagramTitle::gainedFocus, this, &PageDiagramItem::onChildFocusIn);
    connect(m_xEdit, &DiagramTitle::lostFocus, this, &PageDiagramItem::onChildFocusOut);
    connect(m_yEdit, &DiagramTitle::itemSizeChanged, this, &PageDiagramItem::updateLayout);
    connect(m_yEdit, &DiagramTitle::gainedFocus, this, &PageDiagramItem::onChildFocusIn);
    connect(m_yEdit, &DiagramTitle::lostFocus, this, &PageDiagramItem::onChildFocusOut);
    
    refresh();
}

// ============================================================
// Geometry
QSizeF PageDiagramItem::diagramSize() const {
    return m_size;
}

void PageDiagramItem::setDiagramSize(const QSizeF &size) {
    if (size.width() < 200.0 || size.height() < 150.0) return;
    
    //prepareGeometryChange();
    m_size = size;
    
    //updateMargins();
    
    //update();
    //emit itemDataChanged();
}

QRectF PageDiagramItem::boundingRect() const {
    return QRectF(0, 0, m_size.width(), m_size.height());
}

// ============================================================
// Data
void PageDiagramItem::setData(Data *data) {
    m_data = data;
    refresh();
}

void PageDiagramItem::refresh() {
    compute();
    if (m_autoScaleX || m_autoScaleY)
        calculateAutomaticRanges();
    
    ensureValidRanges();
    updateMargins();
    update();
}

void PageDiagramItem::compute() {
    m_xEdit->compute();
    m_yEdit->compute();
}

// ============================================================
// Variable names
QString PageDiagramItem::xVariable() const {
    return m_xEdit->text();
}

QString PageDiagramItem::yVariable() const {
    return m_yEdit->text();
}

QString PageDiagramItem::title() const {
    return m_titleEdit->text();
}

void PageDiagramItem::setXVariable(const QString &name) {
    m_xEdit->setText(name);
    refresh();
}

void PageDiagramItem::setYVariable(const QString &name) {
    m_yEdit->setText(name);
    refresh();
}

void PageDiagramItem::setTitle(const QString &title) {
    m_titleEdit->setText(title);
    update();
}

void PageDiagramItem::setXEdit(const QJsonObject &object) {
    m_xEdit = new DiagramTitle(m_data, this);
    m_xEdit->fromJson(object, m_data, this);
    
    connect(m_xEdit, &DiagramTitle::itemSizeChanged, this, &PageDiagramItem::updateLayout);
    connect(m_xEdit, &DiagramTitle::gainedFocus, this, &PageDiagramItem::onChildFocusIn);
    connect(m_xEdit, &DiagramTitle::lostFocus, this, &PageDiagramItem::onChildFocusOut);
}

void PageDiagramItem::setYEdit(const QJsonObject &object) {
    m_yEdit = new DiagramTitle(m_data, this);
    m_yEdit->fromJson(object, m_data, this);
    
    connect(m_yEdit, &DiagramTitle::itemSizeChanged, this, &PageDiagramItem::updateLayout);
    connect(m_yEdit, &DiagramTitle::gainedFocus, this, &PageDiagramItem::onChildFocusIn);
    connect(m_yEdit, &DiagramTitle::lostFocus, this, &PageDiagramItem::onChildFocusOut);
}

// ============================================================
// Slots
void PageDiagramItem::variableTextChanged() {
    refresh();
    emit itemDataChanged();
}

void PageDiagramItem::titleTextChanged() {
    update();
    emit itemDataChanged();
}

void PageDiagramItem::editingFinished() {
    refresh();
}

// ============================================================
// Scaling
void PageDiagramItem::setAutoScaleX(bool enabled) {
    m_autoScaleX = enabled;
    if (enabled) calculateAutomaticXRange();
    
    ensureValidRanges();
    update();
    
    emit itemDataChanged();
}

void PageDiagramItem::setAutoScaleY(bool enabled) {
    m_autoScaleY = enabled;
    
    if (enabled)
        calculateAutomaticYRange();
    
    ensureValidRanges();
    update();
    
    emit itemDataChanged();
}

void PageDiagramItem::setXRange(qreal minimum, qreal maximum) {
    if (!std::isfinite(minimum) ||
        !std::isfinite(maximum) ||
        minimum >= maximum)
        return;
    
    m_xMin = minimum;
    m_xMax = maximum;
    
    m_autoScaleX = false;
    
    update();
    emit itemDataChanged();
}

void PageDiagramItem::setYRange(qreal minimum, qreal maximum) {
    if (!std::isfinite(minimum) ||
        !std::isfinite(maximum) ||
        minimum >= maximum)
        return;
    
    m_yMin = minimum;
    m_yMax = maximum;
    
    m_autoScaleY = false;
    
    update();
    emit itemDataChanged();
}

// ============================================================
// MathVariable -> QVector
QVector<qreal> PageDiagramItem::mathVariableValues(const MathVariable &variable) const {
    return variable.values().toVector();
}

// ============================================================
// Get plot data
PageDiagramItem::PlotData PageDiagramItem::getPlotData() const {
    PlotData result;
    
    if (!m_data) return result;
    
    const QString xName = xVariable();
    const QString yName = yVariable();
    
    if (xName.isEmpty() || yName.isEmpty())
        return result;
    
    if (!m_data->contains(xName) ||
        !m_data->contains(yName)) {
        return result;
    }
    
    const MathVariable xVariableValue = mul(m_xUnitMultiplier, m_data->getValue(xName));
    const MathVariable yVariableValue = mul(m_yUnitMultiplier, m_data->getValue(yName));
    
    result.x = mathVariableValues(xVariableValue);
    result.y = mathVariableValues(yVariableValue);
    
    if (result.x.isEmpty() ||
        result.y.isEmpty()) {
        return result;
    }
    
    /*
     * Only plot matching pairs.
     *
     * This also protects us if the user accidentally selects
     * variables with different lengths.
     */
    const int count = std::min(result.x.size(), result.y.size());
    
    result.x.resize(count);
    result.y.resize(count);
    
    result.valid = count > 0;
    
    return result;
}

// ============================================================
// Automatic scaling
void PageDiagramItem::calculateAutomaticRanges() {
    if (m_autoScaleX) calculateAutomaticXRange();
    if (m_autoScaleY) calculateAutomaticYRange();
    
    ensureValidRanges();
}

void PageDiagramItem::calculateAutomaticXRange() {
    PlotData data = getPlotData();
    
    if (!data.valid)
        return;
    
    qreal minValue =
    std::numeric_limits<qreal>::max();
    
    qreal maxValue =
    std::numeric_limits<qreal>::lowest();
    
    for (qreal value : data.x) {
        if (!std::isfinite(value))
            continue;
        
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }
    
    if (minValue == std::numeric_limits<qreal>::max()) return;
    
    if (qFuzzyCompare(minValue, maxValue)) {
        qreal delta = std::max(std::abs(minValue) * 0.1, 1.0);
        minValue -= delta;
        maxValue += delta;
    } else {
        const qreal padding = (maxValue - minValue) * 0.05;
        minValue -= padding;
        maxValue += padding;
    }
    
    m_xMin = minValue;
    m_xMax = maxValue;
}

void PageDiagramItem::calculateAutomaticYRange() {
    PlotData data = getPlotData();
    
    if (!data.valid) return;
    
    qreal minValue = std::numeric_limits<qreal>::max();
    qreal maxValue = std::numeric_limits<qreal>::lowest();
    
    for (qreal value : data.y) {
        if (!std::isfinite(value)) continue;
        
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }
    if (minValue == std::numeric_limits<qreal>::max()) return;
    
    if (qFuzzyCompare(minValue, maxValue)) {
        qreal delta = std::max(std::abs(minValue) * 0.1, 1.0);
        minValue -= delta;
        maxValue += delta;
    } else {
        const qreal padding = (maxValue - minValue) * 0.05;
        minValue -= padding;
        maxValue += padding;
    }
    m_yMin = minValue;
    m_yMax = maxValue;
}

void PageDiagramItem::ensureValidRanges() {
    if (!std::isfinite(m_xMin) ||
        !std::isfinite(m_xMax) ||
        m_xMin >= m_xMax) {
        m_xMin = -1.0;
        m_xMax = 1.0;
    }
    if (!std::isfinite(m_yMin) ||
        !std::isfinite(m_yMax) ||
        m_yMin >= m_yMax) {
        m_yMin = -1.0;
        m_yMax = 1.0;
    }
}

// ============================================================
// Plot rectangle
QRectF PageDiagramItem::plotRect() const {
    return QRectF(m_leftMargin, m_topMargin, m_size.width() - m_leftMargin - m_rightMargin,
                                             m_size.height() - m_topMargin - m_bottomMargin);
}

// ============================================================
// Data -> graphics coordinate
QPointF PageDiagramItem::dataToScene(qreal x, qreal y) const {
    const QRectF rect = plotRect();
    
    const qreal xRatio = (x - m_xMin) / (m_xMax - m_xMin);
    const qreal yRatio = (y - m_yMin) / (m_yMax - m_yMin);
    
    const qreal px = rect.left() + xRatio * rect.width();
    
    /*
     * Graphics coordinates have Y pointing down,
     * mathematical coordinates have Y pointing up.
     */
    const qreal py = rect.bottom() - yRatio * rect.height();
    
    return QPointF(px, py);
}

// ============================================================
// Nice tick calculation
qreal PageDiagramItem::niceNumber(qreal value, bool round) const
{
    if (value == 0.0) return 0.0;
    
    const qreal exponent = std::floor(std::log10(std::abs(value)));
    const qreal fraction = std::abs(value) / std::pow(10.0, exponent);
    
    qreal niceFraction;
    
    if (round) {
        if (fraction < 1.5)
            niceFraction = 1.0;
        else if (fraction < 3.0)
            niceFraction = 2.0;
        else if (fraction < 7.0)
            niceFraction = 5.0;
        else
            niceFraction = 10.0;
    } else {
        if (fraction <= 1.0)
            niceFraction = 1.0;
        else if (fraction <= 2.0)
            niceFraction = 2.0;
        else if (fraction <= 5.0)
            niceFraction = 5.0;
        else
            niceFraction = 10.0;
    }
    
    if (value < 0) niceFraction *= -1.0;
    
    return niceFraction * std::pow(10.0, exponent);
}

QVector<qreal> PageDiagramItem::generateTicks(qreal minimum, qreal maximum, int targetCount) const {
    QVector<qreal> ticks;
    
    if (minimum >= maximum) return ticks;
    
    const qreal range = maximum - minimum;
    const qreal spacing = niceNumber(range / std::max(targetCount - 1, 1), true);
    
    if (spacing <= 0.0 || !std::isfinite(spacing)) {
        return ticks;
    }
    
    const qreal first = std::ceil(minimum / spacing) * spacing;
    const qreal last = std::floor(maximum / spacing) * spacing;
    
    for (qreal value = first; value <= last + spacing * 0.001; value += spacing) {
            ticks.append(value);
    }
        
    return ticks;
}

QVector<qreal> PageDiagramItem::generateSubTicks(const QVector<qreal> &ticks, bool isXAxis , int targetCount) const {
    QVector<qreal> subTicks;
    
    qreal minimum = ticks.first();
    qreal maximum = ticks.last();
    
    if (ticks.size() < 2 || minimum >= maximum)
        return subTicks;
    
    // Main tick spacing.
    const qreal spacing = std::abs(ticks[1] - ticks[0]);
    
    if (spacing <= 0.0 || !std::isfinite(spacing))
        return subTicks;
    
    // Determine whether spacing is 1, 2, or 5 × 10^n.
    const qreal exponent = std::floor(std::log10(spacing));
    const qreal magnitude = std::pow(10.0, exponent);
    const qreal fraction = spacing / magnitude;
    
    int divisions;
    
    if (qFuzzyCompare(fraction, 1.0) || qFuzzyCompare(fraction, 5.0)) {
        divisions = 5;
    } else {
        // For the nice-number scheme, this should be 2.
        divisions = 2;
    }
    
    const qreal subSpacing = (ticks[1] - ticks[0]) / divisions;
    // Generate subticks below the main ticks
    qreal subtick = minimum - subSpacing;
    qreal border = isXAxis? m_xMin: m_yMin;
    while (subtick > border) {
        subTicks.append(subtick);
        subtick -= subSpacing;
    }
    // Generate subticks between each pair of main ticks.
    for (int i = 0; i < ticks.size() - 1; ++i) {
        const qreal start = ticks[i];
        
        for (int j = 1; j < divisions; ++j) {
            const qreal value = start + j * subSpacing;
            
            if (value > minimum && value < maximum)
                subTicks.append(value);
        }
    }
    // Generate subticks above the main ticks
    subtick = maximum + subSpacing;
    border = isXAxis? m_xMax: m_yMax;
    while (subtick < border) {
        subTicks.append(subtick);
        subtick += subSpacing;
    }
    
    return subTicks;
}

QString PageDiagramItem::formatXTick(qreal value) const {
    
    qreal epsilon = (m_xMax - m_xMin) * 0.1 * 0.0000001;
    
    if (std::abs(value) < epsilon) return QStringLiteral("0");
    
    const qreal absolute = std::abs(value);
    
    if (absolute >= 1e5 || absolute < 1e-4) {
        return QString::number(value, 'g', 5);
    }
    
    return QString::number(value, 'f', 4).remove(QRegularExpression("0+$")).remove(QRegularExpression("\\.$"));
}
QString PageDiagramItem::formatYTick(qreal value) const {
    
    qreal epsilon = (m_yMax - m_yMin) * 0.1 * 0.0000001;
    
    if (std::abs(value) < epsilon) return QStringLiteral("0");
    
    const qreal absolute = std::abs(value);
    
    if (absolute >= 1e5 || absolute < 1e-4) {
        return QString::number(value, 'g', 5);
    }
    
    return QString::number(value, 'f', 4).remove(QRegularExpression("0+$")).remove(QRegularExpression("\\.$"));
}

// ============================================================
// Drawing
void PageDiagramItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(widget);
    static int count = 0; ++count;
    if (count % 100 == 0) qDebug() << "PageDiagramItem::paint:" << count;
    
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    drawBackground(painter);
    drawGrid(painter);
    drawAxes(painter);
    drawTicks(painter);
    drawCurve(painter);
    drawSelection(painter, option);
    
    painter->restore();
}

void PageDiagramItem::drawBackground(QPainter *painter) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_backgroundColor);
    
    painter->drawRect(boundingRect());
    
    // Plot area background
    painter->setBrush(m_axisBGColor);
    painter->drawRect(plotRect());
}

void PageDiagramItem::drawGrid(QPainter *painter) {
    const QRectF rect = plotRect();
    
    // Draw the outer plot rectangle.
    QPen axisPen(m_axisColor);
    axisPen.setWidthF(1.5);
    painter->setPen(axisPen);
    painter->drawRect(rect);
    
    QPen gridPen(m_gridColor);
    gridPen.setWidthF(m_gridPenWidth);
    gridPen.setStyle(Qt::DashLine);
    painter->setPen(gridPen);
    
    const QVector<qreal> xTicks = generateTicks(m_xMin, m_xMax);
    const QVector<qreal> yTicks = generateTicks(m_yMin, m_yMax);
    const QVector<qreal> xSubTicks = generateSubTicks(xTicks, true);
    const QVector<qreal> ySubTicks = generateSubTicks(yTicks, false);
    
    for (qreal x : xTicks) {
        const QPointF p = dataToScene(x, m_yMin);
        painter->drawLine(QPointF(p.x(), rect.top()), QPointF(p.x(), rect.bottom()));
    }
    for (qreal y : yTicks) {
        const QPointF p = dataToScene(m_xMin, y);
        painter->drawLine(QPointF(rect.left(), p.y()), QPointF(rect.right(), p.y()));
    }
    
    gridPen.setStyle(Qt::DotLine);
    gridPen.setColor(m_subGridColor);
    painter->setPen(gridPen);
    for (qreal x : xSubTicks) {
        const QPointF p = dataToScene(x, m_yMin);
        painter->drawLine(QPointF(p.x(), rect.top()), QPointF(p.x(), rect.bottom()));
    }
    for (qreal y : ySubTicks) {
        const QPointF p = dataToScene(m_xMin, y);
        painter->drawLine(QPointF(rect.left(), p.y()), QPointF(rect.right(), p.y()));
    }
}

void PageDiagramItem::drawAxes(QPainter *painter) {
    const QRectF rect = plotRect();
    
    QPen axisPen(m_axisColor);
    axisPen.setWidthF(1.5);
    
    painter->setPen(axisPen);
    
    
    // X=0 axis.
    if (m_xMin <= 0.0 && m_xMax >= 0.0) {
        const QPointF p =
        dataToScene(0.0, m_yMin);
        
        painter->drawLine(QPointF(p.x(), rect.top()), QPointF(p.x(), rect.bottom()));
    }
    
    // Y=0 axis.
    if (m_yMin <= 0.0 && m_yMax >= 0.0) {
        const QPointF p =
        dataToScene(m_xMin, 0.0);
        
        painter->drawLine(QPointF(rect.left(), p.y()), QPointF(rect.right(), p.y()));
    }
}

void PageDiagramItem::drawTicks(QPainter *painter) {
    const QRectF rect = plotRect();
    
    painter->setFont(m_axisFont);
    
    QPen pen(m_axisColor);
    pen.setWidthF(1.0);
    
    painter->setPen(pen);
    
    const QVector<qreal> xTicks = generateTicks(m_xMin, m_xMax);
    const QVector<qreal> yTicks = generateTicks(m_yMin, m_yMax);
    
    // --------------------------------------------------------
    // X ticks
    for (qreal x : xTicks) {
        const QPointF p = dataToScene(x, m_yMin);
        painter->drawLine(QPointF(p.x(), rect.bottom()), QPointF(p.x(), rect.bottom() + 5));
        qreal ticTextWidth = axisTextWidth(formatXTick(x));
        QRectF textRect(p.x() - 0.5*ticTextWidth, rect.bottom() + 6, ticTextWidth, m_axisFm.height());
        painter->drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, formatXTick(x));
    }
    
    // --------------------------------------------------------
    // Y ticks
    for (qreal y : yTicks) {
        const QPointF p = dataToScene(m_xMin, y);
        painter->drawLine(QPointF(rect.left() - 5, p.y()), QPointF(rect.left(), p.y()));
        QRectF textRect(m_yEdit->height() + m_axisFm.horizontalAdvance(" "),
                        p.y() - 0.5*m_axisFm.height(),
                        m_yAxisTicsWidth,
                        m_axisFm.height());
        //painter->drawRect(textRect);
        painter->drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, formatYTick(y));
    }
}

qreal PageDiagramItem::axisTextWidth(const QString &text) {
    // 1. Create a layout for the text with the exact font you will draw with.
    QTextLayout layout(text, m_axisFont);
    layout.beginLayout();
    QTextLine line = layout.createLine();   // single line – we do not support multiline yet
    line.setLineWidth(1e9);                // huge width so the line never wraps
    layout.endLayout();
    // 3. The natural width of the line is the visual width we need.
    return line.naturalTextWidth();         // already includes over‑hangs & kerning
}

void PageDiagramItem::updateMargins() {
    const QVector<qreal> xTicks = generateTicks(m_xMin, m_xMax);
    const QVector<qreal> yTicks = generateTicks(m_yMin, m_yMax);
    
    QStringList tmpList;
    m_yAxisTicsWidth = 0.0;
    for (qreal y : yTicks) {
        qreal w = axisTextWidth(formatYTick(y));
        tmpList.append(formatYTick(y));
        if (w > m_yAxisTicsWidth) m_yAxisTicsWidth = w; 
    }
    m_leftMargin = m_yEdit->height() + m_axisFm.horizontalAdvance(" ") + m_yAxisTicsWidth + 10;
    m_rightMargin = 20.0;
    m_topMargin = m_titleFm.height() + 10;
    m_bottomMargin = 1.0*m_xEdit->getBoundingRectangle().height() + m_axisFm.height() + 6;
    
    m_xEdit->setPos(plotRect().center().x() - 0.5*m_xEdit->width(),
                    m_size.height() - m_xEdit->bottom() - 2);
    
    m_yEdit->setPos(-m_yEdit->y(),
                    m_topMargin + 0.5*plotRect().height() + 0.5*m_yEdit->width());
    m_yEdit->setRotation(270);
    
    m_titleEdit->resize(m_size.width() - m_leftMargin - m_rightMargin, m_titleFm.height());
    m_titleEditProxy->setPos(m_leftMargin, 5.0);
    
}

void PageDiagramItem::drawCurve(QPainter *painter) {
    PlotData data = getPlotData();
    
    if (!data.valid) return;
    
    QPainterPath path;
    bool pathStarted = false;
    
    const int count = std::min(data.x.size(), data.y.size());
    
    QPen curvePen(m_curveColor);
    curvePen.setWidthF(m_curvePenWidth);
    curvePen.setStyle(Qt::SolidLine);
    
    painter->setPen(curvePen);
    painter->setBrush(Qt::NoBrush);
    
    const QRectF rect = plotRect();
    
    for (int i = 0; i < count; ++i)
    {
        const qreal x = data.x[i];
        const qreal y = data.y[i];
        
        if (!std::isfinite(x) || !std::isfinite(y)) {
            pathStarted = false;
            continue;
        }
        
        /*
         * Ignore points outside the numerical range.
         *
         * The clipping rectangle below also protects the
         * graphics item from drawing outside the plot area.
         */
        const QPointF point = dataToScene(x, y);
        
        if (!pathStarted) {
            path.moveTo(point);
            pathStarted = true;
        } else {
            path.lineTo(point);
        }
    }
    
    painter->save();
    painter->setClipRect(rect);
    painter->drawPath(path);
    painter->restore();
}

void PageDiagramItem::drawSelection(QPainter *painter, const QStyleOptionGraphicsItem *option) {
    if (!(option->state & QStyle::State_Selected)) return;
    
    QPen selectionPen(Qt::blue, 1.0, Qt::DashLine);
    
    painter->setPen(selectionPen);
    painter->setBrush(Qt::NoBrush);
    
    painter->drawRect(boundingRect().adjusted(1, 1, -1, -1));
}

// ============================================================
// Mouse handling
void PageDiagramItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (!(event->modifiers() & Qt::ControlModifier))
            scene()->clearSelection();
        
        setSelected(true);
        setFocus();
        
        emit itemSelected();
    }
    QGraphicsObject::mousePressEvent(event);
}

QVariant PageDiagramItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionHasChanged || change == ItemTransformHasChanged) {
        emit itemDataChanged();
    }
    return QGraphicsObject::itemChange(change, value);
}


// ============================================================
// JSON
QJsonObject PageDiagramItem::toJson() const {
    QJsonObject object;
    
    object["type"] = static_cast<int>(type());
    
    object["x"] = pos().x();
    object["y"] = pos().y();
    
    object["width"] = m_size.width();
    object["height"] = m_size.height();
    
    object["xEdit"] = m_xEdit->toJson();
    object["yEdit"] = m_yEdit->toJson();
    object["title"] = title();
    
    object["autoScaleX"] = m_autoScaleX;
    object["autoScaleY"] = m_autoScaleY;
    object["xMin"] = m_xMin;
    object["xMax"] = m_xMax;
    object["yMin"] = m_yMin;
    object["yMax"] = m_yMax;
    
    return object;
}


PageDiagramItem *PageDiagramItem::fromJson( const QJsonObject &object, Data *data, QGraphicsItem *parent) {
    PageDiagramItem *item = new PageDiagramItem(data, parent);
    
    item->setPos(QPointF(object["x"].toDouble(), object["y"].toDouble()));
    
    if (object.contains("width") && object.contains("height")) {
        item->setDiagramSize(QSizeF(object["width"].toDouble(), object["height"].toDouble()));
    }
    item->setXEdit(object["xEdit"].toObject());
    item->setYEdit(object["yEdit"].toObject());
    item->setTitle(object["title"].toString());
    
    if (object.contains("xMin") && object.contains("xMax")) {
        item->m_xMin = object["xMin"].toDouble();
        item->m_xMax = object["xMax"].toDouble();
    }
    if (object.contains("yMin") && object.contains("yMax")) {
        item->m_yMin = object["yMin"].toDouble();
        item->m_yMax = object["yMax"].toDouble();
    }
    item->m_autoScaleX = object["autoScaleX"].toBool(true);
    item->m_autoScaleY = object["autoScaleY"].toBool(true);
    
    item->refresh();
    return item;
}

void PageDiagramItem::onChildFocusIn() {
    
}

void PageDiagramItem::onChildFocusOut() {
    editingFinished();
}

// Slot to be connected to child signals to trigger a layout update
void PageDiagramItem::updateLayout() {
    updateMargins();
}
