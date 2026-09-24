#pragma once

#include <QGraphicsObject>
#include <QImage>
#include <QJsonObject>
#include <QPointF>
#include <QRectF>
#include <QString>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
class QGraphicsSceneMouseEvent;

class PageImageItem : public QGraphicsObject
{
    Q_OBJECT
    
public:
    enum { Type = UserType + 5 }; // unique per subclass
    int type() const override { return Type; }
    
    explicit PageImageItem(QGraphicsItem *parent = nullptr);
    explicit PageImageItem(const QString &fileName,
                           QGraphicsItem *parent = nullptr);
    
    ~PageImageItem() override = default;
    
    // Image
    bool loadImage(const QString &fileName);
    void setImage(const QImage &image);
    const QImage &image() const;
    
    // JSON serialization
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &object);
    
    // QGraphicsItem
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    
    static PageImageItem *createFromFileDialog(QWidget *parent, QGraphicsItem *graphicsParent = nullptr);
    
signals:
    void itemDataChanged();
    void itemSelected();
    
protected:
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;
    
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    
private:
    enum class Handle {
        None,
        TopLeft,
        TopRight,
        BottomRight,
        BottomLeft,
        Rotate
    };
    
    QRectF imageRect() const;
    
    void paintHandles(QPainter *painter);
    
    Handle handleAt(const QPointF &pos) const;
    QRectF handleRect(Handle handle) const;
    
    void beginResize(Handle handle, const QPointF &scenePos);
    void updateResize(const QPointF &scenePos);
    
    void beginRotate(const QPointF &scenePos);
    void updateRotate(const QPointF &scenePos);
    
    void finishInteraction();
    
    QPointF handlePosition(Handle handle) const;
    
    double angleFromCenter(const QPointF &scenePos) const;
    
    static constexpr qreal HandleSize = 20.0;
    static constexpr qreal RotateHandleDistance = 10.0;
    static constexpr qreal MinimumSize = 100.0;
    
private:
    QImage m_image;
    
    qreal m_width = 100.0;
    qreal m_height = 100.0;
    
    Handle m_activeHandle = Handle::None;
    
    QPointF m_pressScenePos;
    
    // Resize state
    QRectF m_originalRect;
    QPointF m_originalScenePos;
    QPointF m_originalItemPos;
    QPointF m_originalFixedScenePos;
    
    // Rotation state
    qreal m_originalRotation = 0.0;
    qreal m_pressAngle = 0.0;
    
    // Frame visibility state
    bool m_isFrameVisible = false;
};
