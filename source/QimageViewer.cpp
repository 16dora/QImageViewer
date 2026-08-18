#include "QimageViewer.h"
#include "ImgGraphicsView.h"
#include "ui_QimageViewer.h"
#include <QFileDialog>

QimageViewer::QimageViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::QimageViewerClass())
    , m_roiScene(nullptr)
    , m_roiPixmapItem(nullptr)
{
    ui->setupUi(this);
    connect(ui->graphic_MainWindow, &ImgGraphicsView::zoomChanged,
            this, &QimageViewer::onZoomChanged);
}

QimageViewer::~QimageViewer()
{
    delete ui;
}

void QimageViewer::on_Btn_LoadImage_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Image Files (*.png *.jpg *.jpeg *.bmp)");
    if (fileName.isEmpty())
        return;

    m_originalPixmap.load(fileName);
    if (m_originalPixmap.isNull())
        return;

    ImgGraphicsView* imageViewer = static_cast<ImgGraphicsView*>(ui->graphic_MainWindow);

    static bool connected = false;
    if (!connected)
    {
        connect(imageViewer, &ImgGraphicsView::roiSelected, this, &QimageViewer::onROISelected);
        connected = true;
    }

    imageViewer->scene()->clear();
    imageViewer->m_pixmapItem = imageViewer->scene()->addPixmap(m_originalPixmap);
    imageViewer->m_roiItem = nullptr;
    imageViewer->fitImageInView();

    if (!m_roiScene)
    {
        m_roiScene = new QGraphicsScene(this);
        m_roiPixmapItem = m_roiScene->addPixmap(QPixmap());
        QGraphicsView* roiView = new QGraphicsView(m_roiScene, this);
        roiView->setMinimumSize(300, 300);
        ui->gridLayout->addWidget(roiView, 0, 1);
    }
}

void QimageViewer::onZoomChanged(double scaleFactor)
{
    ui->label_Zoom->setText(QString("Zoom: %1x").arg(scaleFactor, 0, 'f', 2));
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

