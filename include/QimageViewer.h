#pragma once

#include <QtWidgets/QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QRectF>

QT_BEGIN_NAMESPACE
namespace Ui { class QimageViewerClass; };
QT_END_NAMESPACE

class QimageViewer : public QMainWindow
{
    Q_OBJECT

public:
    QimageViewer(QWidget *parent = nullptr);
    ~QimageViewer();

private slots:
    void on_Btn_LoadImage_clicked();
    void onROISelected(const QRectF& roiRect);

private:
    Ui::QimageViewerClass *ui;
    QGraphicsScene* m_roiScene;
    QGraphicsPixmapItem* m_roiPixmapItem;
    QPixmap m_originalPixmap;
};

