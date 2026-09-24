#include "pageimageitem.h"

#include <QBuffer>
#include <QByteArray>
#include <QImageReader>
#include <QJsonValue>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QtMath>
#include <QFileDialog>
#include <cmath>

PageImageItem::PageImageItem(QGraphicsItem *parent)
: QGraphicsObject(parent)
{
    setFlags(QGraphicsItem::ItemIsSelectable |
    QGraphicsItem::ItemIsMovable |
    QGraphicsItem::ItemSendsGeometryChanges);
    
    setAcceptHoverEvents(true);
    
    setTransformOriginPoint(QPointF(m_width / 2.0, m_height / 2.0));
}

PageImageItem::PageImageItem(const QString &fileName,
                             QGraphicsItem *parent)
: PageImageItem(parent)
{
    loadImage(fileName);
}

bool PageImageItem::loadImage(const QString &fileName)
{
    QImageReader reader(fileName);
    
    // Let Qt select the appropriate image plugin.
    reader.setAutoTransform(true);
    
    QImage image = reader.read();
    
    if (image.isNull())
        return false;
    
    setImage(image);
    return true;
}

PageImageItem *PageImageItem::createFromFileDialog(QWidget *parent, QGraphicsItem *graphicsParent)
{
    QStringList extensions;
    
    for (const QByteArray &format: QImageReader::supportedImageFormats()) {
        extensions << "*." +
        QString::fromLatin1(format);
    }
    
    const QString filter = QObject::tr("Images (%1);;All Files (*)").arg(extensions.join(' '));
    const QString fileName = QFileDialog::getOpenFileName(parent, QObject::tr("Select Image"), QString(), filter);
    
    if (fileName.isEmpty())
        return nullptr;
    
    PageImageItem *item = new PageImageItem(graphicsParent);
    
    if (!item->loadImage(fileName)) {
        delete item;
        return nullptr;
    }
    
    return item;
}

void PageImageItem::setImage(const QImage &image)
{
    if (image.isNull())
        return;
    
    prepareGeometryChange();
    
    m_image = image;
    
    // Start with a reasonable display size while preserving
    // the original image aspect ratio.
    const qreal maxSize = 400.0;
    
    const qreal iw = image.width();
    const qreal ih = image.height();
    
    if (iw > 0.0 && ih > 0.0)
    {
        const qreal scale =
        qMin(maxSize / iw, maxSize / ih);
        
        m_width = qMax(MinimumSize, iw * scale);
        m_height = qMax(MinimumSize, ih * scale);
        setTransformOriginPoint(imageRect().center());
    }
    
    update();
}

const QImage &PageImageItem::image() const
{
    return m_image;
}

QRectF PageImageItem::imageRect() const
{
    return QRectF(0.0, 0.0, m_width, m_height);
}

QRectF PageImageItem::boundingRect() const
{
    // Include the rotation handle in the bounding rectangle.
    return QRectF(-HandleSize,
                  -RotateHandleDistance - HandleSize,
                  m_width + 2.0 * HandleSize,
                  m_height + 2.0 * HandleSize);
}

QPainterPath PageImageItem::shape() const
{
    QPainterPath path;
    path.addRect(imageRect());
    return path;
}

void PageImageItem::paint(QPainter *painter,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    const QRectF rect = imageRect();
    
    if (!m_image.isNull()) {
        painter->drawImage(rect, m_image);
    }
    else {
        painter->fillRect(rect, Qt::lightGray);
    }
    
    // Frame
    QPen framePen;
    if (isSelected()) {
        if (m_isFrameVisible)
            framePen.setWidthF(2.0);
        else
            framePen.setWidthF(1.0);
        framePen.setColor(QColor(40, 120, 220));
    } else if (!isSelected() && m_isFrameVisible) {
        framePen.setWidthF(1.0);
        framePen.setColor(QColor(100, 100, 100));
    } else {
        framePen.setWidthF(0.0);
        framePen.setColor(QColor(255, 255, 255));
    }
    if (isSelected() || m_isFrameVisible) {
        painter->setPen(framePen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);
    }
    
    if (isSelected())
        paintHandles(painter);
}

void PageImageItem::paintHandles(QPainter *painter)
{
    painter->save();
    
    painter->setRenderHint(QPainter::Antialiasing, true);
    
    QPen pen(QColor(40, 120, 220));
    pen.setWidthF(1.0);
    
    painter->setPen(pen);
    painter->setBrush(Qt::white);
    
    const Handle handles[] =
    {
        Handle::TopLeft,
        Handle::TopRight,
        Handle::BottomRight,
        Handle::BottomLeft,
    };
    
    for (Handle handle : handles) {
        painter->drawRect(handleRect(handle));
    }
    
    const QPointF rotateCenter = handlePosition(Handle::Rotate);
    
    // Rotation handle
    painter->setBrush(Qt::white);
    painter->drawEllipse(
        rotateCenter,
        HandleSize / 2.0,
        HandleSize / 2.0);
    
    painter->restore();
}

QPointF PageImageItem::handlePosition(Handle handle) const
{
    const QRectF r = imageRect();
    qreal half = HandleSize / 2.0;
    
    switch (handle)
    {
        case Handle::TopLeft:
            return r.topLeft() + QPointF(half, half);
            
        case Handle::TopRight:
            return r.topRight() + QPointF(-half, half);
            
        case Handle::BottomRight:
            return r.bottomRight() + QPointF(-half, -half);
            
        case Handle::BottomLeft:
            return r.bottomLeft() + QPointF(half, -half);
            
        case Handle::Rotate:
            return QPointF(r.center().x(), r.top() + half);
            
        default:
            return QPointF();
    }
}

QRectF PageImageItem::handleRect(Handle handle) const
{
    const QPointF center = handlePosition(handle);
    
    return QRectF(center.x() - HandleSize / 2.0,
                  center.y() - HandleSize / 2.0,
                  HandleSize,
                  HandleSize);
}

PageImageItem::Handle
PageImageItem::handleAt(const QPointF &pos) const
{
    const Handle handles[] =
    {
        Handle::TopLeft,
        Handle::TopRight,
        Handle::BottomRight,
        Handle::BottomLeft,
        Handle::Rotate
    };
    
    // Convert the desired screen-space grab size into item coordinates.
    const QPointF sceneOrigin = mapToScene(QPointF(0, 0));
    const QPointF sceneX = mapToScene(QPointF(1, 0));
    
    const qreal pixelsPerItemUnit = QLineF(sceneOrigin, sceneX).length();
    
    if (pixelsPerItemUnit <= 0.0)
        return Handle::None;
    
    const qreal grabSize = HandleSize / pixelsPerItemUnit;
    
    for (Handle handle : handles)
    {
        const QPointF center = handlePosition(handle);
        
        const QRectF hitRect(center.x() - grabSize / 2.0,
                             center.y() - grabSize / 2.0,
                             grabSize,
                             grabSize);
        
        if (hitRect.contains(pos))
            return handle;
    }
    
    return Handle::None;
}

void PageImageItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    const QPointF pos = event->pos();
    
    m_activeHandle = handleAt(pos);
    m_pressScenePos = event->scenePos();
    
    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        m_isFrameVisible = !m_isFrameVisible;
        update();
        event->accept();
        return;
    } else {
        if (m_activeHandle == Handle::Rotate) {
            beginRotate(event->scenePos());
            event->accept();
            return;
        }
        
        if (m_activeHandle != Handle::None) {
            beginResize(m_activeHandle, event->scenePos());
            event->accept();
            return;
        }
    }
    
    emit itemSelected();
    
    QGraphicsObject::mousePressEvent(event);
}

void PageImageItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_activeHandle == Handle::Rotate) {
        updateRotate(event->scenePos());
        event->accept();
        return;
    }
    
    if (m_activeHandle != Handle::None) {
        updateResize(event->scenePos());
        event->accept();
        return;
    }
    
    QGraphicsObject::mouseMoveEvent(event);
}

void PageImageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_activeHandle != Handle::None) {
        finishInteraction();
        event->accept();
        return;
    }
    
    emit itemDataChanged();
    
    QGraphicsObject::mouseReleaseEvent(event);
}

void PageImageItem::beginResize(Handle handle, const QPointF &scenePos)
{
    m_activeHandle = handle;
    
    m_originalRect = imageRect();
    m_originalScenePos = scenePos;
    
    switch (handle)
    {
        case Handle::TopLeft:
            m_originalFixedScenePos = mapToScene(m_originalRect.bottomRight());
            break;
            
        case Handle::TopRight:
            m_originalFixedScenePos = mapToScene(m_originalRect.bottomLeft());
            break;
            
        case Handle::BottomRight:
            m_originalFixedScenePos = mapToScene(m_originalRect.topLeft());
            break;
            
        case Handle::BottomLeft:
            m_originalFixedScenePos = mapToScene(m_originalRect.topRight());
            break;
            
        default:
            break;
    }
}


void PageImageItem::updateResize(const QPointF &scenePos)
{
    if (m_activeHandle == Handle::None ||
        m_originalRect.width() <= 0.0 ||
        m_originalRect.height() <= 0.0)
        return;
    
    // Calculate mouse movement in scene coordinates.
    const QPointF sceneDelta = scenePos - m_originalScenePos;
    
    // Convert the mouse movement into the item's local
    //coordinate system, taking the item's rotation into account.
    const qreal radians = qDegreesToRadians(-rotation());
    
    const QPointF delta(sceneDelta.x() * qCos(radians) - sceneDelta.y() * qSin(radians),
                        sceneDelta.x() * qSin(radians) + sceneDelta.y() * qCos(radians));
    
    qreal newWidth = m_originalRect.width();
    qreal newHeight = m_originalRect.height();
    
    const qreal aspect = m_originalRect.width() / m_originalRect.height();
    
    // Calculate the new size.
    switch (m_activeHandle)
    {
        case Handle::TopLeft:
        case Handle::TopRight:
        case Handle::BottomLeft:
        case Handle::BottomRight:
        {
            // Preserve the original aspect ratio.
            // Use whichever dimension has changed more.
            const qreal widthChange = qAbs(delta.x());
            const qreal heightChange = qAbs(delta.y());
            
            if (widthChange >= heightChange) {
                newWidth = qMax(MinimumSize, m_originalRect.width() +
                     ((m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::BottomLeft)? -delta.x(): delta.x()));
                newHeight = newWidth / aspect;
            } else {
                newHeight = qMax(MinimumSize, m_originalRect.height() +
                     ((m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::TopRight)? -delta.y(): delta.y()));
                newWidth = newHeight * aspect;
            }
            break;
        }
        default:
            return;
    }
    
    // Change the geometry.
    prepareGeometryChange();
    
    m_width = newWidth;
    m_height = newHeight;
    setTransformOriginPoint(imageRect().center());
    
    // Determine which corner is the fixed corner in the
    // NEW local geometry.
    QPointF newFixedLocalPos;
    
    switch (m_activeHandle)
    {
        case Handle::TopLeft:
            newFixedLocalPos = QPointF(newWidth, newHeight);
            break;
            
        case Handle::TopRight:
            newFixedLocalPos = QPointF(0.0, newHeight);
            break;
            
        case Handle::BottomRight:
            newFixedLocalPos = QPointF(0.0, 0.0);
            break;
            
        case Handle::BottomLeft:
            newFixedLocalPos = QPointF(newWidth, 0.0);
            break;
            
        default:
            return;
    }
    
    // Find where the fixed corner currently is in scene coordinates after changing the geometry.
    const QPointF newFixedScenePos = mapToScene(newFixedLocalPos);
    
    // The original fixed corner was stored in beginResize().
    // Move the item so that the new position of that corner exactly matches its original scene position.
    const QPointF sceneCorrection = m_originalFixedScenePos - newFixedScenePos;
    
    // setPos() operates in parent coordinates, so convert the scene-space correction into parent-space correction.
    QPointF parentCorrection;
    
    if (QGraphicsItem *parent = parentItem()) {
        const QPointF oldParentPoint = parent->mapFromScene(newFixedScenePos);
        const QPointF targetParentPoint = parent->mapFromScene(m_originalFixedScenePos);
        parentCorrection = targetParentPoint - oldParentPoint;
    } else {
        parentCorrection = sceneCorrection;
    }
    
    // Apply the correction.
    setPos(pos() + parentCorrection);
    
    emit itemDataChanged();
    update();
}


void PageImageItem::beginRotate(const QPointF &scenePos)
{
    m_originalScenePos = scenePos;
    m_originalRotation = rotation();
    
    const QPointF centerScenePos = mapToScene(imageRect().center());
    
    m_pressAngle = std::atan2(scenePos.y() - centerScenePos.y(),
                              scenePos.x() - centerScenePos.x());
}

void PageImageItem::updateRotate(const QPointF &scenePos)
{
    const QPointF centerScenePos = mapToScene(imageRect().center());
    
    const qreal currentAngle = std::atan2(scenePos.y() - centerScenePos.y(),
                                          scenePos.x() - centerScenePos.x());
    
    const qreal deltaAngle =
    qRadiansToDegrees(currentAngle - m_pressAngle);
    
    // Snap to 5 degree increments.
    const qreal snappedRotation =
    qRound((m_originalRotation + deltaAngle) / 5.0) * 5.0;
    
    setRotation(snappedRotation);
    
    emit itemDataChanged();
    update();
}

qreal PageImageItem::angleFromCenter(const QPointF &scenePos) const
{
    const QPointF center = mapToScene(imageRect().center());
    const QPointF vector = scenePos - center;
    
    return qRadiansToDegrees(qAtan2(vector.y(), vector.x()));
}

void PageImageItem::finishInteraction()
{
    m_activeHandle = Handle::None;
    m_pressScenePos = QPointF();
    m_originalScenePos = QPointF();
    
    emit itemDataChanged();
    update();
}

QJsonObject PageImageItem::toJson() const
{
    QJsonObject object;
    
    object["type"] = static_cast<int>(type());
    object["version"] = 1;
    
    // Geometry
    object["x"] = pos().x();
    object["y"] = pos().y();
    
    object["width"] = m_width;
    object["height"] = m_height;
    
    object["rotation"] = rotation();
    
    // Image
    if (!m_image.isNull())
    {
        QByteArray imageData;
        
        QBuffer buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        
        // PNG is used for serialization because it is lossless
        // and universally supported by Qt.
        m_image.save(&buffer, "PNG");
        
        object["imageFormat"] = "png";
        object["image"] =
        QString::fromLatin1(imageData.toBase64());
    }
    
    return object;
}

bool PageImageItem::fromJson(const QJsonObject &object)
{
    if (object["type"].toInt() != static_cast<int>(type()))
        return false;
    
    prepareGeometryChange();
    
    m_width = qMax(MinimumSize, object["width"].toDouble(100.0));
    m_height = qMax(MinimumSize, object["height"].toDouble(100.0));
    
    setPos(QPointF(object["x"].toDouble(),
                   object["y"].toDouble()));
    
    setRotation(object["rotation"].toDouble());
    
    setTransformOriginPoint(imageRect().center());
    
    const QString encodedImage = object["image"].toString();
    
    if (!encodedImage.isEmpty())
    {
        const QByteArray imageData =
        QByteArray::fromBase64(
            encodedImage.toLatin1());
        
        QImage image;
        
        if (!image.loadFromData(imageData))
            return false;
        
        m_image = image;
    }
    
    emit itemDataChanged();
    update();
    
    return true;
}
