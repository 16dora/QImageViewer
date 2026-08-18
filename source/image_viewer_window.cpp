#include "image_viewer_window.h"

#include "img_graphics_view.h"
#include "ui_image_viewer_window.h"

#include <QFileDialog>
#include <QStatusBar>

namespace image_viewer {

// 创建主窗口、初始化UI，并建立图片视图信号连接。
ImageViewerWindow::ImageViewerWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_uiPtr(std::make_unique<Ui::ImageViewerWindow>())
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
    m_uiPtr->gview_mainImage->setImage(m_originalPixmap);
    m_uiPtr->label_roiPreview->clear();
    m_uiPtr->label_roiPreview->setText(tr("ROI"));
}

// 将ROI限制在原图范围内，并更新右侧缩略预览。
void ImageViewerWindow::onRoiSelected(const QRectF& roiRect)
{
    if (m_originalPixmap.isNull())
    {
        return;
    }

    const QRectF boundedRoi = roiRect.intersected(QRectF(m_originalPixmap.rect()));
    const QPixmap roiPixmap = m_originalPixmap.copy(boundedRoi.toRect());
    if (roiPixmap.isNull())
    {
        return;
    }

    const QSize previewSize = m_uiPtr->label_roiPreview->contentsRect().size();
    m_uiPtr->label_roiPreview->setPixmap(
        roiPixmap.scaled(previewSize,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation));
}

// 将实际倍率格式化为带单位的英文文本。
void ImageViewerWindow::onZoomChanged(double scaleFactor)
{
    m_uiPtr->label_zoom->setText(tr("Zoom: %1x").arg(scaleFactor, 0, 'f', 2));
}

} // namespace image_viewer
