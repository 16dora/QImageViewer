#include "img_graphics_view.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include <cmath>

namespace image_viewer {

// 创建图片交互视图并初始化场景与交互行为。
ImgGraphicsView::ImgGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
    , m_scenePtr(new QGraphicsScene(this))
    , m_pixmapItemPtr(nullptr)
    , m_roiItemPtr(nullptr)
    , m_drawingPreviewItemPtr(nullptr)
    , m_pickItemPtr(nullptr)
    , m_selectedDrawingItemPtr(nullptr)
    , m_toolMode(ToolMode::Roi)
    , m_activeDrawingMode(ToolMode::Roi)
    , m_isDrawing(false)
    , m_hasExceededDragThreshold(false)
    , m_isPanning(false)
{
    setScene(m_scenePtr);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setFocusPolicy(Qt::StrongFocus);
}

// 替换当前图片并重置由场景管理的全部覆盖图形。
void ImgGraphicsView::setImage(
    const QPixmap& pixmap,
    const QTransform& currentToOriginalTransform,
    const QSize& originalImageSize)
{
    m_scenePtr->clear();
    m_pixmapItemPtr = m_scenePtr->addPixmap(pixmap);
    m_scenePtr->setSceneRect(m_pixmapItemPtr->sceneBoundingRect());
    m_roiItemPtr = nullptr;
    m_drawingPreviewItemPtr = nullptr;
    m_pickItemPtr = nullptr;
    m_selectedDrawingItemPtr = nullptr;
    m_drawingItems.clear();
    m_currentToOriginalTransform = currentToOriginalTransform;
    m_originalImageSize = originalImageSize;
    m_isDrawing = false;
    m_hasExceededDragThreshold = false;
    m_isPanning = false;
    emit pixelPickCleared();
    fitImageInView();
}

// 切换互斥工具并取消旧工具尚未完成的预览与选择。
void ImgGraphicsView::setToolMode(ToolMode toolMode)
{
    if (m_toolMode == toolMode)
    {
        return;
    }

    cancelDrawing();
    clearDrawingSelection();
    m_toolMode = toolMode;
}

// 根据滚轮方向等比例缩放图片并同步覆盖图形与倍率。
void ImgGraphicsView::wheelEvent(QWheelEvent* eventPtr)
{
    if (m_pixmapItemPtr == nullptr)
    {
        return;
    }

    if (eventPtr->angleDelta().y() > 0)
    {
        scale(ZOOM_STEP_FACTOR, ZOOM_STEP_FACTOR);
    }
    else
    {
        scale(1.0 / ZOOM_STEP_FACTOR, 1.0 / ZOOM_STEP_FACTOR);
    }

    updateOverlayAppearance();
    viewport()->update();
    emitCurrentZoom();
}

// 将当前图片按原图比例适配到视图。
void ImgGraphicsView::fitImageInView()
{
    if (m_pixmapItemPtr == nullptr)
    {
        return;
    }

    fitInView(m_pixmapItemPtr, Qt::KeepAspectRatio);
    updateOverlayAppearance();
    emitCurrentZoom();
}

// 读取视图变换中的缩放分量并发送实际倍率。
void ImgGraphicsView::emitCurrentZoom()
{
    emit zoomChanged(currentViewScale());
}

// 返回当前视图变换中的统一缩放倍率。
qreal ImgGraphicsView::currentViewScale() const
{
    return qMax(std::abs(transform().m11()), 1.0e-9);
}

// 按分段规则计算覆盖图形在场景坐标中的宽度。
qreal ImgGraphicsView::calculateOverlaySceneWidth() const
{
    const qreal scaleFactor = currentViewScale();
    if (scaleFactor < 1.0)
    {
        return OVERLAY_PEN_WIDTH_VIEW_PIXELS / scaleFactor;
    }

    return OVERLAY_PEN_WIDTH_IMAGE_PIXELS;
}

// 使用当前倍率创建不改变图形颜色的覆盖画笔。
QPen ImgGraphicsView::createOverlayPen(const QColor& color) const
{
    return QPen(color, calculateOverlaySceneWidth());
}

// 缩放后同步ROI、普通图形、实时预览和Pick标记外观。
void ImgGraphicsView::updateOverlayAppearance()
{
    if (m_roiItemPtr != nullptr)
    {
        m_roiItemPtr->setPen(createOverlayPen(Qt::red));
    }

    if (m_drawingPreviewItemPtr != nullptr)
    {
        if (m_activeDrawingMode == ToolMode::Line)
        {
            static_cast<QGraphicsLineItem*>(m_drawingPreviewItemPtr)
                ->setPen(createOverlayPen(Qt::blue));
        }
        else if (m_activeDrawingMode == ToolMode::Circle)
        {
            static_cast<QGraphicsEllipseItem*>(m_drawingPreviewItemPtr)
                ->setPen(createOverlayPen(Qt::blue));
        }
        else
        {
            const QColor previewColor =
                m_activeDrawingMode == ToolMode::Roi ? Qt::red : Qt::blue;
            static_cast<QGraphicsRectItem*>(m_drawingPreviewItemPtr)
                ->setPen(createOverlayPen(previewColor));
        }
    }

    for (const DrawingItemRecord& drawingItem : m_drawingItems)
    {
        if (drawingItem.toolMode == ToolMode::Line)
        {
            static_cast<QGraphicsLineItem*>(drawingItem.itemPtr)
                ->setPen(createOverlayPen(Qt::blue));
        }
        else if (drawingItem.toolMode == ToolMode::Circle)
        {
            static_cast<QGraphicsEllipseItem*>(drawingItem.itemPtr)
                ->setPen(createOverlayPen(Qt::blue));
        }
        else
        {
            static_cast<QGraphicsRectItem*>(drawingItem.itemPtr)
                ->setPen(createOverlayPen(Qt::blue));
        }
    }

    if (m_pickItemPtr != nullptr)
    {
        const qreal markerSize = calculateOverlaySceneWidth();
        m_pickItemPtr->setRect(
            m_pickSceneCenter.x() - markerSize / 2.0,
            m_pickSceneCenter.y() - markerSize / 2.0,
            markerSize,
            markerSize);
    }
}

// 将场景坐标限制在当前图片外接矩形范围内。
QPointF ImgGraphicsView::constrainScenePositionToImage(
    const QPointF& scenePosition) const
{
    if (m_pixmapItemPtr == nullptr)
    {
        return scenePosition;
    }

    const QRectF imageSceneRect = m_pixmapItemPtr->sceneBoundingRect();
    return QPointF(
        qBound(imageSceneRect.left(), scenePosition.x(), imageSceneRect.right()),
        qBound(imageSceneRect.top(), scenePosition.y(), imageSceneRect.bottom()));
}

// 判断视图坐标映射后是否落在当前图片画布中。
bool ImgGraphicsView::isViewPositionInsideImage(const QPoint& viewPosition) const
{
    return m_pixmapItemPtr != nullptr
           && m_pixmapItemPtr->sceneBoundingRect().contains(
               mapToScene(viewPosition));
}

// 创建当前工具对应的零尺寸预览并记录绘制起点。
void ImgGraphicsView::beginDrawing(
    ToolMode toolMode,
    const QPointF& scenePosition,
    const QPoint& viewPosition)
{
    cancelDrawing();
    m_activeDrawingMode = toolMode;
    m_drawingStartPos = constrainScenePositionToImage(scenePosition);
    m_drawingStartViewPos = viewPosition;
    m_isDrawing = true;
    m_hasExceededDragThreshold = false;

    if (toolMode == ToolMode::Roi)
    {
        m_drawingPreviewItemPtr = m_scenePtr->addRect(
            QRectF(m_drawingStartPos, m_drawingStartPos),
            createOverlayPen(Qt::red));
    }
    else if (toolMode == ToolMode::Line)
    {
        m_drawingPreviewItemPtr = m_scenePtr->addLine(
            QLineF(m_drawingStartPos, m_drawingStartPos),
            createOverlayPen(Qt::blue));
    }
    else if (toolMode == ToolMode::Circle)
    {
        m_drawingPreviewItemPtr = m_scenePtr->addEllipse(
            QRectF(m_drawingStartPos, m_drawingStartPos),
            createOverlayPen(Qt::blue),
            Qt::NoBrush);
    }
    else if (toolMode == ToolMode::Rect)
    {
        m_drawingPreviewItemPtr = m_scenePtr->addRect(
            QRectF(m_drawingStartPos, m_drawingStartPos),
            createOverlayPen(Qt::blue),
            Qt::NoBrush);
    }

    if (m_drawingPreviewItemPtr != nullptr)
    {
        m_drawingPreviewItemPtr->setZValue(2.0);
    }
}

// 根据当前工具几何规则更新实时预览并限制完整图形不越界。
void ImgGraphicsView::updateDrawingPreview(const QPointF& scenePosition)
{
    if (!m_isDrawing || m_drawingPreviewItemPtr == nullptr)
    {
        return;
    }

    const QPointF currentScenePosition =
        constrainScenePositionToImage(scenePosition);
    if (m_activeDrawingMode == ToolMode::Roi)
    {
        static_cast<QGraphicsRectItem*>(m_drawingPreviewItemPtr)
            ->setRect(QRectF(m_drawingStartPos, currentScenePosition).normalized());
        return;
    }

    if (m_activeDrawingMode == ToolMode::Line)
    {
        static_cast<QGraphicsLineItem*>(m_drawingPreviewItemPtr)
            ->setLine(QLineF(m_drawingStartPos, currentScenePosition));
        return;
    }

    const QRectF imageSceneRect = m_pixmapItemPtr->sceneBoundingRect();
    const qreal maximumHalfWidth = qMin(
        m_drawingStartPos.x() - imageSceneRect.left(),
        imageSceneRect.right() - m_drawingStartPos.x());
    const qreal maximumHalfHeight = qMin(
        m_drawingStartPos.y() - imageSceneRect.top(),
        imageSceneRect.bottom() - m_drawingStartPos.y());

    if (m_activeDrawingMode == ToolMode::Circle)
    {
        const qreal requestedRadius =
            QLineF(m_drawingStartPos, currentScenePosition).length();
        const qreal radius = qMin(
            requestedRadius, qMin(maximumHalfWidth, maximumHalfHeight));
        static_cast<QGraphicsEllipseItem*>(m_drawingPreviewItemPtr)->setRect(
            m_drawingStartPos.x() - radius,
            m_drawingStartPos.y() - radius,
            radius * 2.0,
            radius * 2.0);
        return;
    }

    const qreal halfWidth = qMin(
        std::abs(currentScenePosition.x() - m_drawingStartPos.x()),
        maximumHalfWidth);
    const qreal halfHeight = qMin(
        std::abs(currentScenePosition.y() - m_drawingStartPos.y()),
        maximumHalfHeight);
    static_cast<QGraphicsRectItem*>(m_drawingPreviewItemPtr)->setRect(
        m_drawingStartPos.x() - halfWidth,
        m_drawingStartPos.y() - halfHeight,
        halfWidth * 2.0,
        halfHeight * 2.0);
}

// 删除未提交预览并结束当前左键操作。
void ImgGraphicsView::cancelDrawing()
{
    deleteDrawingPreview();
    m_isDrawing = false;
    m_hasExceededDragThreshold = false;
}

// 判断当前预览是否满足对应工具的最小有效几何条件。
bool ImgGraphicsView::isDrawingPreviewValid() const
{
    if (m_drawingPreviewItemPtr == nullptr)
    {
        return false;
    }

    if (m_activeDrawingMode == ToolMode::Line)
    {
        return static_cast<QGraphicsLineItem*>(m_drawingPreviewItemPtr)
                   ->line().length() > 0.0;
    }

    QRectF drawingRect;
    if (m_activeDrawingMode == ToolMode::Circle)
    {
        drawingRect = static_cast<QGraphicsEllipseItem*>(m_drawingPreviewItemPtr)
                          ->rect();
    }
    else
    {
        drawingRect = static_cast<QGraphicsRectItem*>(m_drawingPreviewItemPtr)
                          ->rect();
    }
    if (m_activeDrawingMode == ToolMode::Roi)
    {
        return drawingRect.width() > MIN_ROI_SIZE
               && drawingRect.height() > MIN_ROI_SIZE;
    }

    return drawingRect.width() > 0.0 && drawingRect.height() > 0.0;
}

// 将有效预览提交为ROI或可单选删除的普通绘图对象。
void ImgGraphicsView::commitDrawingPreview()
{
    if (m_drawingPreviewItemPtr == nullptr)
    {
        return;
    }

    if (m_activeDrawingMode == ToolMode::Roi)
    {
        if (m_roiItemPtr != nullptr)
        {
            delete m_roiItemPtr;
        }
        m_roiItemPtr = static_cast<QGraphicsRectItem*>(m_drawingPreviewItemPtr);
        m_drawingPreviewItemPtr = nullptr;
        m_roiItemPtr->setZValue(2.0);
        emit roiSelected(m_roiItemPtr->rect());
        return;
    }

    m_drawingPreviewItemPtr->setFlag(QGraphicsItem::ItemIsSelectable, true);
    m_drawingPreviewItemPtr->setZValue(1.0);
    m_drawingItems.append(
        DrawingItemRecord{m_activeDrawingMode, m_drawingPreviewItemPtr});
    selectOnlyDrawingItem(m_drawingPreviewItemPtr);
    m_drawingPreviewItemPtr = nullptr;
}

// 删除当前实时预览并清空场景观察指针。
void ImgGraphicsView::deleteDrawingPreview()
{
    if (m_drawingPreviewItemPtr == nullptr)
    {
        return;
    }

    delete m_drawingPreviewItemPtr;
    m_drawingPreviewItemPtr = nullptr;
}

// 从最上层开始选择当前工具类型的单个图形。
void ImgGraphicsView::selectDrawingAt(const QPointF& scenePosition)
{
    for (int itemIndex = m_drawingItems.size() - 1; itemIndex >= 0; --itemIndex)
    {
        const DrawingItemRecord& drawingItem = m_drawingItems.at(itemIndex);
        if (drawingItem.toolMode == m_toolMode
            && isDrawingItemHit(drawingItem, scenePosition))
        {
            selectOnlyDrawingItem(drawingItem.itemPtr);
            return;
        }
    }

    clearDrawingSelection();
}

// 使用固定视图像素容差命中线段、圆轮廓或矩形轮廓。
bool ImgGraphicsView::isDrawingItemHit(
    const DrawingItemRecord& drawingItem,
    const QPointF& scenePosition) const
{
    const qreal tolerance =
        SELECTION_HIT_TOLERANCE_VIEW_PIXELS / currentViewScale();
    if (drawingItem.toolMode == ToolMode::Line)
    {
        const QLineF line =
            static_cast<QGraphicsLineItem*>(drawingItem.itemPtr)->line();
        const QPointF lineVector = line.p2() - line.p1();
        const qreal lineLengthSquared =
            lineVector.x() * lineVector.x() + lineVector.y() * lineVector.y();
        if (lineLengthSquared <= 0.0)
        {
            return false;
        }

        const QPointF pointVector = scenePosition - line.p1();
        const qreal projection = qBound(
            0.0,
            (pointVector.x() * lineVector.x()
             + pointVector.y() * lineVector.y()) / lineLengthSquared,
            1.0);
        const QPointF closestPoint = line.p1() + lineVector * projection;
        return QLineF(scenePosition, closestPoint).length() <= tolerance;
    }

    QRectF itemRect;
    if (drawingItem.toolMode == ToolMode::Circle)
    {
        itemRect = static_cast<QGraphicsEllipseItem*>(drawingItem.itemPtr)->rect();
        QPainterPath outerPath;
        outerPath.addEllipse(itemRect.adjusted(
            -tolerance, -tolerance, tolerance, tolerance));
        const QRectF innerRect = itemRect.adjusted(
            tolerance, tolerance, -tolerance, -tolerance);
        if (!outerPath.contains(scenePosition))
        {
            return false;
        }
        if (innerRect.width() <= 0.0 || innerRect.height() <= 0.0)
        {
            return true;
        }

        QPainterPath innerPath;
        innerPath.addEllipse(innerRect);
        return !innerPath.contains(scenePosition);
    }

    itemRect = static_cast<QGraphicsRectItem*>(drawingItem.itemPtr)->rect();
    const QRectF outerRect = itemRect.adjusted(
        -tolerance, -tolerance, tolerance, tolerance);
    const QRectF innerRect = itemRect.adjusted(
        tolerance, tolerance, -tolerance, -tolerance);
    return outerRect.contains(scenePosition)
           && (!innerRect.isValid() || !innerRect.contains(scenePosition));
}

// 清除旧选择并设置唯一普通绘图选中项。
void ImgGraphicsView::selectOnlyDrawingItem(QGraphicsItem* itemPtr)
{
    clearDrawingSelection();
    m_selectedDrawingItemPtr = itemPtr;
    if (m_selectedDrawingItemPtr != nullptr)
    {
        m_selectedDrawingItemPtr->setSelected(true);
    }
}

// 清除当前普通绘图的Qt标准选择轮廓。
void ImgGraphicsView::clearDrawingSelection()
{
    if (m_selectedDrawingItemPtr != nullptr)
    {
        m_selectedDrawingItemPtr->setSelected(false);
        m_selectedDrawingItemPtr = nullptr;
    }
}

// 删除Pick点或当前工具模式下唯一选中的普通绘图。
void ImgGraphicsView::deleteCurrentToolSelection()
{
    if (m_toolMode == ToolMode::Pick)
    {
        clearPickedPixel();
        return;
    }

    if (m_selectedDrawingItemPtr == nullptr)
    {
        return;
    }

    for (int itemIndex = 0; itemIndex < m_drawingItems.size(); ++itemIndex)
    {
        if (m_drawingItems.at(itemIndex).itemPtr == m_selectedDrawingItemPtr
            && m_drawingItems.at(itemIndex).toolMode == m_toolMode)
        {
            QGraphicsItem* deletedItemPtr = m_selectedDrawingItemPtr;
            m_selectedDrawingItemPtr = nullptr;
            m_drawingItems.removeAt(itemIndex);
            delete deletedItemPtr;
            return;
        }
    }
}

// 将当前画布点击位置映射为原图像素并更新唯一蓝色标记。
void ImgGraphicsView::pickPixelAt(const QPointF& scenePosition)
{
    if (m_pixmapItemPtr == nullptr || m_originalImageSize.isEmpty())
    {
        clearPickedPixel();
        return;
    }

    const QPointF originalPosition =
        m_currentToOriginalTransform.map(scenePosition);
    const int originalPixelX = static_cast<int>(std::floor(originalPosition.x()));
    const int originalPixelY = static_cast<int>(std::floor(originalPosition.y()));
    if (originalPixelX < 0
        || originalPixelY < 0
        || originalPixelX >= m_originalImageSize.width()
        || originalPixelY >= m_originalImageSize.height())
    {
        clearPickedPixel();
        return;
    }

    const QPixmap& currentPixmap = m_pixmapItemPtr->pixmap();
    const int currentPixelX = qBound(
        0,
        static_cast<int>(std::floor(scenePosition.x())),
        currentPixmap.width() - 1);
    const int currentPixelY = qBound(
        0,
        static_cast<int>(std::floor(scenePosition.y())),
        currentPixmap.height() - 1);
    m_pickSceneCenter = QPointF(currentPixelX + 0.5, currentPixelY + 0.5);

    if (m_pickItemPtr == nullptr)
    {
        m_pickItemPtr = m_scenePtr->addRect(
            QRectF(), QPen(Qt::NoPen), QBrush(Qt::blue));
        m_pickItemPtr->setZValue(3.0);
    }
    updateOverlayAppearance();
    emit pixelPicked(QPoint(originalPixelX, originalPixelY));
}

// 删除唯一Pick标记并通知窗口恢复无效坐标文本。
void ImgGraphicsView::clearPickedPixel()
{
    if (m_pickItemPtr != nullptr)
    {
        delete m_pickItemPtr;
        m_pickItemPtr = nullptr;
    }
    emit pixelPickCleared();
}

// 根据鼠标按钮启动右键平移或当前左键工具。
void ImgGraphicsView::mousePressEvent(QMouseEvent* eventPtr)
{
    const QPoint currentViewPosition = eventPtr->position().toPoint();
    if (eventPtr->button() == Qt::RightButton)
    {
        if (m_isDrawing)
        {
            cancelDrawing();
            eventPtr->accept();
            return;
        }

        m_isPanning = true;
        m_lastPanPos = currentViewPosition;
        setCursor(Qt::ClosedHandCursor);
        eventPtr->accept();
        return;
    }

    if (eventPtr->button() == Qt::LeftButton
        && isViewPositionInsideImage(currentViewPosition))
    {
        setFocus(Qt::MouseFocusReason);
        const QPointF scenePosition = mapToScene(currentViewPosition);
        beginDrawing(m_toolMode, scenePosition, currentViewPosition);
        eventPtr->accept();
        return;
    }

    QGraphicsView::mousePressEvent(eventPtr);
}

// 根据当前交互状态更新平移位置或实时绘图预览。
void ImgGraphicsView::mouseMoveEvent(QMouseEvent* eventPtr)
{
    const QPoint currentViewPosition = eventPtr->position().toPoint();
    if (m_isPanning)
    {
        const QPoint delta = currentViewPosition - m_lastPanPos;
        m_lastPanPos = currentViewPosition;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        viewport()->update();
        eventPtr->accept();
        return;
    }

    if (m_isDrawing)
    {
        const int dragDistance =
            (currentViewPosition - m_drawingStartViewPos).manhattanLength();
        if (dragDistance >= QApplication::startDragDistance())
        {
            m_hasExceededDragThreshold = true;
        }
        updateDrawingPreview(mapToScene(currentViewPosition));
        eventPtr->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(eventPtr);
}

// 结束平移、提交绘制、执行单选或完成Pick。
void ImgGraphicsView::mouseReleaseEvent(QMouseEvent* eventPtr)
{
    const QPoint currentViewPosition = eventPtr->position().toPoint();
    if (eventPtr->button() == Qt::RightButton)
    {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        eventPtr->accept();
        return;
    }

    if (eventPtr->button() == Qt::LeftButton && m_isDrawing)
    {
        const QPointF releaseScenePosition =
            constrainScenePositionToImage(mapToScene(currentViewPosition));
        const ToolMode completedToolMode = m_activeDrawingMode;
        updateDrawingPreview(releaseScenePosition);

        if (completedToolMode == ToolMode::Pick)
        {
            if (!m_hasExceededDragThreshold)
            {
                pickPixelAt(releaseScenePosition);
            }
            cancelDrawing();
        }
        else if (completedToolMode == ToolMode::Roi)
        {
            if (isDrawingPreviewValid())
            {
                commitDrawingPreview();
            }
            else
            {
                deleteDrawingPreview();
            }
            m_isDrawing = false;
            m_hasExceededDragThreshold = false;
        }
        else
        {
            if (m_hasExceededDragThreshold && isDrawingPreviewValid())
            {
                commitDrawingPreview();
            }
            else
            {
                deleteDrawingPreview();
                if (!m_hasExceededDragThreshold)
                {
                    selectDrawingAt(releaseScenePosition);
                }
            }
            m_isDrawing = false;
            m_hasExceededDragThreshold = false;
        }

        eventPtr->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(eventPtr);
}

// 删除当前工具对应的唯一选中对象。
void ImgGraphicsView::keyPressEvent(QKeyEvent* eventPtr)
{
    if (eventPtr->key() == Qt::Key_Delete)
    {
        deleteCurrentToolSelection();
        eventPtr->accept();
        return;
    }

    QGraphicsView::keyPressEvent(eventPtr);
}

// 判断图片显示区域是否在任一方向超出当前视口。
bool ImgGraphicsView::isNavigationThumbnailVisible() const
{
    if (m_pixmapItemPtr == nullptr || m_pixmapItemPtr->pixmap().isNull())
    {
        return false;
    }

    const QRectF imageViewportRect =
        viewportTransform().mapRect(m_pixmapItemPtr->sceneBoundingRect());
    return imageViewportRect.width() > viewport()->width()
           || imageViewportRect.height() > viewport()->height();
}

// 根据视口尺寸上限和原图比例计算右上角缩略图区域。
QRect ImgGraphicsView::calculateNavigationThumbnailRect() const
{
    const int availableWidth = qMax(
        1,
        qMin(NAVIGATION_THUMBNAIL_MAX_WIDTH,
             static_cast<int>(viewport()->width()
                              * NAVIGATION_THUMBNAIL_VIEWPORT_RATIO)));
    const int availableHeight = qMax(
        1,
        qMin(NAVIGATION_THUMBNAIL_MAX_HEIGHT,
             static_cast<int>(viewport()->height()
                              * NAVIGATION_THUMBNAIL_VIEWPORT_RATIO)));
    const QSize thumbnailSize = m_pixmapItemPtr->pixmap().size().scaled(
        QSize(availableWidth, availableHeight), Qt::KeepAspectRatio);
    return QRect(viewport()->width() - NAVIGATION_THUMBNAIL_MARGIN
                     - thumbnailSize.width(),
                 NAVIGATION_THUMBNAIL_MARGIN,
                 thumbnailSize.width(),
                 thumbnailSize.height());
}

// 将当前视口可见的场景区域映射到缩略导航图坐标。
QRectF ImgGraphicsView::calculateNavigationViewportRect(
    const QRect& thumbnailRect) const
{
    const QRectF imageSceneRect = m_pixmapItemPtr->sceneBoundingRect();
    const QRectF visibleSceneRect =
        mapToScene(viewport()->rect()).boundingRect().intersected(imageSceneRect);
    if (imageSceneRect.isEmpty() || visibleSceneRect.isEmpty())
    {
        return QRectF();
    }

    const qreal thumbnailScaleX =
        static_cast<qreal>(thumbnailRect.width()) / imageSceneRect.width();
    const qreal thumbnailScaleY =
        static_cast<qreal>(thumbnailRect.height()) / imageSceneRect.height();
    return QRectF(
        thumbnailRect.left()
            + (visibleSceneRect.left() - imageSceneRect.left()) * thumbnailScaleX,
        thumbnailRect.top()
            + (visibleSceneRect.top() - imageSceneRect.top()) * thumbnailScaleY,
        visibleSceneRect.width() * thumbnailScaleX,
        visibleSceneRect.height() * thumbnailScaleY);
}

// 绘制完整原图缩略图、深色外边框和黄色视口反馈框。
void ImgGraphicsView::drawNavigationThumbnail(QPainter* painterPtr) const
{
    const QRect thumbnailRect = calculateNavigationThumbnailRect();
    painterPtr->setRenderHint(QPainter::SmoothPixmapTransform);
    painterPtr->drawPixmap(thumbnailRect,
                           m_pixmapItemPtr->pixmap(),
                           m_pixmapItemPtr->pixmap().rect());

    painterPtr->setBrush(Qt::NoBrush);
    painterPtr->setPen(
        QPen(Qt::black, NAVIGATION_THUMBNAIL_BORDER_WIDTH));
    painterPtr->drawRect(thumbnailRect);

    const QRectF navigationViewportRect =
        calculateNavigationViewportRect(thumbnailRect);
    if (!navigationViewportRect.isEmpty())
    {
        painterPtr->setPen(
            QPen(Qt::yellow, NAVIGATION_VIEWPORT_PEN_WIDTH));
        painterPtr->drawRect(navigationViewportRect);
    }
}

// 视图尺寸变化后重新适配图片并同步倍率。
void ImgGraphicsView::resizeEvent(QResizeEvent* eventPtr)
{
    QGraphicsView::resizeEvent(eventPtr);
    fitImageInView();
}

// 完成主场景绘制后，在视口右上角按需绘制缩略导航图。
void ImgGraphicsView::paintEvent(QPaintEvent* eventPtr)
{
    QGraphicsView::paintEvent(eventPtr);
    if (!isNavigationThumbnailVisible())
    {
        return;
    }

    QPainter painter(viewport());
    drawNavigationThumbnail(&painter);
}

} // namespace image_viewer
