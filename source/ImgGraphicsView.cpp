#include "ImgGraphicsView.h"

#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

ImgGraphicsView::ImgGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(nullptr)
    , m_pixmapItem(nullptr)
    , m_roiItem(nullptr)
    , m_drawingROI(false)
    , m_panning(false)
    , m_scaleFactor(1.0)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void ImgGraphicsView::wheelEvent(QWheelEvent* event)
{
    if (!m_pixmapItem)
        return;

    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0)
        scale(scaleFactor, scaleFactor);
    else
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);

    m_scaleFactor = transform().m11();
    emit zoomChanged(m_scaleFactor);
}

void ImgGraphicsView::fitImageInView()
{
    if (!m_pixmapItem)
        return;

    fitInView(m_pixmapItem, Qt::KeepAspectRatio);
    m_scaleFactor = transform().m11();
    emit zoomChanged(m_scaleFactor);
}

void ImgGraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        m_panning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    else if (event->button() == Qt::LeftButton && m_pixmapItem)
    {
        m_drawingROI = true;
        m_startPos = mapToScene(event->pos());
        if (!m_roiItem)
        {
            m_roiItem = m_scene->addRect(QRectF(), QPen(Qt::red, 2));
            m_roiItem->setZValue(1);
        }
        m_roiItem->setRect(QRectF(m_startPos, m_startPos));
    }
    QGraphicsView::mousePressEvent(event);
}

void ImgGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_panning)
    {
        QPointF delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }
    else if (m_drawingROI && m_roiItem)
    {
        QPointF currentPos = mapToScene(event->pos());
        QRectF roiRect(m_startPos, currentPos);
        roiRect = roiRect.normalized();
        m_roiItem->setRect(roiRect);
    }
    QGraphicsView::mouseMoveEvent(event);
}

void ImgGraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
    }
    else if (event->button() == Qt::LeftButton && m_drawingROI && m_roiItem)
    {
        m_drawingROI = false;
        QRectF roiRect = m_roiItem->rect();
        if (roiRect.width() > 5 && roiRect.height() > 5)
        {
            emit roiSelected(roiRect);
        }
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void ImgGraphicsView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    fitImageInView();
}
