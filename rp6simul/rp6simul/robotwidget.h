#ifndef ROBOTWIDGET_H
#define ROBOTWIDGET_H

#include "rp6simul.h"

#include <QMap>
#include <QWidget>

class CBumper;
class CLED;

class CRobotWidget : public QWidget
{
    Q_OBJECT   

    QSize origRobotSize;
    const int motorArrowWidth, motorArrowXSpacing;
    QSize widgetMinSize;
    QPixmap robotPixmap;
    QList<CLED *> LEDs;
    QList<CBumper *> bumpers;
    QMap<EMotor, int> motorPower, motorSpeed;
    QMap<EMotor, EMotorDirection> motorDirection;

protected:
    void paintEvent(QPaintEvent *);

public:
    explicit CRobotWidget(QWidget *parent = 0);

    QSize minimumSizeHint(void) const { return widgetMinSize; }

    void addBumper(CBumper *b);
    void removeBumper(CBumper *b);
    void addLED(CLED *l);
    void removeLED(CLED *l);
    void setMotorPower(EMotor m, int p) { motorPower[m] = p; update(); }
    void setMotorSpeed(EMotor m, int s) { motorSpeed[m] = s; update(); }
    void setMotorDirection(EMotor m, EMotorDirection d)
    { motorDirection[m] = d; update(); }
};

#endif // ROBOTWIDGET_H
