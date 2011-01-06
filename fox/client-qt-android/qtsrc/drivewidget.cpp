#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QVBoxLayout>

#include "drivewidget.h"

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

    painter.setPen(Qt::red);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(startDragPos, endDragPos);

    painter.restore();
}

void CDriveWidget::mousePressEvent(QMouseEvent *event)
{
    startDragPos = event->pos();
    event->accept();
}

void CDriveWidget::mouseMoveEvent(QMouseEvent *event)
{
    endDragPos = event->pos();
    update();
    event->accept();
}

void CDriveWidget::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    startDragPos = QPoint(0, 0);
    update();
}
