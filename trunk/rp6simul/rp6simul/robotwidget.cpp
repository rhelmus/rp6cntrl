#include "robotwidget.h"

#include <QtGui>

CRobotWidget::CRobotWidget(QWidget *parent) :
    QWidget(parent), robotImgSize(135, 214), // Scaled factor 1.25
    widgetExtraSize(100, 0),
    robotPixmap(QPixmap("../resource/rp6-top.png").scaled(robotImgSize,
                                                          Qt::IgnoreAspectRatio,
                                                          Qt::SmoothTransformation))
{
}

void CRobotWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    // center
    const int x = (width() - robotPixmap.width()) / 2;
    const int y = (height() - robotPixmap.height()) / 2;
    painter.drawPixmap(x, y, robotPixmap);
}
