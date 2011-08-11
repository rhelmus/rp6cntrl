#include "robotwidget.h"
#include "simulator.h"
#include "utils.h"

#include <QtGui>

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

    // NOTE: drawLED() alters brush
    const QPoint base(x, y);

    // Equal aspect ratio, so scaling equals for width and height
    const qreal scale = (qreal)robotPixmap.width() / (qreal)origRobotSize.width();

    if (enabledLEDs[LED1])
        drawLED(painter, "led1", base, scale);
    if (enabledLEDs[LED2])
        drawLED(painter, "led2", base, scale);
    if (enabledLEDs[LED3])
        drawLED(painter, "led3", base, scale);
    if (enabledLEDs[LED4])
        drawLED(painter, "led4", base, scale);
    if (enabledLEDs[LED5])
        drawLED(painter, "led5", base, scale);
    if (enabledLEDs[LED6])
        drawLED(painter, "led6", base, scale);
    if (enabledLEDs[ACSL])
        drawLED(painter, "ACSLeft", base, scale);
    if (enabledLEDs[ACSR])
        drawLED(painter, "ACSRight", base, scale);
}
