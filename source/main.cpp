#include "image_viewer_window.h"
#include <QtWidgets/QApplication>

// 创建Qt应用并显示图片查看器主窗口。
int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    image_viewer::ImageViewerWindow window;
    window.show();
    return application.exec();
}
