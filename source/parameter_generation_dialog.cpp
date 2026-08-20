#include "parameter_generation_dialog.h"

#include "ui_parameter_generation_dialog.h"

#include <QPushButton>

namespace image_viewer {

// 创建指定工具的非模态置顶参数窗口并连接固定按钮行为。
ParameterGenerationDialog::ParameterGenerationDialog(
    ImgGraphicsView::ToolMode toolMode,
    ImgGraphicsView* imageViewPtr,
    QWidget* parentPtr)
    : QDialog(parentPtr, Qt::Dialog | Qt::WindowStaysOnTopHint)
    , ui(std::make_unique<Ui::ParameterGenerationDialog>())
    , m_imageViewPtr(imageViewPtr)
    , m_toolMode(toolMode)
{
    ui->setupUi(this);
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose);
    initializeToolUiState();
    initializeInputRanges();
    connect(ui->btn_generate,
            &QPushButton::clicked,
            this,
            &ParameterGenerationDialog::onGenerateButtonClicked);
    connect(ui->btn_cancel,
            &QPushButton::clicked,
            this,
            &ParameterGenerationDialog::onCancelButtonClicked);
}

// 使用默认析构释放智能指针管理的UI对象。
ParameterGenerationDialog::~ParameterGenerationDialog() = default;

// 根据工具类型切换Designer中已经定义的参数页和英文标题。
void ParameterGenerationDialog::initializeToolUiState()
{
    if (m_toolMode == ImgGraphicsView::ToolMode::Line)
    {
        setWindowTitle(tr("Generate Line"));
        ui->stack_parameterPages->setCurrentWidget(ui->page_lineParameters);
    }
    else if (m_toolMode == ImgGraphicsView::ToolMode::Circle)
    {
        setWindowTitle(tr("Generate Circle"));
        ui->stack_parameterPages->setCurrentWidget(ui->page_circleParameters);
    }
    else if (m_toolMode == ImgGraphicsView::ToolMode::Rect)
    {
        setWindowTitle(tr("Generate Rectangle"));
        ui->stack_parameterPages->setCurrentWidget(ui->page_rectParameters);
    }
    else if (m_toolMode == ImgGraphicsView::ToolMode::Pick)
    {
        setWindowTitle(tr("Generate Pick"));
        ui->stack_parameterPages->setCurrentWidget(ui->page_pickParameters);
    }
}

// 使用窗口打开时的当前图片尺寸设置全部整数输入范围。
void ParameterGenerationDialog::initializeInputRanges()
{
    if (m_imageViewPtr == nullptr || !m_imageViewPtr->hasImage())
    {
        return;
    }

    const QSize canvasSize = m_imageViewPtr->canvasSize();
    const QSize sourceImageSize = m_imageViewPtr->sourceImageSize();
    ui->sbox_lineStartX->setRange(0, canvasSize.width());
    ui->sbox_lineStartY->setRange(0, canvasSize.height());
    ui->sbox_lineEndX->setRange(0, canvasSize.width());
    ui->sbox_lineEndY->setRange(0, canvasSize.height());
    ui->sbox_circleCenterX->setRange(0, canvasSize.width());
    ui->sbox_circleCenterY->setRange(0, canvasSize.height());
    ui->sbox_circleRadius->setRange(
        1,
        qMax(1, qMin(canvasSize.width(), canvasSize.height())));
    ui->sbox_rectStartX->setRange(0, qMax(0, canvasSize.width() - 1));
    ui->sbox_rectStartY->setRange(0, qMax(0, canvasSize.height() - 1));
    ui->sbox_rectWidth->setRange(1, qMax(1, canvasSize.width()));
    ui->sbox_rectHeight->setRange(1, qMax(1, canvasSize.height()));
    ui->sbox_pickX->setRange(0, qMax(0, sourceImageSize.width() - 1));
    ui->sbox_pickY->setRange(0, qMax(0, sourceImageSize.height() - 1));
}

// 校验当前工具参数，成功提交后关闭非模态窗口。
void ParameterGenerationDialog::onGenerateButtonClicked()
{
    if (m_imageViewPtr == nullptr || !m_imageViewPtr->hasImage())
    {
        showValidationError(
            IMAGE_UNAVAILABLE_ERROR_CODE,
            tr("No image is available for parameter generation."));
        return;
    }

    bool isGenerated = false;
    int errorCode = IMAGE_UNAVAILABLE_ERROR_CODE;
    QString errorMessage = tr("Unsupported parameter generation tool.");
    if (m_toolMode == ImgGraphicsView::ToolMode::Line)
    {
        isGenerated = m_imageViewPtr->generateLineByParameters(
            QPoint(ui->sbox_lineStartX->value(),
                   ui->sbox_lineStartY->value()),
            QPoint(ui->sbox_lineEndX->value(),
                   ui->sbox_lineEndY->value()));
        errorCode = INVALID_LINE_PARAMETER_ERROR_CODE;
        errorMessage = tr(
            "Line endpoints must be different and within the current canvas.");
    }
    else if (m_toolMode == ImgGraphicsView::ToolMode::Circle)
    {
        isGenerated = m_imageViewPtr->generateCircleByParameters(
            QPoint(ui->sbox_circleCenterX->value(),
                   ui->sbox_circleCenterY->value()),
            ui->sbox_circleRadius->value());
        errorCode = INVALID_CIRCLE_PARAMETER_ERROR_CODE;
        errorMessage = tr(
            "The circle must have a positive radius and remain within the current canvas.");
    }
    else if (m_toolMode == ImgGraphicsView::ToolMode::Rect)
    {
        isGenerated = m_imageViewPtr->generateRectByParameters(
            QPoint(ui->sbox_rectStartX->value(),
                   ui->sbox_rectStartY->value()),
            QSize(ui->sbox_rectWidth->value(),
                  ui->sbox_rectHeight->value()));
        errorCode = INVALID_RECT_PARAMETER_ERROR_CODE;
        errorMessage = tr(
            "The rectangle must have a positive size and remain within the current canvas.");
    }
    else if (m_toolMode == ImgGraphicsView::ToolMode::Pick)
    {
        isGenerated = m_imageViewPtr->generatePickByParameters(
            QPoint(ui->sbox_pickX->value(), ui->sbox_pickY->value()));
        errorCode = INVALID_PICK_PARAMETER_ERROR_CODE;
        errorMessage = tr(
            "Pick coordinates must be within the logical image pixel range.");
    }

    if (!isGenerated)
    {
        showValidationError(errorCode, errorMessage);
        return;
    }

    close();
}

// 直接关闭参数窗口且不改变图片视图状态。
void ParameterGenerationDialog::onCancelButtonClicked()
{
    close();
}

// 在窗口内显示包含模块名和错误码的英文校验信息。
void ParameterGenerationDialog::showValidationError(
    int errorCode,
    const QString& message)
{
    ui->label_validationError->setText(
        tr("[ParameterGeneration] %1 (Code: %2)")
            .arg(message)
            .arg(errorCode));
}

} // namespace image_viewer
