#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRectF>

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

signals:
    // ROI选择完成后发出场景坐标区域。
    void roiSelected(const QRectF& roiRect);

    // 图片显示倍率变化后发出实际倍率。
    void zoomChanged(double scaleFactor);

private:
    static constexpr double ZOOM_STEP_FACTOR = 1.15;
    static constexpr qreal MIN_ROI_SIZE = 5.0;
    static constexpr qreal ROI_PEN_WIDTH = 2.0;

    // 输入：无。
    // 输出：无。
    // 作用：按原图比例将当前图片适配到视图。
    void fitImageInView();

    // 输入：无。
    // 输出：无。
    // 作用：读取视图变换并发送实际倍率。
    void emitCurrentZoom();

    QGraphicsScene* m_scenePtr;             // 由QObject父子关系管理。
    QGraphicsPixmapItem* m_pixmapItemPtr;   // 由m_scenePtr管理。
    QGraphicsRectItem* m_roiItemPtr;        // 由m_scenePtr管理。
    QPointF m_roiStartPos;                  // ROI起始场景坐标。
    QPoint m_lastPanPos;                    // 上一次平移视图坐标。
    bool m_isDrawingRoi;                    // 是否正在绘制ROI。
    bool m_isPanning;                       // 是否正在平移图片。
};

} // namespace image_viewer
