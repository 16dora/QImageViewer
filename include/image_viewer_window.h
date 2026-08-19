#pragma once

#include <QImage>
#include <QMainWindow>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QPolygonF>
#include <QRect>
#include <QRectF>
#include <QTransform>
#include <QVector>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class ImageViewerWindow; }
QT_END_NAMESPACE

class MultiCurvePlot;

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

    // 接收工具按钮组编号并切换图片视图左键模式。
    void onImageToolModeButtonClicked(int buttonId);

    // 接收Pick到的原图零基像素坐标并更新标签。
    void onPixelPicked(const QPoint& originalPixelPosition);

    // Pick点被清除后将坐标标签恢复为无效状态。
    void onPixelPickCleared();

private:
    // 强度剖面的原图坐标轴方向。
    enum class ProfileAxis
    {
        X,
        Y
    };

    // 强度剖面支持的单通道类型。
    enum class ImageChannel
    {
        Gray,
        Red,
        Green,
        Blue
    };

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

    // 八位图像通道的最大原始强度。
    static constexpr int EIGHT_BIT_INTENSITY_MAXIMUM = 255;

    // 十六位图像通道的最大原始强度。
    static constexpr int SIXTEEN_BIT_INTENSITY_MAXIMUM = 65535;

    // 强度剖面曲线的固定线宽，单位为Plot视图像素。
    static constexpr int PROFILE_CURVE_LINE_WIDTH = 1;

    // 强度剖面不显示独立采样点。
    static constexpr int PROFILE_CURVE_POINT_SIZE = 0;

    // 输入：目标顺时针旋转角度、有效内容多边形和逆变换输出指针。
    // 输出：使用黑色背景承载的旋转图像。
    // 作用：始终从原图数据生成指定绝对角度的当前图像及Pick坐标变换。
    QImage createRotatedImage(
        double rotationAngleDegree,
        QPolygonF* validImagePolygonPtr,
        QTransform* currentToOriginalTransformPtr) const;

    // 输入：无。
    // 输出：无。
    // 作用：创建两个Plot控制器并注册灰度与RGB曲线。
    void initializeIntensityPlots();

    // 输入：需要注册曲线的Plot控制器。
    // 输出：无。
    // 作用：使用统一样式注册Gray、R、G和B四条稳定ID曲线。
    void addIntensityProfileCurves(MultiCurvePlot* profilePlotPtr);

    // 输入：当前Pick到的原图零基像素坐标。
    // 输出：无。
    // 作用：从未旋转原图生成X行和Y列的灰度或RGB强度曲线。
    void updateIntensityProfiles(const QPoint& originalPixelPosition);

    // 输入：无。
    // 输出：无。
    // 作用：清除两个Plot的数据和全部通道图例。
    void clearIntensityProfiles();

    // 输入：剖面方向、固定坐标和需要读取的图像通道。
    // 输出：以原图像素索引为横坐标的强度样本。
    // 作用：统一生成完整原图行或列的Plot数据。
    QVector<QPointF> createProfileSamples(
        ProfileAxis profileAxis,
        int fixedPixelPosition,
        ImageChannel imageChannel) const;

    // 输入：原图零基像素坐标和通道。
    // 输出：保持原始八位或十六位范围的通道强度。
    // 作用：按QImage像素格式准确读取灰度或RGB值。
    double pixelIntensityAt(
        int pixelX,
        int pixelY,
        ImageChannel imageChannel) const;

    // 输入：无。
    // 输出：当前原图是否为有效单通道灰度格式。
    // 作用：决定显示Gray曲线还是RGB三曲线。
    bool isOriginalImageGrayscale() const;

    // 输入：无。
    // 输出：当前原图通道是否保留十六位整数强度。
    // 作用：决定取样方式和Plot纵轴范围。
    bool isOriginalImageSixteenBit() const;

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
    std::unique_ptr<MultiCurvePlot> m_xProfilePlotPtr; // 原图X行强度Plot控制器。
    std::unique_ptr<MultiCurvePlot> m_yProfilePlotPtr; // 原图Y列强度Plot控制器。
    QImage m_originalImage;                        // 当前加载且保持格式的原始图像数据。
    QImage m_currentImage;                         // 当前绝对角度对应的旋转图像数据。
    QImage m_currentRoiImage;                      // 不包含任何辅助标记的当前ROI数据。
    QPolygonF m_validImagePolygon;                 // 旋转后真实原图内容的闭合轮廓。
    QTransform m_currentToOriginalTransform;       // 当前旋转画布到原图的Pick变换。
    double m_currentRotationAngleDegree;           // 当前顺时针绝对旋转角度。
};

} // namespace image_viewer
