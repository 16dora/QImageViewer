#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>

class QPaintEvent;
class QPainter;

namespace image_viewer {

class ImgGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    // 创建图片交互视图并初始化场景。
    explicit ImgGraphicsView(QWidget* parent = nullptr);

    // 输入：需要显示的图片。
    // 输出：无。
    // 作用：替换当前图片并自适应视图大小。
    void setImage(const QPixmap& pixmap);

protected:
    // 处理滚轮事件并更新图片缩放倍率。
    void wheelEvent(QWheelEvent* eventPtr) override;

    // 处理鼠标按下事件并启动平移或ROI绘制。
    void mousePressEvent(QMouseEvent* eventPtr) override;

    // 处理鼠标移动事件并更新平移或ROI区域。
    void mouseMoveEvent(QMouseEvent* eventPtr) override;

    // 处理鼠标释放事件并提交有效ROI区域。
    void mouseReleaseEvent(QMouseEvent* eventPtr) override;

    // 处理视图尺寸变化并重新自适应图片。
    void resizeEvent(QResizeEvent* eventPtr) override;

    // 绘制主场景并在需要时叠加缩略导航图。
    void paintEvent(QPaintEvent* eventPtr) override;

signals:
    // ROI选择完成后发出场景坐标区域。
    void roiSelected(const QRectF& roiRect);

    // 图片显示倍率变化后发出实际倍率。
    void zoomChanged(double scaleFactor);

private:
    // 单次滚轮缩放使用的倍率。
    static constexpr double ZOOM_STEP_FACTOR = 1.15;

    // ROI有效选择的最小边长，单位为场景坐标。
    static constexpr qreal MIN_ROI_SIZE = 5.0;

    // ROI红色边框的线宽，单位为场景坐标。
    static constexpr qreal ROI_PEN_WIDTH = 2.0;

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

    // 输入：需要限制的场景坐标。
    // 输出：限制在当前图片画布范围内的场景坐标。
    // 作用：防止ROI红框越过当前图片外接矩形。
    QPointF constrainScenePositionToImage(const QPointF& scenePosition) const;

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
    QPointF m_roiStartPos;                  // ROI起始场景坐标。
    QPoint m_lastPanPos;                    // 上一次平移视图坐标。
    bool m_isDrawingRoi;                    // 是否正在绘制ROI。
    bool m_isPanning;                       // 是否正在平移图片。
};

} // namespace image_viewer
