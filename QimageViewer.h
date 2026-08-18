#pragma once

#include <QtWidgets/QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QPixmap>
#include <QPointF>


QT_BEGIN_NAMESPACE
namespace Ui { class QimageViewerClass; };
QT_END_NAMESPACE

class ImageViewer : public QGraphicsView
{
    Q_OBJECT

public:
    ImageViewer(QWidget* parent = nullptr);

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
};

class QimageViewer : public QMainWindow
{
    Q_OBJECT

public:
    QimageViewer(QWidget *parent = nullptr);
    ~QimageViewer();

private slots:
    void on_Btn_LoadImage_clicked();
    void onROISelected(const QRectF& roiRect);

private:
    Ui::QimageViewerClass *ui;
    QGraphicsScene* m_roiScene;
    QGraphicsPixmapItem* m_roiPixmapItem;
    QPixmap m_originalPixmap;
};

