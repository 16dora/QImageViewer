#include "multi_curve_plot.h"

#include <cstddef>
#include <utility>

#include <QBrush>
#include <QPen>
#include <QSize>

#include <qwt_legend.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_series_data.h>
#include <qwt_symbol.h>

struct MultiCurvePlot::CurveEntry
{
    QString curveId;
    QVector<QPointF> samples;
    std::unique_ptr<QwtPlotCurve> curvePtr;
};

// 构造数值型多曲线控制器。
MultiCurvePlot::MultiCurvePlot(
    QwtPlot* plotPtr,
    Options options,
    QObject* parent)
    : QObject(parent),
      m_plotPtr(plotPtr),
      m_options(std::move(options))
{
    initializePlot();
}

// 解除曲线、网格和图例与外部 plot 的绑定。
MultiCurvePlot::~MultiCurvePlot()
{
    for (const std::unique_ptr<CurveEntry>& entry : m_curveEntries)
    {
        if (entry != nullptr && entry->curvePtr != nullptr)
        {
            entry->curvePtr->detach();
        }
    }
    if (m_gridPtr != nullptr)
    {
        m_gridPtr->detach();
    }
    if (m_plotPtr != nullptr && m_legendPtr != nullptr)
    {
        m_plotPtr->insertLegend(nullptr);
        m_legendPtr = nullptr;
    }
}

// 注册一条由稳定字符串 ID 管理的新曲线。
bool MultiCurvePlot::addCurve(
    const QString& curveId,
    const CurveOptions& options)
{
    if (m_plotPtr == nullptr ||
        curveId.isEmpty() ||
        findCurveEntry(curveId) != nullptr)
    {
        return false;
    }

    std::unique_ptr<CurveEntry> entry =
        std::make_unique<CurveEntry>();
    entry->curveId = curveId;
    entry->curvePtr = std::make_unique<QwtPlotCurve>(
        options.title.isEmpty() ? curveId : options.title);
    entry->curvePtr->setRenderHint(
        QwtPlotItem::RenderAntialiased,
        true);
    entry->curvePtr->setStyle(QwtPlotCurve::Lines);
    entry->curvePtr->setPen(QPen(
        options.lineColor,
        options.lineWidth > 0 ? options.lineWidth : 1));
    if (options.pointSize > 0)
    {
        entry->curvePtr->setSymbol(new QwtSymbol(
            QwtSymbol::Ellipse,
            QBrush(options.pointColor),
            QPen(options.pointColor),
            QSize(options.pointSize, options.pointSize)));
    }
    else
    {
        entry->curvePtr->setSymbol(nullptr);
    }
    entry->curvePtr->setVisible(options.isVisible);
    entry->curvePtr->setItemAttribute(
        QwtPlotItem::Legend,
        options.isVisible);
    entry->curvePtr->attach(m_plotPtr);
    m_curveEntries.push_back(std::move(entry));
    m_plotPtr->replot();
    return true;
}

// 整体替换指定曲线的采样点。
void MultiCurvePlot::setCurveSamples(
    const QString& curveId,
    const QVector<QPointF>& samples)
{
    CurveEntry* entry = findCurveEntry(curveId);
    if (entry == nullptr || entry->curvePtr == nullptr)
    {
        return;
    }
    entry->samples = samples;
    entry->curvePtr->setSamples(entry->samples);
    if (m_plotPtr != nullptr)
    {
        m_plotPtr->replot();
    }
}

// 向指定曲线追加一个采样点。
void MultiCurvePlot::appendCurveSample(
    const QString& curveId,
    const QPointF& sample)
{
    CurveEntry* entry = findCurveEntry(curveId);
    if (entry == nullptr || entry->curvePtr == nullptr)
    {
        return;
    }
    entry->samples.append(sample);
    entry->curvePtr->setSamples(entry->samples);
    if (m_plotPtr != nullptr)
    {
        m_plotPtr->replot();
    }
}

// 从 Qwt 当前数据对象复制指定曲线样本。
QVector<QPointF> MultiCurvePlot::curveSamples(
    const QString& curveId) const
{
    QVector<QPointF> samples;
    const CurveEntry* entry = findCurveEntry(curveId);
    if (entry == nullptr || entry->curvePtr == nullptr)
    {
        return samples;
    }
    const QwtSeriesData<QPointF>* dataPtr = entry->curvePtr->data();
    if (dataPtr == nullptr)
    {
        return samples;
    }
    samples.reserve(static_cast<qsizetype>(dataPtr->size()));
    for (std::size_t index = 0; index < dataPtr->size(); ++index)
    {
        samples.append(dataPtr->sample(index));
    }
    return samples;
}

// 清空指定曲线的数据。
void MultiCurvePlot::clearCurve(const QString& curveId)
{
    CurveEntry* entry = findCurveEntry(curveId);
    if (entry == nullptr || entry->curvePtr == nullptr)
    {
        return;
    }
    entry->samples.clear();
    entry->curvePtr->setSamples(entry->samples);
    if (m_plotPtr != nullptr)
    {
        m_plotPtr->replot();
    }
}

// 清空全部曲线数据并只刷新一次。
void MultiCurvePlot::clear()
{
    for (const std::unique_ptr<CurveEntry>& entry : m_curveEntries)
    {
        if (entry == nullptr || entry->curvePtr == nullptr)
        {
            continue;
        }
        entry->samples.clear();
        entry->curvePtr->setSamples(entry->samples);
    }
    if (m_plotPtr != nullptr)
    {
        m_plotPtr->replot();
    }
}

// 切换指定曲线和对应图例条目的显示状态。
void MultiCurvePlot::setCurveVisible(
    const QString& curveId,
    bool isVisible)
{
    CurveEntry* entry = findCurveEntry(curveId);
    if (entry == nullptr || entry->curvePtr == nullptr)
    {
        return;
    }
    entry->curvePtr->setVisible(isVisible);
    entry->curvePtr->setItemAttribute(QwtPlotItem::Legend, isVisible);
    if (m_plotPtr != nullptr)
    {
        m_plotPtr->replot();
    }
}

// 固定图表横轴像素索引范围和纵轴原始强度范围。
void MultiCurvePlot::setAxisRanges(
    double xMinimum,
    double xMaximum,
    double yMinimum,
    double yMaximum)
{
    if (m_plotPtr == nullptr
        || xMaximum <= xMinimum
        || yMaximum <= yMinimum)
    {
        return;
    }
    m_plotPtr->setAxisScale(QwtPlot::xBottom, xMinimum, xMaximum);
    m_plotPtr->setAxisScale(QwtPlot::yLeft, yMinimum, yMaximum);
    m_plotPtr->replot();
}

// 替换 X/Y 轴标题。
void MultiCurvePlot::setAxisTitles(
    const QString& xAxisTitle,
    const QString& yAxisTitle)
{
    if (m_plotPtr == nullptr)
    {
        return;
    }
    m_plotPtr->setAxisTitle(QwtPlot::xBottom, xAxisTitle);
    m_plotPtr->setAxisTitle(QwtPlot::yLeft, yAxisTitle);
    m_plotPtr->replot();
}

// 恢复由当前全部曲线数据确定的自动坐标范围。
void MultiCurvePlot::resetView()
{
    if (m_plotPtr == nullptr)
    {
        return;
    }
    m_plotPtr->setAxisAutoScale(QwtPlot::xBottom, true);
    m_plotPtr->setAxisAutoScale(QwtPlot::yLeft, true);
    m_plotPtr->replot();
}

// 初始化数值坐标、网格、图例、缩放和平移。
void MultiCurvePlot::initializePlot()
{
    if (m_plotPtr == nullptr)
    {
        return;
    }
    m_plotPtr->setTitle(m_options.title);
    m_plotPtr->setAxisTitle(
        QwtPlot::xBottom,
        m_options.xAxisTitle);
    m_plotPtr->setAxisTitle(
        QwtPlot::yLeft,
        m_options.yAxisTitle);
    m_plotPtr->setAxisAutoScale(QwtPlot::xBottom, true);
    m_plotPtr->setAxisAutoScale(QwtPlot::yLeft, true);

    m_gridPtr = std::make_unique<QwtPlotGrid>();
    m_gridPtr->setMajorPen(QPen(QColor(205, 205, 205), 0.0));
    m_gridPtr->setMinorPen(QPen(
        QColor(225, 225, 225),
        0.0,
        Qt::DotLine));
    m_gridPtr->enableXMin(true);
    m_gridPtr->enableYMin(true);
    m_gridPtr->attach(m_plotPtr);

    if (m_options.isLegendVisible)
    {
        m_legendPtr = new QwtLegend();
        m_plotPtr->insertLegend(
            m_legendPtr,
            QwtPlot::RightLegend);
    }

    m_magnifierPtr = std::make_unique<QwtPlotMagnifier>(
        m_plotPtr->canvas());
    m_magnifierPtr->setWheelFactor(
        1.0 / m_magnifierPtr->wheelFactor());
    m_pannerPtr = std::make_unique<QwtPlotPanner>(
        m_plotPtr->canvas());
}

// 查找可修改的曲线条目。
MultiCurvePlot::CurveEntry* MultiCurvePlot::findCurveEntry(
    const QString& curveId)
{
    for (const std::unique_ptr<CurveEntry>& entry : m_curveEntries)
    {
        if (entry != nullptr && entry->curveId == curveId)
        {
            return entry.get();
        }
    }
    return nullptr;
}

// 查找只读曲线条目。
const MultiCurvePlot::CurveEntry* MultiCurvePlot::findCurveEntry(
    const QString& curveId) const
{
    for (const std::unique_ptr<CurveEntry>& entry : m_curveEntries)
    {
        if (entry != nullptr && entry->curveId == curveId)
        {
            return entry.get();
        }
    }
    return nullptr;
}
