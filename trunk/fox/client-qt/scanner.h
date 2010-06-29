#ifndef SCANNER_H
#define SCANNER_H

#include <QList>
#include <QWidget>

#include "shared.h"

class CScannerWidget: public QWidget
{
    struct SScanPoint
    {
        int startAngle, stopAngle, distance;
        SScanPoint(void) : startAngle(0), stopAngle(0), distance(0) {}
        SScanPoint(int s, int sp, int d) : startAngle(s), stopAngle(sp), distance(d) {}
    };
    
    QList<SScanPoint> scanPoints;
    bool newPoint;
    int maxDistance, minAngle, maxAngle;
    
    QRect getRingRect(int dist) const;
    
protected:
    void paintEvent(QPaintEvent *);

public:
    CScannerWidget(int maxdist, int mina=0, int maxa=360, QWidget *parent = 0, Qt::WindowFlags f = 0);
    
    void addPoint(int angle, int distance);
    void endPoint(void) { newPoint = true; }
    void clear(void) { scanPoints.clear(); update(); }
    
    QSize minimumSizeHint() const { return QSize(100, 100); }
    QSize sizeHint() const { return QSize(175, 175); }
};

#endif
