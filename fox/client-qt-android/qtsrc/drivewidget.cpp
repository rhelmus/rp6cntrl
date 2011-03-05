#include <math.h>

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QVBoxLayout>

#include "drivewidget.h"

namespace {

float angleBetween(const QVector2D &v1, const QVector2D &v2)
{
    QVector2D dv2 = v2 - v1;
    float rad = atan2f(dv2.y(), dv2.x()) - atan2f(v1.y(), v1.x());
//    rad += M_PI;
//    if (rad > (2*M_PI))
//        rad -= (2*M_PI);
    return rad * (180.0 / M_PI);
}

}

CDriveWidget::CDriveWidget(QWidget *parent) : QFrame(parent)
{
    setFrameStyle(StyledPanel | Raised);
}

void CDriveWidget::paintEvent(QPaintEvent *event)
{
    if (startDragPos.isNull())
        return;

    QPainter painter(this);
    painter.save();

    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QPen(Qt::green, 3));
    painter.setBrush(Qt::green);
    QRect rect(endDragPos.x() - 10, endDragPos.y() - 10, 20, 20);
    painter.drawEllipse(rect);

    float angle = angleBetween(startDragPos, endDragPos);
    const float res = 10.0;
    float closest = floorf(angle / res + 0.5) * res;
    QLineF line(startDragPos.toPointF(), QPointF(startDragPos.x(), endDragPos.y()));
    line.setAngle(-closest - 0);
    painter.setPen(Qt::red);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(line);
//    painter.drawLine(startDragPos.toPoint(), endDragPos.toPoint());

    painter.restore();
}

void CDriveWidget::mousePressEvent(QMouseEvent *event)
{
    startDragPos = QVector2D(event->pos());
    event->accept();
}

void CDriveWidget::mouseMoveEvent(QMouseEvent *event)
{
    endDragPos = QVector2D(event->pos());
    update();
    event->accept();
}

void CDriveWidget::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    startDragPos = QVector2D(0, 0);
    update();
}
