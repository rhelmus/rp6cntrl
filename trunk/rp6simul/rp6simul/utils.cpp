#include "utils.h"

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
void drawLED(QPainter &painter, const char *propname,
             const QPoint &base, qreal scale)
{
    QPointF pos =
            CSimulator::getInstance()->getRobotProperty(propname, "pos").toPoint();

    // Account for scaling
    pos.rx() *= scale;
    pos.ry() *= scale;

    const QColor c =
            CSimulator::getInstance()->getRobotProperty(propname, "color").value<QColor>();

    const int rad =
            CSimulator::getInstance()->getRobotProperty(propname, "radius").toInt();
    const int srad = static_cast<qreal>(rad) * scale;
    const int totalrad = 2 * srad; // light effect is bigger than led

    QRadialGradient rg(base + pos, totalrad);
    rg.setColorAt(0.0, c);
    rg.setColorAt(0.3, QColor(c.red(), c.green(), c.blue(), 255));
    rg.setColorAt(1.0, QColor(c.red(), c.green(), c.blue(), 0));
    painter.setBrush(rg);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(base + pos, totalrad, totalrad);
}
