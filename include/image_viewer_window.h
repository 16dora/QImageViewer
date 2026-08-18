#pragma once

#include <QMainWindow>
#include <QPixmap>
#include <QRectF>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class ImageViewerWindow; }
QT_END_NAMESPACE

namespace image_viewer {

class ImageViewerWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 创建主窗口并初始化界面信号连接。
    explicit ImageViewerWindow(QWidget* parent = nullptr);

    // 释放由智能指针管理的UI对象。
    ~ImageViewerWindow() override;

private slots:
    // 响应加载图片按钮并更新主图片视图。
    void on_btn_loadImage_clicked();

    // 接收ROI场景区域并更新右侧预览。
    void onRoiSelected(const QRectF& roiRect);

    // 接收实际缩放倍率并更新倍率Label。
    void onZoomChanged(double scaleFactor);

private:
    static constexpr int IMAGE_LOAD_ERROR_CODE = 1001;
    static constexpr int STATUS_MESSAGE_TIMEOUT_MS = 5000;

    std::unique_ptr<Ui::ImageViewerWindow> m_uiPtr; // 独占UI对象。
    QPixmap m_originalPixmap;                      // 当前加载的原始图片。
};

} // namespace image_viewer
