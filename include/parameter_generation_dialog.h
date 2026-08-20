#pragma once

#include "img_graphics_view.h"

#include <QDialog>

#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class ParameterGenerationDialog; }
QT_END_NAMESPACE

namespace image_viewer {

class ParameterGenerationDialog : public QDialog
{
    Q_OBJECT

public:
    // 输入：需要生成的工具类型、图片视图和父窗口。
    // 输出：无。
    // 作用：创建对应参数页的非模态置顶窗口。
    explicit ParameterGenerationDialog(
        ImgGraphicsView::ToolMode toolMode,
        ImgGraphicsView* imageViewPtr,
        QWidget* parentPtr = nullptr);

    // 释放由智能指针管理的UI对象。
    ~ParameterGenerationDialog() override;

private slots:
    // 校验当前参数，成功生成后关闭窗口。
    void onGenerateButtonClicked();

    // 不生成任何内容并关闭窗口。
    void onCancelButtonClicked();

private:
    // 参数生成窗口使用的错误码。
    static constexpr int IMAGE_UNAVAILABLE_ERROR_CODE = 2001;
    static constexpr int INVALID_LINE_PARAMETER_ERROR_CODE = 2002;
    static constexpr int INVALID_CIRCLE_PARAMETER_ERROR_CODE = 2003;
    static constexpr int INVALID_RECT_PARAMETER_ERROR_CODE = 2004;
    static constexpr int INVALID_PICK_PARAMETER_ERROR_CODE = 2005;

    // 输入：无。
    // 输出：无。
    // 作用：根据工具类型切换参数页并设置英文窗口标题。
    void initializeToolUiState();

    // 输入：无。
    // 输出：无。
    // 作用：根据窗口打开时的画布和逻辑图尺寸限制整数输入范围。
    void initializeInputRanges();

    // 输入：错误码和错误说明。
    // 输出：无。
    // 作用：在窗口内显示可追溯的英文参数错误。
    void showValidationError(int errorCode, const QString& message);

    std::unique_ptr<Ui::ParameterGenerationDialog> ui; // 独占UI对象。
    ImgGraphicsView* m_imageViewPtr; // 由父窗口持有的图片视图观察指针。
    ImgGraphicsView::ToolMode m_toolMode; // 当前窗口负责生成的工具类型。
};

} // namespace image_viewer
