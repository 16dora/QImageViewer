#include "img_graphics_view.h"

#include <QMouseEvent>
#include <QPen>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

namespace image_viewer {

// 创建图片交互视图并初始化场景与交互行为。
ImgGraphicsView::ImgGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
    , m_scenePtr(new QGraphicsScene(this))
    , m_pixmapItemPtr(nullptr)
    , m_roiItemPtr(nullptr)
    , m_isDrawingRoi(false)
    , m_isPanning(false)
{
    setScene(m_scenePtr);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

// 替换当前图片并重置由场景管理的图形项。
void ImgGraphicsView::setImage(const QPixmap& pixmap)
{
    m_scenePtr->clear();
    m_pixmapItemPtr = m_scenePtr->addPixmap(pixmap);
    m_roiItemPtr = nullptr;
    fitImageInView();
}

// 根据滚轮方向等比例缩放图片并发送最新倍率。
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
    emitCurrentZoom();
}

// 读取视图变换中的缩放分量并发送实际倍率。
void ImgGraphicsView::emitCurrentZoom()
{
    emit zoomChanged(transform().m11());
}

// 根据鼠标按钮启动右键平移或左键ROI绘制。
void ImgGraphicsView::mousePressEvent(QMouseEvent* eventPtr)
{
    if (eventPtr->button() == Qt::RightButton)
    {
        m_isPanning = true;
        m_lastPanPos = eventPtr->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
    }
    else if (eventPtr->button() == Qt::LeftButton && m_pixmapItemPtr != nullptr)
    {
        m_isDrawingRoi = true;
        m_roiStartPos = mapToScene(eventPtr->position().toPoint());
        if (m_roiItemPtr == nullptr)
        {
            m_roiItemPtr = m_scenePtr->addRect(
                QRectF(), QPen(Qt::red, ROI_PEN_WIDTH));
            m_roiItemPtr->setZValue(1.0);
        }
        m_roiItemPtr->setRect(QRectF(m_roiStartPos, m_roiStartPos));
    }

    QGraphicsView::mousePressEvent(eventPtr);
}

// 根据当前交互模式更新平移位置或ROI矩形。
void ImgGraphicsView::mouseMoveEvent(QMouseEvent* eventPtr)
{
    const QPoint currentViewPos = eventPtr->position().toPoint();
    if (m_isPanning)
    {
        const QPoint delta = currentViewPos - m_lastPanPos;
        m_lastPanPos = currentViewPos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }
    else if (m_isDrawingRoi && m_roiItemPtr != nullptr)
    {
        const QPointF currentScenePos = mapToScene(currentViewPos);
        const QRectF roiRect = QRectF(m_roiStartPos, currentScenePos).normalized();
        m_roiItemPtr->setRect(roiRect);
    }

    QGraphicsView::mouseMoveEvent(eventPtr);
}

// 结束当前交互，并在ROI尺寸有效时发送选择结果。
void ImgGraphicsView::mouseReleaseEvent(QMouseEvent* eventPtr)
{
    if (eventPtr->button() == Qt::RightButton)
    {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
    else if (eventPtr->button() == Qt::LeftButton
             && m_isDrawingRoi
             && m_roiItemPtr != nullptr)
    {
        m_isDrawingRoi = false;
        const QRectF roiRect = m_roiItemPtr->rect();
        if (roiRect.width() > MIN_ROI_SIZE && roiRect.height() > MIN_ROI_SIZE)
        {
            emit roiSelected(roiRect);
        }
    }

    QGraphicsView::mouseReleaseEvent(eventPtr);
}

// 视图尺寸变化后重新适配图片并同步倍率。
void ImgGraphicsView::resizeEvent(QResizeEvent* eventPtr)
{
    QGraphicsView::resizeEvent(eventPtr);
    fitImageInView();
}

} // namespace image_viewer
