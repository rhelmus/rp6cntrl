#include "robotwidget.h"
#include "simulator.h"
#include "utils.h"

#include <QtGui>

namespace {

void drawBumper(QPainter &painter, const char *propname,
             const QTransform &tr, qreal scale)
{
    QPolygonF points =
            CSimulator::getInstance()->getRobotProperty(propname, "points").value<QPolygon>();

    // Account for scaling/transformation
    for (QPolygonF::iterator it=points.begin(); it!=points.end(); ++it)
    {
        it->rx() *= scale;
        it->ry() *= scale;
        *it = tr.map(*it);
    }

    QColor c =
            CSimulator::getInstance()->getRobotProperty(propname, "color").value<QColor>();

    c.setAlpha(127);

    painter.setBrush(c);
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(points);
}

}

CRobotWidget::CRobotWidget(QWidget *parent) :
    QWidget(parent), widgetExtraSize(100, 0)
{
    const QPixmap p("../resource/rp6-top.png");
    origRobotSize = p.size();
    robotPixmap = p.scaledToWidth(250, Qt::SmoothTransformation);
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

    QTransform tr;
    tr.translate(x, y);

    // Equal aspect ratio, so scaling equals for width and height
    const qreal scale = (qreal)robotPixmap.width() / (qreal)origRobotSize.width();

    // NOTE: drawLED() alters brush
    if (enabledLEDs[LED1])
        drawLED(painter, "led1", tr, scale);
    if (enabledLEDs[LED2])
        drawLED(painter, "led2", tr, scale);
    if (enabledLEDs[LED3])
        drawLED(painter, "led3", tr, scale);
    if (enabledLEDs[LED4])
        drawLED(painter, "led4", tr, scale);
    if (enabledLEDs[LED5])
        drawLED(painter, "led5", tr, scale);
    if (enabledLEDs[LED6])
        drawLED(painter, "led6", tr, scale);
    if (enabledLEDs[ACSL])
        drawLED(painter, "ACSLeft", tr, scale);
    if (enabledLEDs[ACSR])
        drawLED(painter, "ACSRight", tr, scale);

    if (hitBumpers[BUMPER_LEFT])
        drawBumper(painter, "bumperLeft", tr, scale);
    if (hitBumpers[BUMPER_RIGHT])
        drawBumper(painter, "bumperRight", tr, scale);
}
