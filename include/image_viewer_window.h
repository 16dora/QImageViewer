#pragma once

#include <QMainWindow>
#include <QPixmap>
#include <QPolygonF>
#include <QRect>
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

    // 按下旋转按钮后将原图旋转到当前选择的绝对角度。
    void on_btn_rotateImage_clicked();

    // 接收ROI场景区域并更新右侧预览。
    void onRoiSelected(const QRectF& roiRect);

    // 接收实际缩放倍率并更新倍率Label。
    void onZoomChanged(double scaleFactor);

private:
    // 图片加载失败时使用的错误码。
    static constexpr int IMAGE_LOAD_ERROR_CODE = 1001;

    // 状态栏错误消息的显示时长，单位为毫秒。
    static constexpr int STATUS_MESSAGE_TIMEOUT_MS = 5000;

    // 旋转角度下拉框相邻选项的角度步进，单位为度。
    static constexpr double ROTATION_ANGLE_STEP_DEGREE = 22.5;

    // 判断两个旋转角度相同时使用的误差，单位为度。
    static constexpr double ROTATION_ANGLE_EPSILON = 1.0e-6;

    // ROI预览中有效内容蓝色轮廓的线宽，单位为预览像素。
    static constexpr qreal ROI_VALID_CONTENT_PEN_WIDTH = 1.0;

    // 蓝色轮廓相对ROI预览边缘的内缩距离，单位为预览像素。
    static constexpr qreal ROI_VALID_CONTENT_INSET = 0.5;

    // 输入：目标顺时针旋转角度和有效内容多边形输出指针。
    // 输出：使用黑色背景承载的旋转图片。
    // 作用：始终从原图生成指定绝对角度的当前图片。
    QPixmap createRotatedPixmap(
        double rotationAngleDegree,
        QPolygonF* validImagePolygonPtr) const;

    // 输入：无。
    // 输出：无。
    // 作用：清除底层ROI数据并恢复右侧预览初始状态。
    void resetRoiPreview();

    // 输入：ROI预览副本和ROI在当前图片中的像素区域。
    // 输出：无。
    // 作用：仅在部分包含黑色区域时绘制有效内容闭合蓝色轮廓。
    void drawRoiValidContentOutline(
        QPixmap* previewPixmapPtr,
        const QRect& roiRect) const;

    std::unique_ptr<Ui::ImageViewerWindow> m_uiPtr; // 独占UI对象。
    QPixmap m_originalPixmap;                      // 当前加载且保持不变的原始图片。
    QPixmap m_currentPixmap;                       // 当前绝对角度对应的旋转图片。
    QPixmap m_currentRoiPixmap;                    // 不包含蓝色标记的当前ROI数据。
    QPolygonF m_validImagePolygon;                 // 旋转后真实原图内容的闭合轮廓。
    double m_currentRotationAngleDegree;           // 当前顺时针绝对旋转角度。
};

} // namespace image_viewer
