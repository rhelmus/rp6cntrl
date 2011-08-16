#ifndef ROBOTWIDGET_H
#define ROBOTWIDGET_H

#include "rp6simul.h"

#include <QMap>
#include <QWidget>

class CRobotWidget : public QWidget
{
    Q_OBJECT   

    QSize origRobotSize;
    const QSize widgetExtraSize;
    QPixmap robotPixmap;
    QMap<ELEDType, bool> enabledLEDs;
    QMap<EBumper, bool> hitBumpers;

protected:
    void paintEvent(QPaintEvent *);

public:
    explicit CRobotWidget(QWidget *parent = 0);

    QSize minimumSizeHint(void) const
    { return robotPixmap.size() + widgetExtraSize; }

    void enableLED(ELEDType l, bool e) { enabledLEDs[l] = e; update(); }
    void setBumperHit(EBumper b, bool e) { hitBumpers[b] = e; update(); }
};

#endif // ROBOTWIDGET_H
