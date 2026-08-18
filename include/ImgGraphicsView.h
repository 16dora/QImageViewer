#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPointF>
#include <QRectF>

class ImgGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    ImgGraphicsView(QWidget* parent = nullptr);
    void fitImageInView();

    QGraphicsScene* m_scene;
    QGraphicsPixmapItem* m_pixmapItem;
    QGraphicsRectItem* m_roiItem;
    QPointF m_startPos;
    QPointF m_lastPanPos;
    bool m_drawingROI;
    bool m_panning;
    double m_scaleFactor;

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

signals:
    void roiSelected(const QRectF& roiRect);
    void zoomChanged(double scaleFactor);
};
