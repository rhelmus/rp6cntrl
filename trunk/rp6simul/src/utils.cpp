#include "utils.h"

#include "led.h"
#include "rp6simul.h"
#include "simulator.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QString>

namespace {

bool initializedOverridedPath = false;
QString overridedDataPath;

QString getDataDir()
{
    if (!initializedOverridedPath)
    {
        Q_ASSERT(!QCoreApplication::startingUp());

        QStringList args(QCoreApplication::arguments());
        int index = args.lastIndexOf("--datadir");
        if (index == -1)
            index = args.lastIndexOf("-d");
        if ((index != -1) && (args.size() > (index+1)))
            overridedDataPath = args.at(index + 1);
        initializedOverridedPath = true;
    }

    if (!overridedDataPath.isEmpty())
        return overridedDataPath;

    const QString apppath(QCoreApplication::applicationDirPath());
    return QString(apppath + "/../share/%1").arg(QCoreApplication::applicationName());
}

}


QString getResourcePath(const QString &file)
{/*
    QDir datapath(getDataDir());
    return QDir::toNativeSeparators(QDir::cleanPath(datapath.absoluteFilePath("resource/"+file)));
    */
    QDir datapath(getDataDir());
    QString ret = QDir::toNativeSeparators(QDir::cleanPath(datapath.absoluteFilePath("resource/"+file)));
    if (!QFile(ret).exists())
        qWarning() << ret << "does not exist!";
    return ret;
}

QString getLuaSrcPath(const QString &file)
{
    QDir datapath(getDataDir());
    return QDir::toNativeSeparators(QDir::cleanPath(datapath.absoluteFilePath("src/lua/"+file)));
}

QString getMapTemplatesPath(const QString &file)
{
    QDir datapath(getDataDir());
    return QDir::toNativeSeparators(QDir::cleanPath(datapath.absoluteFilePath("map_templates/"+file)));
}


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
