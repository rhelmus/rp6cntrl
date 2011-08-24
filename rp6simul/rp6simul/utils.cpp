#include "utils.h"

#include "led.h"
#include "rp6simul.h"
#include "simulator.h"

#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QString>

bool verifySettingsFile(QSettings &file)
{
    file.sync();

    if (file.status() == QSettings::AccessError)
        QMessageBox::critical(CRP6Simulator::getInstance(), "File access error", "Unable to access project file!");
    else if (file.status() == QSettings::FormatError)
        QMessageBox::critical(CRP6Simulator::getInstance(), "File format error", "Invalid file format!");
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

    const int srad = static_cast<qreal>(led->getRadius()) * scale;
    const int totalrad = 2 * srad; // light effect is bigger than led

    QRadialGradient rg(pos, totalrad);
    rg.setColorAt(0.0, c);
    rg.setColorAt(0.3, QColor(c.red(), c.green(), c.blue(), c.alpha()));
    rg.setColorAt(1.0, QColor(c.red(), c.green(), c.blue(), 0));
    painter.setBrush(rg);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(pos, totalrad, totalrad);
}
