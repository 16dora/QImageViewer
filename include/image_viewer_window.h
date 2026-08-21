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
class QToolButton;

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

    // 将图片和全部交互状态恢复为刚加载后的初始形式。
    void on_tbtn_resetView_clicked();

    // 将当前变换图像按所选格式保存到应用程序Save目录。
    void on_btn_saveImage_clicked();

    // 选择并加载一张实际位深为1的BMP Mask图像。
    void on_btn_loadMaskFile_clicked();

    // 清除Mask图像、覆盖层和全部Mask变换参数。
    void on_btn_clearMask_clicked();

    // 保留Mask图像并将全部Mask变换参数恢复默认值。
    void on_btn_resetMask_clicked();

    // 应用当前Mask不透明度百分比。
    void on_btn_applyMaskOpacity_clicked();

    // 应用当前Mask绝对顺时针旋转角度。
    void on_btn_applyMaskRotation_clicked();

    // 根据按钮状态启用或取消Mask左右翻转。
    void on_btn_maskVerticalFlip_clicked(bool isChecked);

    // 根据按钮状态启用或取消Mask上下翻转。
    void on_btn_maskHorizontalFlip_clicked(bool isChecked);

    // 根据按钮状态启用或取消绕竖直中轴的左右翻转。
    void on_btn_verticalFlip_clicked(bool isChecked);

    // 根据按钮状态启用或取消绕水平中轴的上下翻转。
    void on_btn_horizontalFlip_clicked(bool isChecked);

    // 接收ROI场景区域并更新右侧预览。
    void onRoiSelected(const QRectF& roiRect);

    // 接收实际缩放倍率并更新倍率Label。
    void onZoomChanged(double scaleFactor);

    // 接收工具按钮组编号并切换图片视图左键模式。
    void onImageToolModeButtonClicked(int buttonId);

    // 接收Pick到的逻辑图像零基像素坐标并更新标签。
    void onPixelPicked(const QPoint& logicalPixelPosition);

    // Pick点被清除后将坐标标签恢复为无效状态。
    void onPixelPickCleared();

    // 显示Line按钮的参数生成右键菜单。
    void onLineToolContextMenuRequested(const QPoint& buttonPosition);

    // 显示Circle按钮的参数生成右键菜单。
    void onCircleToolContextMenuRequested(const QPoint& buttonPosition);

    // 显示Rect按钮的参数生成右键菜单。
    void onRectToolContextMenuRequested(const QPoint& buttonPosition);

    // 显示Pick按钮的参数生成右键菜单。
    void onPickToolContextMenuRequested(const QPoint& buttonPosition);

private:
    // 强度剖面的逻辑图像坐标轴方向。
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

    // 主图更新时对现有交互内容采用的处理方式。
    enum class ImageUpdateMode
    {
        PreserveContent,
        ResetContent
    };

    // 图片加载失败时使用的错误码。
    static constexpr int IMAGE_LOAD_ERROR_CODE = 1001;

    // 状态栏错误消息的显示时长，单位为毫秒。
    static constexpr int STATUS_MESSAGE_TIMEOUT_MS = 5000;

    // 旋转角度下拉框相邻选项的角度步进，单位为度。
    static constexpr double ROTATION_ANGLE_STEP_DEGREE = 22.5;

    // 判断两个旋转角度相同时使用的误差，单位为度。
    static constexpr double ROTATION_ANGLE_EPSILON = 1.0e-6;

    // Mask文件允许的唯一位深。
    static constexpr int MASK_REQUIRED_IMAGE_DEPTH = 1;

    // Mask像素二值化使用的白色阈值。
    static constexpr int MASK_WHITE_THRESHOLD = 128;

    // Mask默认完全遮挡时的不透明度百分比。
    static constexpr int MASK_DEFAULT_OPACITY_PERCENT = 100;

    // 八位Alpha通道的最大不透明值。
    static constexpr int EIGHT_BIT_ALPHA_MAXIMUM = 255;

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

    // 输入：两个翻转状态。
    // 输出：以翻转后左上角为原点的逻辑图像。
    // 作用：从原图生成保持原始尺寸、格式和位深的Plot数据源。
    QImage createFlippedImage(
        bool isVerticalFlipEnabled,
        bool isHorizontalFlipEnabled) const;

    // 输入：逻辑图像、目标顺时针旋转角度和变换结果输出指针。
    // 输出：使用黑色背景承载的绝对角度旋转图像。
    // 作用：只旋转逻辑图像并生成当前画布到逻辑图像的Pick变换。
    QImage createRotatedImage(
        const QImage& logicalImage,
        double rotationAngleDegree,
        QPolygonF* validImagePolygonPtr,
        QTransform* currentToLogicalTransformPtr) const;

    // 输入：目标顺时针旋转角度、两个翻转状态和交互内容处理方式。
    // 输出：成功生成并显示组合变换图像时返回true。
    // 作用：集中提交图像、Pick变换、按钮状态和对应交互内容状态。
    bool applyImageTransform(
        double rotationAngleDegree,
        bool isVerticalFlipEnabled,
        bool isHorizontalFlipEnabled,
        ImageUpdateMode imageUpdateMode);

    // 输入：已通过位深校验的单色Mask图像。
    // 输出：使用纯黑和纯白像素表示的标准Mask图像。
    // 作用：忽略输入色表透明度并统一后续变换语义。
    QImage createBinaryMaskImage(const QImage& maskImage) const;

    // 输入：Mask绝对角度和两个独立翻转状态。
    // 输出：从原始Mask生成的独立变换结果。
    // 作用：依次执行中心翻转和绝对顺时针旋转，不继承原图变换。
    QImage createTransformedMaskImage(
        double rotationAngleDegree,
        bool isVerticalFlipEnabled,
        bool isHorizontalFlipEnabled) const;

    // 输入：已变换Mask和黑色区域不透明度百分比。
    // 输出：与当前图片画布等大的透明黑色覆盖图。
    // 作用：居中Mask并将白色区域设为透明，边界外保持黑色遮挡。
    QImage createMaskOverlayImage(
        const QImage& transformedMaskImage,
        int opacityPercent) const;

    // 输入：Mask绝对角度、双向翻转和不透明度状态。
    // 输出：成功生成并显示Mask覆盖图时返回true。
    // 作用：集中提交Mask独立变换状态和UI状态。
    bool applyMaskTransform(
        double rotationAngleDegree,
        bool isVerticalFlipEnabled,
        bool isHorizontalFlipEnabled,
        int opacityPercent);

    // 输入：无。
    // 输出：无。
    // 作用：清除Mask数据、场景图层并恢复Mask控件默认状态。
    void clearMask();

    // 输入：无。
    // 输出：无。
    // 作用：根据是否存在Mask同步相关控件使能、数值和按钮文本。
    void updateMaskControlUiState();

    // 输入：无。
    // 输出：无。
    // 作用：根据已提交的翻转状态同步两个切换按钮的选中状态和文本。
    void updateFlipButtonUiState();

    // 输入：被右键点击的工具按钮、工具编号和按钮局部坐标。
    // 输出：无。
    // 作用：显示临时菜单并非模态打开置顶参数窗口。
    void showParameterGenerationMenu(
        QToolButton* toolButtonPtr,
        int toolModeId,
        const QPoint& buttonPosition);

    // 输入：无。
    // 输出：无。
    // 作用：创建两个Plot控制器并注册灰度与RGB曲线。
    void initializeIntensityPlots();

    // 输入：需要注册曲线的Plot控制器。
    // 输出：无。
    // 作用：使用统一样式注册Gray、R、G和B四条稳定ID曲线。
    void addIntensityProfileCurves(MultiCurvePlot* profilePlotPtr);

    // 输入：当前Pick到的逻辑图像零基像素坐标。
    // 输出：无。
    // 作用：从翻转后的逻辑图像生成X行和Y列的灰度或RGB强度曲线。
    void updateIntensityProfiles(const QPoint& logicalPixelPosition);

    // 输入：无。
    // 输出：无。
    // 作用：清除两个Plot的数据和全部通道图例。
    void clearIntensityProfiles();

    // 输入：剖面方向、固定坐标和需要读取的图像通道。
    // 输出：以逻辑图像像素索引为横坐标的强度样本。
    // 作用：统一生成完整逻辑图像行或列的Plot数据。
    QVector<QPointF> createProfileSamples(
        ProfileAxis profileAxis,
        int fixedPixelPosition,
        ImageChannel imageChannel) const;

    // 输入：逻辑图像零基像素坐标和通道。
    // 输出：保持原始八位或十六位范围的通道强度。
    // 作用：按逻辑图像的QImage像素格式准确读取灰度或RGB值。
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
    std::unique_ptr<MultiCurvePlot> m_xProfilePlotPtr; // 逻辑图像X行强度Plot控制器。
    std::unique_ptr<MultiCurvePlot> m_yProfilePlotPtr; // 逻辑图像Y列强度Plot控制器。
    QImage m_originalImage;                        // 当前加载且保持格式的原始图像数据。
    QImage m_logicalImage;                         // 以翻转后左上角为原点的Plot数据源。
    QImage m_currentImage;                         // 逻辑图像绝对旋转后的显示数据。
    QImage m_currentRoiImage;                      // 不包含任何辅助标记的当前ROI数据。
    QImage m_originalMaskImage;                    // 标准纯黑白且不继承原图变换的Mask数据。
    QPolygonF m_validImagePolygon;                 // 旋转后逻辑图像内容的闭合轮廓。
    QTransform m_currentToLogicalTransform;        // 当前画布到逻辑图像的Pick变换。
    QPoint m_pickedOriginalPixelPosition;          // Pick对应的未翻转原图零基像素坐标。
    double m_currentRotationAngleDegree;           // 当前顺时针绝对旋转角度。
    double m_currentMaskRotationAngleDegree;       // 当前Mask顺时针绝对旋转角度。
    int m_maskOpacityPercent;                      // 当前Mask黑色区域不透明度百分比。
    bool m_isVerticalFlipEnabled;                  // 是否绕竖直中轴左右翻转。
    bool m_isHorizontalFlipEnabled;                // 是否绕水平中轴上下翻转。
    bool m_isMaskVerticalFlipEnabled;              // Mask是否绕竖直中轴左右翻转。
    bool m_isMaskHorizontalFlipEnabled;            // Mask是否绕水平中轴上下翻转。
};

} // namespace image_viewer
