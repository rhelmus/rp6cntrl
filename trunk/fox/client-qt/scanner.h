#ifndef SCANNER_H
#define SCANNER_H

#include <QList>
#include <QWidget>

#include "shared.h"

class CScannerWidget: public QWidget
{
    struct SScanPoint
    {
        int startAngle, stopAngle;
        EACSPowerState power;
        SScanPoint(void) : startAngle(0), stopAngle(0), power(ACS_POWER_OFF) {}
        SScanPoint(int s, int sp, EACSPowerState p) : startAngle(s), stopAngle(sp), power(p) {}
    };
    
    QList<SScanPoint> scanPoints;
    bool newPoint;
    
    QRect getRingRect(int ring) const;
    
protected:
    void paintEvent(QPaintEvent *event);

public:
    CScannerWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
    
    void addPoint(int angle, EACSPowerState power);
    void endPoint(void) { newPoint = true; }
    void clear(void) { scanPoints.clear(); update(); }
    
    QSize minimumSizeHint() const { return QSize(100, 100); }
    QSize sizeHint() const { return QSize(175, 175); }
};

#endif
