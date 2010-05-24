#include <QPainter>

#include <scanner.h>

CScannerWidget::CScannerWidget(QWidget *parent,
                               Qt::WindowFlags f) : QWidget(parent, f), newPoint(false)
{
    // Test
    scanPoints << SScanPoint(10, 20, ACS_POWER_LOW);
    scanPoints << SScanPoint(40, 50, ACS_POWER_LOW);
    scanPoints << SScanPoint(80, 140, ACS_POWER_MED);
    scanPoints << SScanPoint(160, 180, ACS_POWER_HIGH);
}

QRect CScannerWidget::getRingRect(int ring) const
{
    const int maxsize = std::min(width(), height()) - 10;
    const int minsize = std::max(80, maxsize - 60);
    const int step = ((maxsize - minsize) / 3);
    const int size = maxsize - (ring * 2 * step);
    const int pos = ring * step;
    
    return QRect((pos + (width() - (size + pos))) / 2, // x
                 (pos + (height() - (size + pos))) / 2, // y
                 size, // width
                 size); // height
}

void CScannerWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.save();

    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(Qt::gray);
    
    for (int i=0; i<3; ++i)
        painter.drawEllipse(getRingRect(i));
    
    painter.setPen(QPen(Qt::blue, 3.0));
    
    for (QList<SScanPoint>::iterator it=scanPoints.begin(); it!=scanPoints.end(); ++it)
    {
        int ring = 3 - it->power;
        
        // Convert to weird Qt angles
        int start = -((it->startAngle * 16) - (90 * 16));
        int stop = -((it->stopAngle * 16) - (90 * 16)) - start;
        painter.drawArc(getRingRect(ring), start, stop);
    }
    
    painter.restore();
}

void CScannerWidget::addPoint(int angle, EACSPowerState power)
{
    if (!newPoint && !scanPoints.isEmpty() && (scanPoints.back().power == power))
        scanPoints.back().stopAngle = angle;
    else
        scanPoints << SScanPoint(angle, angle+1, power); // UNDONE: +1?
    
    newPoint = false;
    
    update();
}

