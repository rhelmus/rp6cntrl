#include "utils.h"

#include "led.h"
#include "rp6simul.h"
#include "simulator.h"

#include <QDebug>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QString>

bool verifySettingsFile(QSettings &file, bool silent)
{
    file.sync();

    if (file.status() == QSettings::AccessError)
    {
        if (!silent)
            QMessageBox::critical(CRP6Simulator::getInstance(), "File error",
                                  "Unable to access file!");
    }
    else if (file.status() == QSettings::FormatError)
    {
        if (!silent)
            QMessageBox::critical(CRP6Simulator::getInstance(), "File error",
                                  "Invalid file format!");
    }
    else
        return true;

    return false;
}

// Used by robot widget/item
void drawLED(QPainter &painter, CLED *led, const QTransform &tr,
             qreal scale)
{
    QPointF pos = led->getPosition();

    // Account for scaling
    pos.rx() *= scale;
    pos.ry() *= scale;

    // Apply other transformations
    pos = tr.map(pos);

    const QColor c = led->getColor();

    const qreal srad = static_cast<qreal>(led->getRadius()) * scale;
    const int totalrad = qRound(1.75 * srad); // light effect is bigger than led

    QRadialGradient rg(pos, totalrad);
    rg.setColorAt(0.0, c);
    rg.setColorAt(0.3, QColor(c.red(), c.green(), c.blue(), c.alpha()));
    rg.setColorAt(1.0, QColor(c.red(), c.green(), c.blue(), 0));
    painter.setBrush(rg);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(pos, totalrad, totalrad);
}

qreal toClockwiseAngle(qreal a)
{
    // Convert from counter-clockwise + 90 to clockwise + 0
    return 90.0 - a;
}

qreal toCounterClockwiseAngle(qreal a)
{
    // Convert from clockwise + 0 to counter-clockwise + 90
    // (yes this is the same as the other way around)
    return 90.0 - a;
}
