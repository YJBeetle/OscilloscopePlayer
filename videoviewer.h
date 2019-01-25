#ifndef VIDEOVIEWER_H
#define VIDEOVIEWER_H

#include <QWidget>
#include <QDebug>
#include <QPainter>

class VideoViewer : public QWidget
{
    Q_OBJECT
public:
    explicit VideoViewer(QWidget *parent = nullptr);
    ~VideoViewer();

    QImage *image = nullptr;

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event);

private:

};

#endif // VIDEOVIEWER_H
