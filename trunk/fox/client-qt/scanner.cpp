#include <QDebug>
#include <QPainter>

#include "scanner.h"

namespace {

int convertToArcAngle(int angle)
{
    // Convert to weird Qt angles
    return -((angle * 16) - (90 * 16));
}

void drawArc(QPainter &painter, const QRect &rect, int start, int stop)
{
    int arcstart = convertToArcAngle(start);
    int arcstop = convertToArcAngle(stop) - arcstart;
    painter.drawArc(rect, arcstart, arcstop);
}

}


CScannerWidget::CScannerWidget(int maxdist, int mina, int maxa, QWidget *parent,
                               Qt::WindowFlags f) : QWidget(parent, f), newPoint(false),
                                                    maxDistance(maxdist), minAngle(mina),
                                                    maxAngle(maxa)
{
}

QRect CScannerWidget::getRingRect(int dist) const
{
    const int maxsize = std::min(width(), height()) - 10;
    const int minsize = std::max(40, maxsize - 100);
    const float step = (static_cast<float>(maxsize - minsize) / static_cast<float>(maxDistance));
    const float size = minsize + (dist * step);
    const float pos = dist * step;
    
//     qDebug() << "dist: " << dist << " minsize: " << minsize << " maxsize: " << maxsize << " step: " << step <<
//             " size: " << size << " pos: " << pos;
    
    return QRect((pos + (width() - (size + pos))) / 2, // x
                 (pos + (height() - (size + pos))) / 2, // y
                 size, // width
                 size); // height
}

void CScannerWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();

    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QPen(Qt::blue, 3.0));

    for (QList<SScanPoint>::iterator it=scanPoints.begin(); it!=scanPoints.end(); ++it)
        drawArc(painter, getRingRect(it->distance), it->startAngle, it->stopAngle);
    
    for (int i=0; i<=3; ++i)
    {
        int dist = i * (maxDistance / 3.0);
        QRect rect = getRingRect(dist);
        painter.setPen(Qt::gray);
        drawArc(painter, rect, minAngle, maxAngle);
        
        painter.setPen(Qt::yellow);
        rect.setTop(rect.top() - 5);
        painter.drawText(rect, Qt::AlignTop| Qt::AlignHCenter, QString::number(dist));
    }
    
    painter.restore();
}

void CScannerWidget::addPoint(int angle, int distance)
{
    if (!newPoint && !scanPoints.isEmpty() && (scanPoints.back().distance == distance))
        scanPoints.back().stopAngle = angle;
    else
        scanPoints << SScanPoint(angle, angle+1, distance); // UNDONE: +1?
    
    newPoint = false;
    
    update();
}
