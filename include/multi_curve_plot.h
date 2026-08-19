#pragma once

#include <memory>
#include <vector>

#include <QColor>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>

class QwtLegend;
class QwtPlot;
class QwtPlotGrid;
class QwtPlotMagnifier;
class QwtPlotPanner;

class MultiCurvePlot : public QObject
{
public:
    // 多曲线图表初始化参数。
    struct Options
    {
        // 图表标题。
        QString title;
        // X 轴标题。
        QString xAxisTitle;
        // Y 轴标题。
        QString yAxisTitle;
        // 是否显示曲线图例。
        bool isLegendVisible = true;
    };

    // 单条曲线的显示参数。
    struct CurveOptions
    {
        // 图例中显示的曲线标题。
        QString title;
        // 折线颜色。
        QColor lineColor = QColor(44, 123, 182);
        // 采样点颜色。
        QColor pointColor = QColor(44, 123, 182);
        // 折线宽度。
        int lineWidth = 2;
        // 圆形采样点直径。
        int pointSize = 7;
        // 初始是否显示曲线。
        bool isVisible = true;
    };

    // 构造多曲线控制器。
    // 输入：plotPtr 为 .ui 中的 QwtPlot，options 为图表参数。
    // 输出：无。
    // 作用：初始化数值坐标、网格、图例和鼠标交互。
    explicit MultiCurvePlot(
        QwtPlot* plotPtr,
        Options options,
        QObject* parent = nullptr);

    // 析构多曲线控制器。
    // 输入：无。
    // 输出：无。
    // 作用：解除所有 Qwt 显示项与外部 plot 的绑定。
    ~MultiCurvePlot() override;

    MultiCurvePlot(const MultiCurvePlot& other) = delete;
    MultiCurvePlot& operator=(const MultiCurvePlot& other) = delete;

    // 注册一条稳定 ID 的曲线。
    // 输入：curveId 为唯一字符串，options 为显示参数。
    // 输出：新增成功返回 true，ID 为空或重复返回 false。
    // 作用：支持调用方按业务 ID 管理任意数量的数据集。
    bool addCurve(
        const QString& curveId,
        const CurveOptions& options);

    // 整体替换指定曲线的采样点。
    // 输入：curveId 和新的样本集合。
    // 输出：无。
    // 作用：同步缓存、Qwt 曲线数据并刷新图表。
    void setCurveSamples(
        const QString& curveId,
        const QVector<QPointF>& samples);

    // 向指定曲线追加一个采样点。
    // 输入：curveId 和 sample。
    // 输出：无。
    // 作用：用于采集阶段增量更新实测曲线。
    void appendCurveSample(
        const QString& curveId,
        const QPointF& sample);

    // 获取指定曲线当前实际持有的数据。
    // 输入：curveId 为稳定曲线 ID。
    // 输出：返回 Qwt 数据对象中的当前采样点副本。
    // 作用：确保曲线 CSV 保存当前显示快照。
    QVector<QPointF> curveSamples(
        const QString& curveId) const;

    // 清空指定曲线。
    // 输入：curveId 为稳定曲线 ID。
    // 输出：无。
    // 作用：保留曲线配置，只清除数据。
    void clearCurve(const QString& curveId);

    // 清空全部曲线。
    // 输入：无。
    // 输出：无。
    // 作用：保留全部曲线配置并一次刷新图表。
    void clear();

    // 设置指定曲线是否显示并同步图例条目。
    // 输入：curveId 为稳定曲线 ID，isVisible 为目标状态。
    // 输出：无。
    // 作用：在灰度与RGB图像之间只展示有效通道。
    void setCurveVisible(
        const QString& curveId,
        bool isVisible);

    // 固定横纵轴的数据范围。
    // 输入：横轴和纵轴的最小值与最大值。
    // 输出：无。
    // 作用：完整显示原图像素索引和原始位深强度范围。
    void setAxisRanges(
        double xMinimum,
        double xMaximum,
        double yMinimum,
        double yMaximum);

    // 替换数值坐标轴标题。
    // 输入：xAxisTitle 和 yAxisTitle。
    // 输出：无。
    // 作用：支持原始、正向和反向视图复用同一图表。
    void setAxisTitles(
        const QString& xAxisTitle,
        const QString& yAxisTitle);

    // 恢复默认完整数据视图。
    // 输入：无。
    // 输出：无。
    // 作用：重新启用 X/Y 自动缩放并显示当前全部数据。
    void resetView();

private:
    struct CurveEntry;

    // 初始化 Qwt 公共显示项和交互控制器。
    // 输入：无。
    // 输出：无。
    // 作用：绑定坐标标题、网格、图例、缩放和平移。
    void initializePlot();

    // 查找指定 ID 的曲线条目。
    // 输入：curveId 为稳定曲线 ID。
    // 输出：找到时返回条目指针，否则返回空。
    // 作用：集中完成所有曲线操作的 ID 解析。
    CurveEntry* findCurveEntry(const QString& curveId);
    const CurveEntry* findCurveEntry(const QString& curveId) const;

private:
    // 由外部 .ui 持有生命周期的 QwtPlot。
    QwtPlot* m_plotPtr = nullptr;
    // 图表初始化参数。
    Options m_options;
    // Qwt 网格显示项。
    std::unique_ptr<QwtPlotGrid> m_gridPtr;
    // Qwt 图例，由 QwtPlot 持有生命周期。
    QwtLegend* m_legendPtr = nullptr;
    // 鼠标滚轮缩放控制器。
    std::unique_ptr<QwtPlotMagnifier> m_magnifierPtr;
    // 鼠标拖拽平移控制器。
    std::unique_ptr<QwtPlotPanner> m_pannerPtr;
    // 按稳定 ID 保存的全部曲线。
    std::vector<std::unique_ptr<CurveEntry>> m_curveEntries;
};
