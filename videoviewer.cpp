#include "videoviewer.h"

VideoViewer::VideoViewer(QWidget *parent) : QWidget(parent)
{

}

VideoViewer::~VideoViewer()
{

}

void VideoViewer::paintEvent(QPaintEvent *event)
{
    //QWidget::paintEvent(event);

    QPainter painter(this);
    //painter.save();

    QRect rect = this->geometry();
    painter.fillRect(QRect(QPoint(0, 0), QSize(rect.width(), rect.height())), QBrush(QColor(0, 0, 0)));

        painter.drawImage(0, 0, this->image);

    //painter.restore();

    qDebug() << "VideoViewer::paintEvent";
}
