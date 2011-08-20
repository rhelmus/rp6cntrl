#ifndef ROBOTWIDGET_H
#define ROBOTWIDGET_H

#include "rp6simul.h"

#include <QMap>
#include <QWidget>

class CRobotWidget : public QWidget
{
    Q_OBJECT   

    QSize origRobotSize;
    const int motorArrowWidth, motorArrowXSpacing;
    QSize widgetMinSize;
    QPixmap robotPixmap;
    QMap<ELEDType, bool> enabledLEDs;
    QMap<EBumper, bool> hitBumpers;
    QMap<EMotor, int> motorPower;
    QMap<EMotor, EMotorDirection> motorDirection;

protected:
    void paintEvent(QPaintEvent *);

public:
    explicit CRobotWidget(QWidget *parent = 0);

    QSize minimumSizeHint(void) const { return widgetMinSize; }

    void enableLED(ELEDType l, bool e) { enabledLEDs[l] = e; update(); }
    void setBumperHit(EBumper b, bool e) { hitBumpers[b] = e; update(); }
    void setMotorPower(EMotor m, int p) { motorPower[m] = p; update(); }
    void setMotorDirection(EMotor m, EMotorDirection d)
    { motorDirection[m] = d; update(); }
};

#endif // ROBOTWIDGET_H
