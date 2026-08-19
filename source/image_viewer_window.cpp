#include "image_viewer_window.h"

#include "img_graphics_view.h"
#include "multi_curve_plot.h"
#include "ui_image_viewer_window.h"

#include <QButtonGroup>
#include <QColor>
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
    initializeIntensityPlots();
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_opticsChooseROI,
        static_cast<int>(ImgGraphicsView::ToolMode::Roi));
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_opticsDrawLine,
        static_cast<int>(ImgGraphicsView::ToolMode::Line));
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_opticsDrawCircle,
        static_cast<int>(ImgGraphicsView::ToolMode::Circle));
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_opticsDrawRect,
        static_cast<int>(ImgGraphicsView::ToolMode::Rect));
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_opticsChooseROI_2,
        static_cast<int>(ImgGraphicsView::ToolMode::Pick));
    connect(m_uiPtr->buttonGroup_imageToolMode,
            &QButtonGroup::idClicked,
            this,
            &ImageViewerWindow::onImageToolModeButtonClicked);
    connect(m_uiPtr->gview_mainImage,
            &ImgGraphicsView::zoomChanged,
            this,
            &ImageViewerWindow::onZoomChanged);
    connect(m_uiPtr->gview_mainImage,
            &ImgGraphicsView::roiSelected,
            this,
            &ImageViewerWindow::onRoiSelected);
    connect(m_uiPtr->gview_mainImage,
            &ImgGraphicsView::pixelPicked,
            this,
            &ImageViewerWindow::onPixelPicked);
    connect(m_uiPtr->gview_mainImage,
            &ImgGraphicsView::pixelPickCleared,
            this,
            &ImageViewerWindow::onPixelPickCleared);
    m_uiPtr->gview_mainImage->setToolMode(ImgGraphicsView::ToolMode::Roi);
}

// 使用默认析构释放智能指针管理的UI对象。
ImageViewerWindow::~ImageViewerWindow() = default;

// 创建X/Y强度Plot并注册可按图像类型切换的通道曲线。
void ImageViewerWindow::initializeIntensityPlots()
{
    MultiCurvePlot::Options plotOptions;
    plotOptions.title = QString();
    plotOptions.xAxisTitle = QString();
    plotOptions.yAxisTitle = QString();
    plotOptions.isLegendVisible = true;

    m_xProfilePlotPtr = std::make_unique<MultiCurvePlot>(
        m_uiPtr->plot_opticsXGrayProfile,
        plotOptions);
    m_yProfilePlotPtr = std::make_unique<MultiCurvePlot>(
        m_uiPtr->plot_opticsYGrayProfile,
        plotOptions);
    addIntensityProfileCurves(m_xProfilePlotPtr.get());
    addIntensityProfileCurves(m_yProfilePlotPtr.get());
    clearIntensityProfiles();
}

// 使用统一线宽、无采样点样式注册灰度和RGB曲线。
void ImageViewerWindow::addIntensityProfileCurves(
    MultiCurvePlot* profilePlotPtr)
{
    if (profilePlotPtr == nullptr)
    {
        return;
    }

    MultiCurvePlot::CurveOptions grayCurveOptions;
    grayCurveOptions.title = tr("Gray");
    grayCurveOptions.lineColor = Qt::black;
    grayCurveOptions.pointColor = Qt::black;
    grayCurveOptions.lineWidth = PROFILE_CURVE_LINE_WIDTH;
    grayCurveOptions.pointSize = PROFILE_CURVE_POINT_SIZE;
    grayCurveOptions.isVisible = false;
    profilePlotPtr->addCurve(QStringLiteral("gray"), grayCurveOptions);

    MultiCurvePlot::CurveOptions redCurveOptions;
    redCurveOptions.title = tr("R");
    redCurveOptions.lineColor = Qt::red;
    redCurveOptions.pointColor = Qt::red;
    redCurveOptions.lineWidth = PROFILE_CURVE_LINE_WIDTH;
    redCurveOptions.pointSize = PROFILE_CURVE_POINT_SIZE;
    redCurveOptions.isVisible = false;
    profilePlotPtr->addCurve(QStringLiteral("red"), redCurveOptions);

    MultiCurvePlot::CurveOptions greenCurveOptions;
    greenCurveOptions.title = tr("G");
    greenCurveOptions.lineColor = Qt::green;
    greenCurveOptions.pointColor = Qt::green;
    greenCurveOptions.lineWidth = PROFILE_CURVE_LINE_WIDTH;
    greenCurveOptions.pointSize = PROFILE_CURVE_POINT_SIZE;
    greenCurveOptions.isVisible = false;
    profilePlotPtr->addCurve(QStringLiteral("green"), greenCurveOptions);

    MultiCurvePlot::CurveOptions blueCurveOptions;
    blueCurveOptions.title = tr("B");
    blueCurveOptions.lineColor = Qt::blue;
    blueCurveOptions.pointColor = Qt::blue;
    blueCurveOptions.lineWidth = PROFILE_CURVE_LINE_WIDTH;
    blueCurveOptions.pointSize = PROFILE_CURVE_POINT_SIZE;
    blueCurveOptions.isVisible = false;
    profilePlotPtr->addCurve(QStringLiteral("blue"), blueCurveOptions);
}

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
    QImage loadedImage;
    const bool isLoaded = loadedImage.load(fileName);
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

    m_originalImage = loadedImage;
    m_currentImage = m_originalImage;
    m_validImagePolygon = QPolygonF(QRectF(m_originalImage.rect()));
    m_currentToOriginalTransform = QTransform();
    m_currentRotationAngleDegree = 0.0;
    m_uiPtr->cbox_rotationAngleDegree->setCurrentIndex(0);
    m_uiPtr->gview_mainImage->setImage(
        QPixmap::fromImage(m_currentImage),
        m_currentToOriginalTransform,
        m_originalImage.size());
    resetRoiPreview();
}

// 根据当前选择的绝对角度从原图生成旋转图片并重置视图状态。
void ImageViewerWindow::on_btn_rotateImage_clicked()
{
    if (m_originalImage.isNull())
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
    QTransform currentToOriginalTransform;
    const QImage rotatedImage = createRotatedImage(
        selectedRotationAngleDegree,
        &validImagePolygon,
        &currentToOriginalTransform);
    if (rotatedImage.isNull())
    {
        return;
    }

    m_currentImage = rotatedImage;
    m_validImagePolygon = validImagePolygon;
    m_currentToOriginalTransform = currentToOriginalTransform;
    m_currentRotationAngleDegree = selectedRotationAngleDegree;
    m_uiPtr->gview_mainImage->setImage(
        QPixmap::fromImage(m_currentImage),
        m_currentToOriginalTransform,
        m_originalImage.size());
    resetRoiPreview();
}

// 始终从原图生成指定绝对角度的黑色背景旋转图片。
QImage ImageViewerWindow::createRotatedImage(
    double rotationAngleDegree,
    QPolygonF* validImagePolygonPtr,
    QTransform* currentToOriginalTransformPtr) const
{
    if (m_originalImage.isNull()
        || validImagePolygonPtr == nullptr
        || currentToOriginalTransformPtr == nullptr)
    {
        return QImage();
    }

    if (qAbs(rotationAngleDegree) < ROTATION_ANGLE_EPSILON)
    {
        *validImagePolygonPtr = QPolygonF(QRectF(m_originalImage.rect()));
        *currentToOriginalTransformPtr = QTransform();
        return m_originalImage;
    }

    QTransform rotationTransform;
    rotationTransform.rotate(rotationAngleDegree);
    const QTransform adjustedRotationTransform = QImage::trueMatrix(
        rotationTransform,
        m_originalImage.width(),
        m_originalImage.height());
    *validImagePolygonPtr = adjustedRotationTransform.map(
        QPolygonF(QRectF(m_originalImage.rect())));
    bool isInvertible = false;
    *currentToOriginalTransformPtr =
        adjustedRotationTransform.inverted(&isInvertible);
    if (!isInvertible)
    {
        return QImage();
    }

    const QImage transformedImage = m_originalImage.transformed(
        rotationTransform, Qt::SmoothTransformation);
    const bool isSixteenBitImage = isOriginalImageSixteenBit();
    QImage rotatedImage(
        transformedImage.size(),
        isSixteenBitImage ? QImage::Format_RGBA64 : QImage::Format_ARGB32);
    rotatedImage.fill(Qt::black);
    QPainter painter(&rotatedImage);
    painter.drawImage(0, 0, transformedImage);
    return rotatedImage;
}

// 清除底层ROI数据并恢复右侧预览的英文初始文本。
void ImageViewerWindow::resetRoiPreview()
{
    m_currentRoiImage = QImage();
    m_uiPtr->label_roiPreview->clear();
    m_uiPtr->label_roiPreview->setText(tr("ROI"));
}

// 将ROI限制在当前旋转画布范围内，并更新右侧缩略预览。
void ImageViewerWindow::onRoiSelected(const QRectF& roiRect)
{
    if (m_currentImage.isNull())
    {
        return;
    }

    const QRect boundedRoi =
        roiRect.toAlignedRect().intersected(m_currentImage.rect());
    m_currentRoiImage = m_currentImage.copy(boundedRoi);
    if (m_currentRoiImage.isNull())
    {
        return;
    }

    const QSize previewSize = m_uiPtr->label_roiPreview->contentsRect().size();
    QPixmap previewPixmap = QPixmap::fromImage(m_currentRoiImage).scaled(
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

// 将按钮组编号转换为图片视图工具模式。
void ImageViewerWindow::onImageToolModeButtonClicked(int buttonId)
{
    m_uiPtr->gview_mainImage->setToolMode(
        static_cast<ImgGraphicsView::ToolMode>(buttonId));
    m_uiPtr->gview_mainImage->setFocus(Qt::ShortcutFocusReason);
}

// 显示Pick到的原图零基整数像素坐标。
void ImageViewerWindow::onPixelPicked(const QPoint& originalPixelPosition)
{
    m_uiPtr->label_PickedPixel->setText(
        tr("X: %1,Y:%2")
            .arg(originalPixelPosition.x())
            .arg(originalPixelPosition.y()));
    updateIntensityProfiles(originalPixelPosition);
}

// 将Pick标签和两个强度Plot恢复为无有效点状态。
void ImageViewerWindow::onPixelPickCleared()
{
    m_uiPtr->label_PickedPixel->setText(tr("X: --,Y:--"));
    clearIntensityProfiles();
}

// 从未旋转原图生成当前Pick位置的X行与Y列强度曲线。
void ImageViewerWindow::updateIntensityProfiles(
    const QPoint& originalPixelPosition)
{
    if (m_originalImage.isNull()
        || !m_originalImage.rect().contains(originalPixelPosition)
        || m_xProfilePlotPtr == nullptr
        || m_yProfilePlotPtr == nullptr)
    {
        clearIntensityProfiles();
        return;
    }

    const bool isGrayscale = isOriginalImageGrayscale();
    m_xProfilePlotPtr->setCurveVisible(QStringLiteral("gray"), isGrayscale);
    m_yProfilePlotPtr->setCurveVisible(QStringLiteral("gray"), isGrayscale);
    m_xProfilePlotPtr->setCurveVisible(QStringLiteral("red"), !isGrayscale);
    m_yProfilePlotPtr->setCurveVisible(QStringLiteral("red"), !isGrayscale);
    m_xProfilePlotPtr->setCurveVisible(QStringLiteral("green"), !isGrayscale);
    m_yProfilePlotPtr->setCurveVisible(QStringLiteral("green"), !isGrayscale);
    m_xProfilePlotPtr->setCurveVisible(QStringLiteral("blue"), !isGrayscale);
    m_yProfilePlotPtr->setCurveVisible(QStringLiteral("blue"), !isGrayscale);

    if (isGrayscale)
    {
        m_xProfilePlotPtr->setCurveSamples(
            QStringLiteral("gray"),
            createProfileSamples(
                ProfileAxis::X,
                originalPixelPosition.y(),
                ImageChannel::Gray));
        m_yProfilePlotPtr->setCurveSamples(
            QStringLiteral("gray"),
            createProfileSamples(
                ProfileAxis::Y,
                originalPixelPosition.x(),
                ImageChannel::Gray));
        m_xProfilePlotPtr->clearCurve(QStringLiteral("red"));
        m_xProfilePlotPtr->clearCurve(QStringLiteral("green"));
        m_xProfilePlotPtr->clearCurve(QStringLiteral("blue"));
        m_yProfilePlotPtr->clearCurve(QStringLiteral("red"));
        m_yProfilePlotPtr->clearCurve(QStringLiteral("green"));
        m_yProfilePlotPtr->clearCurve(QStringLiteral("blue"));
    }
    else
    {
        m_xProfilePlotPtr->clearCurve(QStringLiteral("gray"));
        m_yProfilePlotPtr->clearCurve(QStringLiteral("gray"));
        m_xProfilePlotPtr->setCurveSamples(
            QStringLiteral("red"),
            createProfileSamples(
                ProfileAxis::X,
                originalPixelPosition.y(),
                ImageChannel::Red));
        m_xProfilePlotPtr->setCurveSamples(
            QStringLiteral("green"),
            createProfileSamples(
                ProfileAxis::X,
                originalPixelPosition.y(),
                ImageChannel::Green));
        m_xProfilePlotPtr->setCurveSamples(
            QStringLiteral("blue"),
            createProfileSamples(
                ProfileAxis::X,
                originalPixelPosition.y(),
                ImageChannel::Blue));
        m_yProfilePlotPtr->setCurveSamples(
            QStringLiteral("red"),
            createProfileSamples(
                ProfileAxis::Y,
                originalPixelPosition.x(),
                ImageChannel::Red));
        m_yProfilePlotPtr->setCurveSamples(
            QStringLiteral("green"),
            createProfileSamples(
                ProfileAxis::Y,
                originalPixelPosition.x(),
                ImageChannel::Green));
        m_yProfilePlotPtr->setCurveSamples(
            QStringLiteral("blue"),
            createProfileSamples(
                ProfileAxis::Y,
                originalPixelPosition.x(),
                ImageChannel::Blue));
    }

    const double intensityMaximum = isOriginalImageSixteenBit()
        ? SIXTEEN_BIT_INTENSITY_MAXIMUM
        : EIGHT_BIT_INTENSITY_MAXIMUM;
    m_xProfilePlotPtr->setAxisRanges(
        0.0,
        qMax(1, m_originalImage.width() - 1),
        0.0,
        intensityMaximum);
    m_yProfilePlotPtr->setAxisRanges(
        0.0,
        qMax(1, m_originalImage.height() - 1),
        0.0,
        intensityMaximum);
}

// 清空两个Plot数据并隐藏全部通道和图例。
void ImageViewerWindow::clearIntensityProfiles()
{
    if (m_xProfilePlotPtr != nullptr)
    {
        m_xProfilePlotPtr->clear();
        m_xProfilePlotPtr->setCurveVisible(QStringLiteral("gray"), false);
        m_xProfilePlotPtr->setCurveVisible(QStringLiteral("red"), false);
        m_xProfilePlotPtr->setCurveVisible(QStringLiteral("green"), false);
        m_xProfilePlotPtr->setCurveVisible(QStringLiteral("blue"), false);
        m_xProfilePlotPtr->setAxisRanges(0.0, 1.0, 0.0, 1.0);
    }
    if (m_yProfilePlotPtr != nullptr)
    {
        m_yProfilePlotPtr->clear();
        m_yProfilePlotPtr->setCurveVisible(QStringLiteral("gray"), false);
        m_yProfilePlotPtr->setCurveVisible(QStringLiteral("red"), false);
        m_yProfilePlotPtr->setCurveVisible(QStringLiteral("green"), false);
        m_yProfilePlotPtr->setCurveVisible(QStringLiteral("blue"), false);
        m_yProfilePlotPtr->setAxisRanges(0.0, 1.0, 0.0, 1.0);
    }
}

// 生成完整原图行或列的像素索引与通道强度样本。
QVector<QPointF> ImageViewerWindow::createProfileSamples(
    ProfileAxis profileAxis,
    int fixedPixelPosition,
    ImageChannel imageChannel) const
{
    QVector<QPointF> samples;
    if (m_originalImage.isNull())
    {
        return samples;
    }

    const int sampleCount = profileAxis == ProfileAxis::X
        ? m_originalImage.width()
        : m_originalImage.height();
    samples.reserve(sampleCount);
    for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
    {
        const int pixelX = profileAxis == ProfileAxis::X
            ? sampleIndex
            : fixedPixelPosition;
        const int pixelY = profileAxis == ProfileAxis::X
            ? fixedPixelPosition
            : sampleIndex;
        samples.append(QPointF(
            sampleIndex,
            pixelIntensityAt(pixelX, pixelY, imageChannel)));
    }
    return samples;
}

// 按原图实际像素格式读取八位或十六位通道强度。
double ImageViewerWindow::pixelIntensityAt(
    int pixelX,
    int pixelY,
    ImageChannel imageChannel) const
{
    if (imageChannel == ImageChannel::Gray)
    {
        if (m_originalImage.format() == QImage::Format_Grayscale16)
        {
            const quint16* scanLinePtr = reinterpret_cast<const quint16*>(
                m_originalImage.constScanLine(pixelY));
            return scanLinePtr[pixelX];
        }
        if (m_originalImage.format() == QImage::Format_Alpha8)
        {
            return m_originalImage.constScanLine(pixelY)[pixelX];
        }
        return qGray(m_originalImage.pixel(pixelX, pixelY));
    }

    const QColor pixelColor = m_originalImage.pixelColor(pixelX, pixelY);
    if (isOriginalImageSixteenBit())
    {
        const QRgba64 rgba64 = pixelColor.rgba64();
        if (imageChannel == ImageChannel::Red)
        {
            return rgba64.red();
        }
        if (imageChannel == ImageChannel::Green)
        {
            return rgba64.green();
        }
        return rgba64.blue();
    }

    if (imageChannel == ImageChannel::Red)
    {
        return pixelColor.red();
    }
    if (imageChannel == ImageChannel::Green)
    {
        return pixelColor.green();
    }
    return pixelColor.blue();
}

// 根据原图像素格式判断是否应显示单条灰度曲线。
bool ImageViewerWindow::isOriginalImageGrayscale() const
{
    switch (m_originalImage.format())
    {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Grayscale8:
    case QImage::Format_Grayscale16:
    case QImage::Format_Alpha8:
        return true;
    case QImage::Format_Indexed8:
        return m_originalImage.isGrayscale();
    default:
        return false;
    }
}

// 判断原图是否使用十六位整数通道格式。
bool ImageViewerWindow::isOriginalImageSixteenBit() const
{
    switch (m_originalImage.format())
    {
    case QImage::Format_Grayscale16:
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
        return true;
    default:
        return false;
    }
}

} // namespace image_viewer
