#include "image_viewer_window.h"

#include "img_graphics_view.h"
#include "ui_image_viewer_window.h"

#include <QFileDialog>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStatusBar>
#include <QTransform>
#include <QtMath>

namespace image_viewer {

// 创建主窗口、初始化UI，并建立图片视图信号连接。
ImageViewerWindow::ImageViewerWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_uiPtr(std::make_unique<Ui::ImageViewerWindow>())
    , m_currentRotationAngleDegree(0.0)
{
    m_uiPtr->setupUi(this);
    connect(m_uiPtr->gview_mainImage,
            &ImgGraphicsView::zoomChanged,
            this,
            &ImageViewerWindow::onZoomChanged);
    connect(m_uiPtr->gview_mainImage,
            &ImgGraphicsView::roiSelected,
            this,
            &ImageViewerWindow::onRoiSelected);
}

// 使用默认析构释放智能指针管理的UI对象。
ImageViewerWindow::~ImageViewerWindow() = default;

// 选择并加载本地图片，然后更新主视图与ROI初始状态。
void ImageViewerWindow::on_btn_loadImage_clicked()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open Image"),
        QString(),
        tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if (fileName.isEmpty())
    {
        return;
    }

    m_uiPtr->btn_loadImage->setEnabled(false);
    QPixmap loadedPixmap;
    const bool isLoaded = loadedPixmap.load(fileName);
    m_uiPtr->btn_loadImage->setEnabled(true);

    if (!isLoaded)
    {
        statusBar()->showMessage(
            tr("[ImageViewer] Failed to load image '%1' (Code: %2)")
                .arg(fileName)
                .arg(IMAGE_LOAD_ERROR_CODE),
            STATUS_MESSAGE_TIMEOUT_MS);
        return;
    }

    m_originalPixmap = loadedPixmap;
    m_currentPixmap = m_originalPixmap;
    m_validImagePolygon = QPolygonF(QRectF(m_originalPixmap.rect()));
    m_currentRotationAngleDegree = 0.0;
    m_uiPtr->cbox_rotationAngleDegree->setCurrentIndex(0);
    m_uiPtr->gview_mainImage->setImage(m_currentPixmap);
    resetRoiPreview();
}

// 根据当前选择的绝对角度从原图生成旋转图片并重置视图状态。
void ImageViewerWindow::on_btn_rotateImage_clicked()
{
    if (m_originalPixmap.isNull())
    {
        return;
    }

    const double selectedRotationAngleDegree =
        m_uiPtr->cbox_rotationAngleDegree->currentIndex()
        * ROTATION_ANGLE_STEP_DEGREE;
    if (qAbs(selectedRotationAngleDegree - m_currentRotationAngleDegree)
        < ROTATION_ANGLE_EPSILON)
    {
        return;
    }

    QPolygonF validImagePolygon;
    const QPixmap rotatedPixmap = createRotatedPixmap(
        selectedRotationAngleDegree, &validImagePolygon);
    if (rotatedPixmap.isNull())
    {
        return;
    }

    m_currentPixmap = rotatedPixmap;
    m_validImagePolygon = validImagePolygon;
    m_currentRotationAngleDegree = selectedRotationAngleDegree;
    m_uiPtr->gview_mainImage->setImage(m_currentPixmap);
    resetRoiPreview();
}

// 始终从原图生成指定绝对角度的黑色背景旋转图片。
QPixmap ImageViewerWindow::createRotatedPixmap(
    double rotationAngleDegree,
    QPolygonF* validImagePolygonPtr) const
{
    if (m_originalPixmap.isNull() || validImagePolygonPtr == nullptr)
    {
        return QPixmap();
    }

    if (qAbs(rotationAngleDegree) < ROTATION_ANGLE_EPSILON)
    {
        *validImagePolygonPtr = QPolygonF(QRectF(m_originalPixmap.rect()));
        return m_originalPixmap;
    }

    QTransform rotationTransform;
    rotationTransform.rotate(rotationAngleDegree);
    const QTransform adjustedRotationTransform = QPixmap::trueMatrix(
        rotationTransform,
        m_originalPixmap.width(),
        m_originalPixmap.height());
    *validImagePolygonPtr = adjustedRotationTransform.map(
        QPolygonF(QRectF(m_originalPixmap.rect())));

    const QPixmap transformedPixmap = m_originalPixmap.transformed(
        rotationTransform, Qt::SmoothTransformation);
    QPixmap rotatedPixmap(transformedPixmap.size());
    rotatedPixmap.fill(Qt::black);
    QPainter painter(&rotatedPixmap);
    painter.drawPixmap(0, 0, transformedPixmap);
    return rotatedPixmap;
}

// 清除底层ROI数据并恢复右侧预览的英文初始文本。
void ImageViewerWindow::resetRoiPreview()
{
    m_currentRoiPixmap = QPixmap();
    m_uiPtr->label_roiPreview->clear();
    m_uiPtr->label_roiPreview->setText(tr("ROI"));
}

// 将ROI限制在当前旋转画布范围内，并更新右侧缩略预览。
void ImageViewerWindow::onRoiSelected(const QRectF& roiRect)
{
    if (m_currentPixmap.isNull())
    {
        return;
    }

    const QRect boundedRoi =
        roiRect.toAlignedRect().intersected(m_currentPixmap.rect());
    m_currentRoiPixmap = m_currentPixmap.copy(boundedRoi);
    if (m_currentRoiPixmap.isNull())
    {
        return;
    }

    const QSize previewSize = m_uiPtr->label_roiPreview->contentsRect().size();
    QPixmap previewPixmap = m_currentRoiPixmap.scaled(
        previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    drawRoiValidContentOutline(&previewPixmap, boundedRoi);
    m_uiPtr->label_roiPreview->setPixmap(previewPixmap);
}

// 在ROI预览副本上绘制真实图像交集区域的闭合蓝色轮廓。
void ImageViewerWindow::drawRoiValidContentOutline(
    QPixmap* previewPixmapPtr,
    const QRect& roiRect) const
{
    if (previewPixmapPtr == nullptr
        || previewPixmapPtr->isNull()
        || roiRect.isEmpty()
        || m_validImagePolygon.isEmpty())
    {
        return;
    }

    QPainterPath validImagePath;
    validImagePath.addPolygon(m_validImagePolygon);
    validImagePath.closeSubpath();
    const QRectF roiSceneRect(roiRect);
    QPainterPath roiPath;
    roiPath.addRect(roiSceneRect);
    const QPainterPath validRoiPath = validImagePath.intersected(roiPath);
    const QPainterPath invalidRoiPath = roiPath.subtracted(validImagePath);
    if (validRoiPath.isEmpty() || invalidRoiPath.isEmpty())
    {
        return;
    }

    const qreal previewScaleX =
        static_cast<qreal>(previewPixmapPtr->width()) / roiSceneRect.width();
    const qreal previewScaleY =
        static_cast<qreal>(previewPixmapPtr->height()) / roiSceneRect.height();
    const qreal previewMaxX = qMax(
        ROI_VALID_CONTENT_INSET,
        static_cast<qreal>(previewPixmapPtr->width())
            - ROI_VALID_CONTENT_INSET);
    const qreal previewMaxY = qMax(
        ROI_VALID_CONTENT_INSET,
        static_cast<qreal>(previewPixmapPtr->height())
            - ROI_VALID_CONTENT_INSET);

    const QPolygonF validRoiPolygon = validRoiPath.toFillPolygon();
    QPolygonF previewOutlinePolygon;
    previewOutlinePolygon.reserve(validRoiPolygon.size());
    for (const QPointF& validPoint : validRoiPolygon)
    {
        const qreal previewX = qBound(
            ROI_VALID_CONTENT_INSET,
            (validPoint.x() - roiSceneRect.left()) * previewScaleX,
            previewMaxX);
        const qreal previewY = qBound(
            ROI_VALID_CONTENT_INSET,
            (validPoint.y() - roiSceneRect.top()) * previewScaleY,
            previewMaxY);
        previewOutlinePolygon.append(QPointF(previewX, previewY));
    }

    QPainter painter(previewPixmapPtr);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::blue, ROI_VALID_CONTENT_PEN_WIDTH));
    painter.drawPolygon(previewOutlinePolygon);
}

// 将实际倍率格式化为带单位的英文文本。
void ImageViewerWindow::onZoomChanged(double scaleFactor)
{
    m_uiPtr->label_zoom->setText(tr("Zoom: %1x").arg(scaleFactor, 0, 'f', 2));
}

} // namespace image_viewer
