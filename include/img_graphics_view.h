#pragma once

#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QTransform>
#include <QVector>

class QPaintEvent;
class QPainter;
class QKeyEvent;

namespace image_viewer {

class ImgGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    // 图片视图支持的互斥交互工具。
    enum class ToolMode
    {
        Roi,
        Line,
        Circle,
        Rect,
        Pick
    };

    // 创建图片交互视图并初始化场景。
    explicit ImgGraphicsView(QWidget* parent = nullptr);

    // 输入：需要显示的图片、当前画布到原图的变换和原图尺寸。
    // 输出：无。
    // 作用：替换当前图片、清除覆盖图形并自适应视图大小。
    void setImage(const QPixmap& pixmap,
                  const QTransform& currentToOriginalTransform,
                  const QSize& originalImageSize);

    // 输入：需要激活的互斥工具模式。
    // 输出：无。
    // 作用：结束未完成绘制、清除选择并切换左键行为。
    void setToolMode(ToolMode toolMode);

protected:
    // 处理滚轮事件并更新图片缩放倍率。
    void wheelEvent(QWheelEvent* eventPtr) override;

    // 处理鼠标按下事件并启动平移或ROI绘制。
    void mousePressEvent(QMouseEvent* eventPtr) override;

    // 处理鼠标移动事件并更新平移或ROI区域。
    void mouseMoveEvent(QMouseEvent* eventPtr) override;

    // 处理鼠标释放事件并提交有效ROI区域。
    void mouseReleaseEvent(QMouseEvent* eventPtr) override;

    // 处理Delete按键并删除当前工具对应的选中图形。
    void keyPressEvent(QKeyEvent* eventPtr) override;

    // 处理视图尺寸变化并重新自适应图片。
    void resizeEvent(QResizeEvent* eventPtr) override;

    // 绘制主场景并在需要时叠加缩略导航图。
    void paintEvent(QPaintEvent* eventPtr) override;

signals:
    // ROI选择完成后发出场景坐标区域。
    void roiSelected(const QRectF& roiRect);

    // 图片显示倍率变化后发出实际倍率。
    void zoomChanged(double scaleFactor);

    // Pick到有效原图像素后发出零基整数坐标。
    void pixelPicked(const QPoint& originalPixelPosition);

    // Pick点被替换为无效点或删除后通知清空坐标显示。
    void pixelPickCleared();

private:
    // 保存可删除绘图对象及其所属工具类型。
    struct DrawingItemRecord
    {
        ToolMode toolMode;
        QGraphicsItem* itemPtr;
    };

    // 单次滚轮缩放使用的倍率。
    static constexpr double ZOOM_STEP_FACTOR = 1.15;

    // ROI有效选择的最小边长，单位为场景坐标。
    static constexpr qreal MIN_ROI_SIZE = 5.0;

    // 所有覆盖线条固定保持的宽度，单位为视图像素。
    static constexpr qreal OVERLAY_PEN_WIDTH_VIEW_PIXELS = 2.0;

    // Pick标记固定保持的边长，单位为视图像素。
    static constexpr qreal PICK_MARKER_SIZE_VIEW_PIXELS = 4.0;

    // 图形单选使用的不可见命中半径，单位为视图像素。
    static constexpr qreal SELECTION_HIT_TOLERANCE_VIEW_PIXELS = 5.0;

    // 缩略导航图相对于视口尺寸的最大比例。
    static constexpr qreal NAVIGATION_THUMBNAIL_VIEWPORT_RATIO = 0.25;

    // 缩略导航图的最大宽度，单位为视口像素。
    static constexpr int NAVIGATION_THUMBNAIL_MAX_WIDTH = 200;

    // 缩略导航图的最大高度，单位为视口像素。
    static constexpr int NAVIGATION_THUMBNAIL_MAX_HEIGHT = 150;

    // 缩略导航图与视口边缘的间距，单位为视口像素。
    static constexpr int NAVIGATION_THUMBNAIL_MARGIN = 8;

    // 缩略导航图深色外边框的线宽，单位为视口像素。
    static constexpr qreal NAVIGATION_THUMBNAIL_BORDER_WIDTH = 1.0;

    // 黄色视口反馈框的线宽，单位为视口像素。
    static constexpr qreal NAVIGATION_VIEWPORT_PEN_WIDTH = 1.0;

    // 输入：无。
    // 输出：无。
    // 作用：按原图比例将当前图片适配到视图。
    void fitImageInView();

    // 输入：无。
    // 输出：无。
    // 作用：读取视图变换并发送实际倍率。
    void emitCurrentZoom();

    // 输入：无。
    // 输出：当前视图相对于图片场景的实际缩放倍率。
    // 作用：为覆盖图形线宽和命中范围提供统一倍率。
    qreal currentViewScale() const;

    // 输入：无。
    // 输出：当前倍率对应的Pick标记场景边长。
    // 作用：将固定四个视图像素换算为当前场景边长。
    qreal calculatePickMarkerSceneSize() const;

    // 输入：画笔颜色。
    // 输出：固定两个视图像素宽度的覆盖图形画笔。
    // 作用：统一ROI和普通绘图对象的固定显示粗度。
    QPen createOverlayPen(const QColor& color) const;

    // 输入：无。
    // 输出：无。
    // 作用：缩放变化后同步所有覆盖图形线宽和Pick标记大小。
    void updateOverlayAppearance();

    // 输入：需要限制的场景坐标。
    // 输出：限制在当前图片画布范围内的场景坐标。
    // 作用：防止ROI红框越过当前图片外接矩形。
    QPointF constrainScenePositionToImage(const QPointF& scenePosition) const;

    // 输入：视图坐标。
    // 输出：该坐标是否位于当前图片矩形画布内。
    // 作用：禁止从图片画布外开始绘制或Pick。
    bool isViewPositionInsideImage(const QPoint& viewPosition) const;

    // 输入：绘制工具、起始场景坐标和起始视图坐标。
    // 输出：无。
    // 作用：创建对应的实时预览图形并记录拖动状态。
    void beginDrawing(ToolMode toolMode,
                      const QPointF& scenePosition,
                      const QPoint& viewPosition);

    // 输入：当前场景坐标。
    // 输出：无。
    // 作用：按当前绘图类型和画布边界更新实时预览。
    void updateDrawingPreview(const QPointF& scenePosition);

    // 输入：无。
    // 输出：无。
    // 作用：删除未提交预览并结束本次绘制。
    void cancelDrawing();

    // 输入：无。
    // 输出：当前预览是否具有可提交的有效几何尺寸。
    // 作用：避免保存零长度线条、零半径圆或零尺寸矩形。
    bool isDrawingPreviewValid() const;

    // 输入：无。
    // 输出：无。
    // 作用：提交ROI或普通绘图，并使新普通绘图成为唯一选中项。
    void commitDrawingPreview();

    // 输入：无。
    // 输出：无。
    // 作用：删除当前实时预览图元并清空观察指针。
    void deleteDrawingPreview();

    // 输入：场景点击坐标。
    // 输出：无。
    // 作用：只命中当前Line、Circle或Rect类型中最上层的图形。
    void selectDrawingAt(const QPointF& scenePosition);

    // 输入：绘图记录和场景点击坐标。
    // 输出：点击位置是否落在该图形的不可见命中范围内。
    // 作用：为细线条提供固定五个视图像素的选择容差。
    bool isDrawingItemHit(const DrawingItemRecord& drawingItem,
                          const QPointF& scenePosition) const;

    // 输入：需要设为唯一选中项的图形观察指针。
    // 输出：无。
    // 作用：清除旧选择并更新Qt标准选择轮廓。
    void selectOnlyDrawingItem(QGraphicsItem* itemPtr);

    // 输入：无。
    // 输出：无。
    // 作用：取消当前普通绘图对象的选择状态。
    void clearDrawingSelection();

    // 输入：无。
    // 输出：无。
    // 作用：删除当前工具模式对应的唯一选中对象或Pick点。
    void deleteCurrentToolSelection();

    // 输入：当前画布中的点击坐标。
    // 输出：无。
    // 作用：映射并显示唯一天蓝色Pick点，黑色无效区会清除旧Pick。
    void pickPixelAt(const QPointF& scenePosition);

    // 输入：无。
    // 输出：无。
    // 作用：移除唯一Pick覆盖标记并清空保存位置。
    void clearPickedPixel();

    // 输入：无。
    // 输出：当前是否需要显示缩略导航图。
    // 作用：判断图片显示区域是否在任一方向超出视口。
    bool isNavigationThumbnailVisible() const;

    // 输入：无。
    // 输出：缩略导航图在视口中的绘制区域。
    // 作用：根据视口尺寸和原图比例计算右上角缩略图位置。
    QRect calculateNavigationThumbnailRect() const;

    // 输入：缩略导航图在视口中的绘制区域。
    // 输出：当前可见原图区域映射到缩略图后的矩形。
    // 作用：计算黄色视口反馈框的位置和大小。
    QRectF calculateNavigationViewportRect(const QRect& thumbnailRect) const;

    // 输入：用于绘制视口覆盖层的画笔。
    // 输出：无。
    // 作用：绘制完整原图缩略图、外边框和黄色视口框。
    void drawNavigationThumbnail(QPainter* painterPtr) const;

    QGraphicsScene* m_scenePtr;             // 由QObject父子关系管理。
    QGraphicsPixmapItem* m_pixmapItemPtr;   // 由m_scenePtr管理。
    QGraphicsRectItem* m_roiItemPtr;        // 由m_scenePtr管理。
    QGraphicsItem* m_drawingPreviewItemPtr; // 由m_scenePtr管理的实时预览。
    QGraphicsRectItem* m_pickItemPtr;        // 由m_scenePtr管理的唯一Pick标记。
    QGraphicsItem* m_selectedDrawingItemPtr; // 当前唯一选中的普通绘图观察指针。
    QVector<DrawingItemRecord> m_drawingItems; // 当前画布中的可删除绘图集合。
    QTransform m_currentToOriginalTransform;   // 当前画布坐标到原图坐标的变换。
    QSize m_originalImageSize;                 // 原图像素尺寸。
    ToolMode m_toolMode;                       // 当前激活的互斥工具模式。
    ToolMode m_activeDrawingMode;              // 本次实时预览所属工具模式。
    QPointF m_drawingStartPos;                 // 本次绘制起始场景坐标。
    QPointF m_pickSceneCenter;                 // 当前Pick标记的场景中心。
    QPoint m_drawingStartViewPos;              // 本次绘制起始视图坐标。
    QPoint m_lastPanPos;                    // 上一次平移视图坐标。
    bool m_isDrawing;                       // 是否存在左键绘制候选或预览。
    bool m_hasExceededDragThreshold;        // 是否已超过系统拖动阈值。
    bool m_isPanning;                       // 是否正在平移图片。
};

} // namespace image_viewer
