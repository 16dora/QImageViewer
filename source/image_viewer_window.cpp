#include "image_viewer_window.h"

#include "img_graphics_view.h"
#include "multi_curve_plot.h"
#include "parameter_generation_dialog.h"
#include "ui_image_viewer_window.h"

#include <QAbstractButton>
#include <QAction>
#include <QButtonGroup>
#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStatusBar>
#include <QToolButton>
#include <QTransform>
#include <QtMath>

namespace image_viewer {

// 创建主窗口、初始化UI，并建立图片视图信号连接。
ImageViewerWindow::ImageViewerWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_uiPtr(std::make_unique<Ui::ImageViewerWindow>())
    , m_currentRotationAngleDegree(0.0)
    , m_currentMaskRotationAngleDegree(0.0)
    , m_maskOpacityPercent(MASK_DEFAULT_OPACITY_PERCENT)
    , m_isVerticalFlipEnabled(false)
    , m_isHorizontalFlipEnabled(false)
    , m_isMaskVerticalFlipEnabled(false)
    , m_isMaskHorizontalFlipEnabled(false)
{
    m_uiPtr->setupUi(this);
    initializeIntensityPlots();
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_chooseRoi,
        static_cast<int>(ImgGraphicsView::ToolMode::Roi));
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_drawLine,
        static_cast<int>(ImgGraphicsView::ToolMode::Line));
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_drawCircle,
        static_cast<int>(ImgGraphicsView::ToolMode::Circle));
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_drawRect,
        static_cast<int>(ImgGraphicsView::ToolMode::Rect));
    m_uiPtr->buttonGroup_imageToolMode->setId(
        m_uiPtr->tbtn_pickPixel,
        static_cast<int>(ImgGraphicsView::ToolMode::Pick));
    connect(m_uiPtr->buttonGroup_imageToolMode,
            &QButtonGroup::idClicked,
            this,
            &ImageViewerWindow::onImageToolModeButtonClicked);
    m_uiPtr->tbtn_drawLine->setContextMenuPolicy(Qt::CustomContextMenu);
    m_uiPtr->tbtn_drawCircle->setContextMenuPolicy(Qt::CustomContextMenu);
    m_uiPtr->tbtn_drawRect->setContextMenuPolicy(Qt::CustomContextMenu);
    m_uiPtr->tbtn_pickPixel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_uiPtr->tbtn_drawLine,
            &QToolButton::customContextMenuRequested,
            this,
            &ImageViewerWindow::onLineToolContextMenuRequested);
    connect(m_uiPtr->tbtn_drawCircle,
            &QToolButton::customContextMenuRequested,
            this,
            &ImageViewerWindow::onCircleToolContextMenuRequested);
    connect(m_uiPtr->tbtn_drawRect,
            &QToolButton::customContextMenuRequested,
            this,
            &ImageViewerWindow::onRectToolContextMenuRequested);
    connect(m_uiPtr->tbtn_pickPixel,
            &QToolButton::customContextMenuRequested,
            this,
            &ImageViewerWindow::onPickToolContextMenuRequested);
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
    m_uiPtr->gview_mainImage->setToolMode(ImgGraphicsView::ToolMode::None);
    updateFlipButtonUiState();
    updateMaskControlUiState();
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
        m_uiPtr->plot_xIntensityProfile,
        plotOptions);
    m_yProfilePlotPtr = std::make_unique<MultiCurvePlot>(
        m_uiPtr->plot_yIntensityProfile,
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

    clearMask();
    m_originalImage = loadedImage;
    m_uiPtr->cbox_rotationAngleDegree->setCurrentIndex(0);
    applyImageTransform(0.0, false, false);
}

// 根据当前绝对角度和已启用翻转状态重新生成图片。
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

    applyImageTransform(
        selectedRotationAngleDegree,
        m_isVerticalFlipEnabled,
        m_isHorizontalFlipEnabled);
}

// 恢复原始加载图像并清除全部ROI、覆盖图形和Pick状态。
void ImageViewerWindow::on_tbtn_resetView_clicked()
{
    if (m_originalImage.isNull())
    {
        return;
    }

    clearMask();
    if (applyImageTransform(0.0, false, false))
    {
        m_uiPtr->cbox_rotationAngleDegree->setCurrentIndex(0);
    }
}

// 将当前变换图像按时间戳命名并保存到应用程序Save目录。
void ImageViewerWindow::on_btn_saveImage_clicked()
{
    if (m_currentImage.isNull())
    {
        statusBar()->showMessage(
            tr("[ImageViewer] No image is available to save."),
            STATUS_MESSAGE_TIMEOUT_MS);
        return;
    }

    m_uiPtr->btn_saveImage->setEnabled(false);
    const QString saveDirectoryPath =
        QDir(QCoreApplication::applicationDirPath()).filePath(
            QStringLiteral("Save"));
    QDir directoryCreator;
    if (!directoryCreator.mkpath(saveDirectoryPath))
    {
        m_uiPtr->btn_saveImage->setEnabled(true);
        statusBar()->showMessage(
            tr("[ImageViewer] Failed to create save directory '%1'.")
                .arg(QDir::toNativeSeparators(saveDirectoryPath)),
            STATUS_MESSAGE_TIMEOUT_MS);
        return;
    }

    QImage imageToSave = m_currentImage;
    if (m_uiPtr->check_keepOverlay->isChecked())
    {
        imageToSave = m_uiPtr->gview_mainImage->createImageWithOverlays(
            m_currentImage);
    }
    if (imageToSave.isNull())
    {
        m_uiPtr->btn_saveImage->setEnabled(true);
        statusBar()->showMessage(
            tr("[ImageViewer] Failed to prepare the image for saving."),
            STATUS_MESSAGE_TIMEOUT_MS);
        return;
    }

    const QString imageFormat =
        m_uiPtr->cbox_imageFormat->currentText().trimmed().toLower();
    const QString imageFileName =
        QDateTime::currentDateTime().toString(
            QStringLiteral("yyyyMMddHHmmsszzz"))
        + QStringLiteral(".") + imageFormat;
    const QString imageFilePath =
        QDir(saveDirectoryPath).filePath(imageFileName);
    QImageWriter imageWriter(imageFilePath, imageFormat.toLatin1());
    const bool isSaved = imageWriter.write(imageToSave);
    m_uiPtr->btn_saveImage->setEnabled(true);
    if (!isSaved)
    {
        statusBar()->showMessage(
            tr("[ImageViewer] Failed to save image '%1': %2")
                .arg(
                    QDir::toNativeSeparators(imageFilePath),
                    imageWriter.errorString()),
            STATUS_MESSAGE_TIMEOUT_MS);
        return;
    }

    statusBar()->showMessage(
        tr("[ImageViewer] Saved image to '%1'.")
            .arg(QDir::toNativeSeparators(imageFilePath)),
        STATUS_MESSAGE_TIMEOUT_MS);
}

// 选择并加载实际位深为1的BMP Mask文件。
void ImageViewerWindow::on_btn_loadMaskFile_clicked()
{
    if (m_originalImage.isNull())
    {
        QMessageBox::warning(
            this,
            tr("Load Mask"),
            tr("Load an image before loading a Mask file."));
        return;
    }

    const QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Load Mask"),
        QString(),
        tr("1-bit Bitmap Mask (*.bmp)"));
    if (fileName.isEmpty())
    {
        return;
    }

    m_uiPtr->btn_loadMaskFile->setEnabled(false);
    const QByteArray detectedImageFormat =
        QImageReader::imageFormat(fileName).toLower();
    QImageReader maskImageReader(fileName);
    const QImage loadedMaskImage = maskImageReader.read();
    m_uiPtr->btn_loadMaskFile->setEnabled(true);
    if (detectedImageFormat != QByteArrayLiteral("bmp")
        || loadedMaskImage.isNull())
    {
        QMessageBox::critical(
            this,
            tr("Invalid Mask File"),
            tr("The Mask file must be a valid BMP image.\n%1")
                .arg(maskImageReader.errorString()));
        return;
    }
    if (loadedMaskImage.depth() != MASK_REQUIRED_IMAGE_DEPTH)
    {
        QMessageBox::critical(
            this,
            tr("Invalid Mask Depth"),
            tr("The Mask image must have a bit depth of 1. "
               "The selected image has a bit depth of %1.")
                .arg(loadedMaskImage.depth()));
        return;
    }

    const QImage binaryMaskImage = createBinaryMaskImage(loadedMaskImage);
    if (binaryMaskImage.isNull())
    {
        QMessageBox::critical(
            this,
            tr("Invalid Mask File"),
            tr("Failed to prepare the selected Mask image."));
        return;
    }

    clearMask();
    m_originalMaskImage = binaryMaskImage;
    if (!applyMaskTransform(
            0.0,
            false,
            false,
            MASK_DEFAULT_OPACITY_PERCENT))
    {
        clearMask();
        QMessageBox::critical(
            this,
            tr("Mask Error"),
            tr("Failed to display the selected Mask image."));
        return;
    }

    statusBar()->showMessage(
        tr("[ImageViewer] Loaded Mask image '%1'.")
            .arg(QDir::toNativeSeparators(fileName)),
        STATUS_MESSAGE_TIMEOUT_MS);
}

// 应用Mask黑色区域的不透明度百分比。
void ImageViewerWindow::on_btn_applyMaskOpacity_clicked()
{
    if (!applyMaskTransform(
            m_currentMaskRotationAngleDegree,
            m_isMaskVerticalFlipEnabled,
            m_isMaskHorizontalFlipEnabled,
            m_uiPtr->sbox_maskOpacityPercent->value()))
    {
        updateMaskControlUiState();
    }
}

// 从原始Mask应用下拉框选择的绝对顺时针旋转角度。
void ImageViewerWindow::on_btn_applyMaskRotation_clicked()
{
    const double selectedRotationAngleDegree =
        m_uiPtr->cbox_maskRotationAngleDegree->currentIndex()
        * ROTATION_ANGLE_STEP_DEGREE;
    if (!applyMaskTransform(
            selectedRotationAngleDegree,
            m_isMaskVerticalFlipEnabled,
            m_isMaskHorizontalFlipEnabled,
            m_maskOpacityPercent))
    {
        updateMaskControlUiState();
    }
}

// 根据切换状态应用Mask绕竖直中轴的左右翻转。
void ImageViewerWindow::on_btn_maskVerticalFlip_clicked(bool isChecked)
{
    if (!applyMaskTransform(
            m_currentMaskRotationAngleDegree,
            isChecked,
            m_isMaskHorizontalFlipEnabled,
            m_maskOpacityPercent))
    {
        updateMaskControlUiState();
    }
}

// 根据切换状态应用Mask绕水平中轴的上下翻转。
void ImageViewerWindow::on_btn_maskHorizontalFlip_clicked(bool isChecked)
{
    if (!applyMaskTransform(
            m_currentMaskRotationAngleDegree,
            m_isMaskVerticalFlipEnabled,
            isChecked,
            m_maskOpacityPercent))
    {
        updateMaskControlUiState();
    }
}

// 根据切换状态重新生成绕竖直中轴左右翻转的当前图像。
void ImageViewerWindow::on_btn_verticalFlip_clicked(bool isChecked)
{
    if (!applyImageTransform(
            m_currentRotationAngleDegree,
            isChecked,
            m_isHorizontalFlipEnabled))
    {
        updateFlipButtonUiState();
    }
}

// 根据切换状态重新生成绕水平中轴上下翻转的当前图像。
void ImageViewerWindow::on_btn_horizontalFlip_clicked(bool isChecked)
{
    if (!applyImageTransform(
            m_currentRotationAngleDegree,
            m_isVerticalFlipEnabled,
            isChecked))
    {
        updateFlipButtonUiState();
    }
}

// 从原图生成以翻转后左上角为原点的逻辑图像。
QImage ImageViewerWindow::createFlippedImage(
    bool isVerticalFlipEnabled,
    bool isHorizontalFlipEnabled) const
{
    if (m_originalImage.isNull())
    {
        return QImage();
    }
    if (!isVerticalFlipEnabled && !isHorizontalFlipEnabled)
    {
        return m_originalImage;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    Qt::Orientations flipOrientations;
    if (isVerticalFlipEnabled)
    {
        flipOrientations |= Qt::Horizontal;
    }
    if (isHorizontalFlipEnabled)
    {
        flipOrientations |= Qt::Vertical;
    }
    return m_originalImage.flipped(flipOrientations);
#else
    return m_originalImage.mirrored(
        isVerticalFlipEnabled,
        isHorizontalFlipEnabled);
#endif
}

// 从逻辑图像生成指定绝对角度的黑色背景旋转图像。
QImage ImageViewerWindow::createRotatedImage(
    const QImage& logicalImage,
    double rotationAngleDegree,
    QPolygonF* validImagePolygonPtr,
    QTransform* currentToLogicalTransformPtr) const
{
    if (logicalImage.isNull()
        || validImagePolygonPtr == nullptr
        || currentToLogicalTransformPtr == nullptr)
    {
        return QImage();
    }

    if (qAbs(rotationAngleDegree) < ROTATION_ANGLE_EPSILON)
    {
        *validImagePolygonPtr = QPolygonF(QRectF(logicalImage.rect()));
        *currentToLogicalTransformPtr = QTransform();
        return logicalImage;
    }

    QTransform rotationTransform;
    rotationTransform.rotate(rotationAngleDegree);
    const QTransform logicalToCurrentTransform = QImage::trueMatrix(
        rotationTransform,
        logicalImage.width(),
        logicalImage.height());
    *validImagePolygonPtr = logicalToCurrentTransform.map(
        QPolygonF(QRectF(logicalImage.rect())));

    bool isInvertible = false;
    *currentToLogicalTransformPtr =
        logicalToCurrentTransform.inverted(&isInvertible);
    if (!isInvertible)
    {
        return QImage();
    }

    const QImage transformedImage = logicalImage.transformed(
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

// 生成并提交当前绝对旋转与双向翻转状态对应的显示图像。
bool ImageViewerWindow::applyImageTransform(
    double rotationAngleDegree,
    bool isVerticalFlipEnabled,
    bool isHorizontalFlipEnabled)
{
    const QImage logicalImage = createFlippedImage(
        isVerticalFlipEnabled,
        isHorizontalFlipEnabled);
    if (logicalImage.isNull())
    {
        return false;
    }

    QPolygonF validImagePolygon;
    QTransform currentToLogicalTransform;
    const QImage transformedImage = createRotatedImage(
        logicalImage,
        rotationAngleDegree,
        &validImagePolygon,
        &currentToLogicalTransform);
    if (transformedImage.isNull())
    {
        return false;
    }

    m_logicalImage = logicalImage;
    m_currentImage = transformedImage;
    m_validImagePolygon = validImagePolygon;
    m_currentToLogicalTransform = currentToLogicalTransform;
    m_currentRotationAngleDegree = rotationAngleDegree;
    m_isVerticalFlipEnabled = isVerticalFlipEnabled;
    m_isHorizontalFlipEnabled = isHorizontalFlipEnabled;
    updateFlipButtonUiState();
    m_uiPtr->gview_mainImage->setImage(
        QPixmap::fromImage(m_currentImage),
        m_currentToLogicalTransform,
        m_logicalImage.size());
    resetRoiPreview();
    if (!m_originalMaskImage.isNull())
    {
        applyMaskTransform(
            m_currentMaskRotationAngleDegree,
            m_isMaskVerticalFlipEnabled,
            m_isMaskHorizontalFlipEnabled,
            m_maskOpacityPercent);
    }
    return true;
}

// 将一位Mask按色表显示结果标准化为不含透明度的纯黑白图像。
QImage ImageViewerWindow::createBinaryMaskImage(const QImage& maskImage) const
{
    if (maskImage.isNull()
        || maskImage.depth() != MASK_REQUIRED_IMAGE_DEPTH)
    {
        return QImage();
    }

    QImage binaryMaskImage(maskImage.size(), QImage::Format_ARGB32);
    for (int pixelY = 0; pixelY < maskImage.height(); ++pixelY)
    {
        for (int pixelX = 0; pixelX < maskImage.width(); ++pixelX)
        {
            const QColor maskColor = maskImage.pixelColor(pixelX, pixelY);
            const bool isWhite = qGray(maskColor.rgb())
                >= MASK_WHITE_THRESHOLD;
            binaryMaskImage.setPixelColor(
                pixelX,
                pixelY,
                isWhite ? Qt::white : Qt::black);
        }
    }
    return binaryMaskImage;
}

// 从标准Mask依次生成中心翻转和绝对顺时针旋转结果。
QImage ImageViewerWindow::createTransformedMaskImage(
    double rotationAngleDegree,
    bool isVerticalFlipEnabled,
    bool isHorizontalFlipEnabled) const
{
    if (m_originalMaskImage.isNull())
    {
        return QImage();
    }

    QImage flippedMaskImage;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    Qt::Orientations flipOrientations;
    if (isVerticalFlipEnabled)
    {
        flipOrientations |= Qt::Horizontal;
    }
    if (isHorizontalFlipEnabled)
    {
        flipOrientations |= Qt::Vertical;
    }
    flippedMaskImage = flipOrientations == Qt::Orientations()
        ? m_originalMaskImage
        : m_originalMaskImage.flipped(flipOrientations);
#else
    flippedMaskImage = m_originalMaskImage.mirrored(
        isVerticalFlipEnabled,
        isHorizontalFlipEnabled);
#endif
    if (qAbs(rotationAngleDegree) < ROTATION_ANGLE_EPSILON)
    {
        return flippedMaskImage;
    }

    QTransform rotationTransform;
    rotationTransform.rotate(rotationAngleDegree);
    return flippedMaskImage.transformed(
        rotationTransform,
        Qt::FastTransformation);
}

// 生成与当前图片等大的Mask覆盖图，仅在Mask白色区域保留透明窗口。
QImage ImageViewerWindow::createMaskOverlayImage(
    const QImage& transformedMaskImage,
    int opacityPercent) const
{
    if (m_currentImage.isNull() || transformedMaskImage.isNull())
    {
        return QImage();
    }

    const int boundedOpacityPercent = qBound(0, opacityPercent, 100);
    const int blockerAlpha = qRound(
        EIGHT_BIT_ALPHA_MAXIMUM
        * static_cast<double>(boundedOpacityPercent) / 100.0);
    QImage maskOverlayImage(
        m_currentImage.size(),
        QImage::Format_ARGB32_Premultiplied);
    maskOverlayImage.fill(QColor(0, 0, 0, blockerAlpha));

    const int maskOffsetX =
        (m_currentImage.width() - transformedMaskImage.width()) / 2;
    const int maskOffsetY =
        (m_currentImage.height() - transformedMaskImage.height()) / 2;
    for (int maskPixelY = 0;
         maskPixelY < transformedMaskImage.height();
         ++maskPixelY)
    {
        const int overlayPixelY = maskOffsetY + maskPixelY;
        if (overlayPixelY < 0 || overlayPixelY >= maskOverlayImage.height())
        {
            continue;
        }
        for (int maskPixelX = 0;
             maskPixelX < transformedMaskImage.width();
             ++maskPixelX)
        {
            const int overlayPixelX = maskOffsetX + maskPixelX;
            if (overlayPixelX < 0 || overlayPixelX >= maskOverlayImage.width())
            {
                continue;
            }

            const QColor maskColor = transformedMaskImage.pixelColor(
                maskPixelX,
                maskPixelY);
            if (maskColor.alpha() > 0
                && qGray(maskColor.rgb()) >= MASK_WHITE_THRESHOLD)
            {
                maskOverlayImage.setPixel(
                    overlayPixelX,
                    overlayPixelY,
                    qRgba(0, 0, 0, 0));
            }
        }
    }
    return maskOverlayImage;
}

// 生成并提交Mask独立变换、Opacity和场景显示层。
bool ImageViewerWindow::applyMaskTransform(
    double rotationAngleDegree,
    bool isVerticalFlipEnabled,
    bool isHorizontalFlipEnabled,
    int opacityPercent)
{
    if (m_originalMaskImage.isNull() || m_currentImage.isNull())
    {
        return false;
    }

    const QImage transformedMaskImage = createTransformedMaskImage(
        rotationAngleDegree,
        isVerticalFlipEnabled,
        isHorizontalFlipEnabled);
    const int boundedOpacityPercent = qBound(0, opacityPercent, 100);
    const QImage maskOverlayImage = createMaskOverlayImage(
        transformedMaskImage,
        boundedOpacityPercent);
    if (maskOverlayImage.isNull())
    {
        return false;
    }

    m_currentMaskRotationAngleDegree = rotationAngleDegree;
    m_isMaskVerticalFlipEnabled = isVerticalFlipEnabled;
    m_isMaskHorizontalFlipEnabled = isHorizontalFlipEnabled;
    m_maskOpacityPercent = boundedOpacityPercent;
    m_uiPtr->gview_mainImage->setMaskOverlayImage(maskOverlayImage);
    updateMaskControlUiState();
    return true;
}

// 清除Mask数据与场景图层并恢复默认控制状态。
void ImageViewerWindow::clearMask()
{
    m_originalMaskImage = QImage();
    m_currentMaskRotationAngleDegree = 0.0;
    m_maskOpacityPercent = MASK_DEFAULT_OPACITY_PERCENT;
    m_isMaskVerticalFlipEnabled = false;
    m_isMaskHorizontalFlipEnabled = false;
    m_uiPtr->gview_mainImage->clearMaskOverlay();
    updateMaskControlUiState();
}

// 同步Mask控件使能状态、数值、勾选状态和英文动作文本。
void ImageViewerWindow::updateMaskControlUiState()
{
    const bool hasMaskImage = !m_originalMaskImage.isNull();
    m_uiPtr->sbox_maskOpacityPercent->setEnabled(hasMaskImage);
    m_uiPtr->btn_applyMaskOpacity->setEnabled(hasMaskImage);
    m_uiPtr->cbox_maskRotationAngleDegree->setEnabled(hasMaskImage);
    m_uiPtr->btn_applyMaskRotation->setEnabled(hasMaskImage);
    m_uiPtr->btn_maskVerticalFlip->setEnabled(hasMaskImage);
    m_uiPtr->btn_maskHorizontalFlip->setEnabled(hasMaskImage);

    m_uiPtr->sbox_maskOpacityPercent->setValue(m_maskOpacityPercent);
    const int maskRotationIndex = qBound(
        0,
        qRound(
            m_currentMaskRotationAngleDegree
            / ROTATION_ANGLE_STEP_DEGREE),
        m_uiPtr->cbox_maskRotationAngleDegree->count() - 1);
    m_uiPtr->cbox_maskRotationAngleDegree->setCurrentIndex(maskRotationIndex);
    m_uiPtr->btn_maskVerticalFlip->setChecked(
        m_isMaskVerticalFlipEnabled);
    m_uiPtr->btn_maskVerticalFlip->setText(
        m_isMaskVerticalFlipEnabled
            ? tr("Disable Vertical Flip")
            : tr("Vertical Flip"));
    m_uiPtr->btn_maskHorizontalFlip->setChecked(
        m_isMaskHorizontalFlipEnabled);
    m_uiPtr->btn_maskHorizontalFlip->setText(
        m_isMaskHorizontalFlipEnabled
            ? tr("Disable Horizontal Flip")
            : tr("Horizontal Flip"));
}

// 同步双向翻转按钮的勾选状态和英文动作文本。
void ImageViewerWindow::updateFlipButtonUiState()
{
    m_uiPtr->btn_verticalFlip->setChecked(m_isVerticalFlipEnabled);
    m_uiPtr->btn_verticalFlip->setText(
        m_isVerticalFlipEnabled
            ? tr("Disable Vertical Flip")
            : tr("Vertical Flip"));
    m_uiPtr->btn_horizontalFlip->setChecked(m_isHorizontalFlipEnabled);
    m_uiPtr->btn_horizontalFlip->setText(
        m_isHorizontalFlipEnabled
            ? tr("Disable Horizontal Flip")
            : tr("Horizontal Flip"));
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

// 维护最多选中一个且允许全部不选的图片工具状态。
void ImageViewerWindow::onImageToolModeButtonClicked(int buttonId)
{
    QAbstractButton* clickedButtonPtr =
        m_uiPtr->buttonGroup_imageToolMode->button(buttonId);
    if (clickedButtonPtr == nullptr)
    {
        return;
    }

    if (clickedButtonPtr->isChecked())
    {
        const QList<QAbstractButton*> toolButtons =
            m_uiPtr->buttonGroup_imageToolMode->buttons();
        for (QAbstractButton* toolButtonPtr : toolButtons)
        {
            if (toolButtonPtr != clickedButtonPtr)
            {
                toolButtonPtr->setChecked(false);
            }
        }
        m_uiPtr->gview_mainImage->setToolMode(
            static_cast<ImgGraphicsView::ToolMode>(buttonId));
    }
    else
    {
        m_uiPtr->gview_mainImage->setToolMode(
            ImgGraphicsView::ToolMode::None);
    }
    m_uiPtr->gview_mainImage->setFocus(Qt::ShortcutFocusReason);
}

// 显示Line按钮的参数生成右键菜单。
void ImageViewerWindow::onLineToolContextMenuRequested(
    const QPoint& buttonPosition)
{
    showParameterGenerationMenu(
        m_uiPtr->tbtn_drawLine,
        static_cast<int>(ImgGraphicsView::ToolMode::Line),
        buttonPosition);
}

// 显示Circle按钮的参数生成右键菜单。
void ImageViewerWindow::onCircleToolContextMenuRequested(
    const QPoint& buttonPosition)
{
    showParameterGenerationMenu(
        m_uiPtr->tbtn_drawCircle,
        static_cast<int>(ImgGraphicsView::ToolMode::Circle),
        buttonPosition);
}

// 显示Rect按钮的参数生成右键菜单。
void ImageViewerWindow::onRectToolContextMenuRequested(
    const QPoint& buttonPosition)
{
    showParameterGenerationMenu(
        m_uiPtr->tbtn_drawRect,
        static_cast<int>(ImgGraphicsView::ToolMode::Rect),
        buttonPosition);
}

// 显示Pick按钮的参数生成右键菜单。
void ImageViewerWindow::onPickToolContextMenuRequested(
    const QPoint& buttonPosition)
{
    showParameterGenerationMenu(
        m_uiPtr->tbtn_pickPixel,
        static_cast<int>(ImgGraphicsView::ToolMode::Pick),
        buttonPosition);
}

// 显示临时菜单并非模态打开对应工具的置顶参数窗口。
void ImageViewerWindow::showParameterGenerationMenu(
    QToolButton* toolButtonPtr,
    int toolModeId,
    const QPoint& buttonPosition)
{
    if (toolButtonPtr == nullptr)
    {
        return;
    }

    QMenu contextMenu(this);
    QAction* generateActionPtr =
        contextMenu.addAction(tr("Generate by Parameters..."));
    generateActionPtr->setEnabled(m_uiPtr->gview_mainImage->hasImage());
    QAction* selectedActionPtr = contextMenu.exec(
        toolButtonPtr->mapToGlobal(buttonPosition));
    if (selectedActionPtr != generateActionPtr)
    {
        return;
    }

    ParameterGenerationDialog* parameterDialogPtr =
        new ParameterGenerationDialog(
            static_cast<ImgGraphicsView::ToolMode>(toolModeId),
            m_uiPtr->gview_mainImage,
            this);
    parameterDialogPtr->show();
    parameterDialogPtr->raise();
    parameterDialogPtr->activateWindow();
}

// 显示Pick到的逻辑图像零基整数像素坐标。
void ImageViewerWindow::onPixelPicked(const QPoint& logicalPixelPosition)
{
    m_uiPtr->label_pickedPixel->setText(
        tr("X: %1,Y:%2")
            .arg(logicalPixelPosition.x())
            .arg(logicalPixelPosition.y()));
    updateIntensityProfiles(logicalPixelPosition);
}

// 将Pick标签和两个强度Plot恢复为无有效点状态。
void ImageViewerWindow::onPixelPickCleared()
{
    m_uiPtr->label_pickedPixel->setText(tr("X: --,Y:--"));
    clearIntensityProfiles();
}

// 从翻转后的逻辑图像生成当前Pick位置的X行与Y列强度曲线。
void ImageViewerWindow::updateIntensityProfiles(
    const QPoint& logicalPixelPosition)
{
    if (m_logicalImage.isNull()
        || !m_logicalImage.rect().contains(logicalPixelPosition)
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
                logicalPixelPosition.y(),
                ImageChannel::Gray));
        m_yProfilePlotPtr->setCurveSamples(
            QStringLiteral("gray"),
            createProfileSamples(
                ProfileAxis::Y,
                logicalPixelPosition.x(),
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
                logicalPixelPosition.y(),
                ImageChannel::Red));
        m_xProfilePlotPtr->setCurveSamples(
            QStringLiteral("green"),
            createProfileSamples(
                ProfileAxis::X,
                logicalPixelPosition.y(),
                ImageChannel::Green));
        m_xProfilePlotPtr->setCurveSamples(
            QStringLiteral("blue"),
            createProfileSamples(
                ProfileAxis::X,
                logicalPixelPosition.y(),
                ImageChannel::Blue));
        m_yProfilePlotPtr->setCurveSamples(
            QStringLiteral("red"),
            createProfileSamples(
                ProfileAxis::Y,
                logicalPixelPosition.x(),
                ImageChannel::Red));
        m_yProfilePlotPtr->setCurveSamples(
            QStringLiteral("green"),
            createProfileSamples(
                ProfileAxis::Y,
                logicalPixelPosition.x(),
                ImageChannel::Green));
        m_yProfilePlotPtr->setCurveSamples(
            QStringLiteral("blue"),
            createProfileSamples(
                ProfileAxis::Y,
                logicalPixelPosition.x(),
                ImageChannel::Blue));
    }

    const double intensityMaximum = isOriginalImageSixteenBit()
        ? SIXTEEN_BIT_INTENSITY_MAXIMUM
        : EIGHT_BIT_INTENSITY_MAXIMUM;
    m_xProfilePlotPtr->setAxisRanges(
        0.0,
        qMax(1, m_logicalImage.width() - 1),
        0.0,
        intensityMaximum);
    m_yProfilePlotPtr->setAxisRanges(
        0.0,
        qMax(1, m_logicalImage.height() - 1),
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

// 生成完整逻辑图像行或列的像素索引与通道强度样本。
QVector<QPointF> ImageViewerWindow::createProfileSamples(
    ProfileAxis profileAxis,
    int fixedPixelPosition,
    ImageChannel imageChannel) const
{
    QVector<QPointF> samples;
    if (m_logicalImage.isNull())
    {
        return samples;
    }

    const int sampleCount = profileAxis == ProfileAxis::X
        ? m_logicalImage.width()
        : m_logicalImage.height();
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

// 按逻辑图像实际像素格式读取八位或十六位通道强度。
double ImageViewerWindow::pixelIntensityAt(
    int pixelX,
    int pixelY,
    ImageChannel imageChannel) const
{
    if (imageChannel == ImageChannel::Gray)
    {
        if (m_logicalImage.format() == QImage::Format_Grayscale16)
        {
            const quint16* scanLinePtr = reinterpret_cast<const quint16*>(
                m_logicalImage.constScanLine(pixelY));
            return scanLinePtr[pixelX];
        }
        if (m_logicalImage.format() == QImage::Format_Alpha8)
        {
            return m_logicalImage.constScanLine(pixelY)[pixelX];
        }
        return qGray(m_logicalImage.pixel(pixelX, pixelY));
    }

    const QColor pixelColor = m_logicalImage.pixelColor(pixelX, pixelY);
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
