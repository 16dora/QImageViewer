#include "QimageViewer.h"
#include "ui_QimageViewer.h"
#include <QFileDialog>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>

ImageViewer::ImageViewer(QWidget* parent)
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

void ImageViewer::wheelEvent(QWheelEvent* event)
{
    if (!m_pixmapItem)
        return;

    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0)
        scale(scaleFactor, scaleFactor);
    else
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);

    m_scaleFactor = transform().m11();
}

void ImageViewer::mousePressEvent(QMouseEvent* event)
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

void ImageViewer::mouseMoveEvent(QMouseEvent* event)
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

void ImageViewer::mouseReleaseEvent(QMouseEvent* event)
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

void ImageViewer::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (m_pixmapItem)
        fitInView(m_pixmapItem, Qt::KeepAspectRatio);
}

void QimageViewer::on_Btn_LoadImage_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Image Files (*.png *.jpg *.jpeg *.bmp)");
    if (fileName.isEmpty())
        return;

    m_originalPixmap.load(fileName);
    if (m_originalPixmap.isNull())
        return;

    ImageViewer* imageViewer = static_cast<ImageViewer*>(ui->graphic_MainWindow);

    static bool connected = false;
    if (!connected)
    {
        connect(imageViewer, &ImageViewer::roiSelected, this, &QimageViewer::onROISelected);
        connected = true;
    }

    imageViewer->scene()->clear();
    imageViewer->m_pixmapItem = imageViewer->scene()->addPixmap(m_originalPixmap);
    imageViewer->m_roiItem = nullptr;
    imageViewer->fitInView(imageViewer->m_pixmapItem, Qt::KeepAspectRatio);
    imageViewer->m_scaleFactor = 1.0;

    if (!m_roiScene)
    {
        m_roiScene = new QGraphicsScene(this);
        m_roiPixmapItem = m_roiScene->addPixmap(QPixmap());
        QGraphicsView* roiView = new QGraphicsView(m_roiScene, this);
        roiView->setMinimumSize(300, 300);
        ui->gridLayout->addWidget(roiView, 0, 1);
    }
}

void QimageViewer::onROISelected(const QRectF& roiRect)
{
    if (m_originalPixmap.isNull())
        return;

    QRect imageRect = m_originalPixmap.rect();
    QRectF normalizedROI = roiRect.intersected(QRectF(imageRect));

    QPixmap roiPixmap = m_originalPixmap.copy(normalizedROI.toRect());
    if (!roiPixmap.isNull())
    {
        m_roiPixmapItem->setPixmap(roiPixmap.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_roiScene->setSceneRect(m_roiPixmapItem->boundingRect());
    }
}

QimageViewer::QimageViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QimageViewerClass())
    , m_roiScene(nullptr)
    , m_roiPixmapItem(nullptr)
{
    ui->setupUi(this);
}

QimageViewer::~QimageViewer()
{
    delete ui;
}


