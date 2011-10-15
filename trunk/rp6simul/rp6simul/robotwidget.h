#ifndef ROBOTWIDGET_H
#define ROBOTWIDGET_H

#include "rp6simul.h"

#include <QMap>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QSignalMapper>
#include <QTime>

class CBumper;
class CDataPlotWidget;
class CIRSensor;
class CLED;
class QTimer;

class CDataPlotSubWindow: public QMdiSubWindow
{
    Q_OBJECT

protected:
    virtual void closeEvent(QCloseEvent *event)
    { emit closed(); QMdiSubWindow::closeEvent(event); }

public:
    CDataPlotSubWindow(QWidget *parent=0, Qt::WindowFlags flags=0)
        : QMdiSubWindow(parent, flags) { }

signals:
    void closed(void);
};

class CRobotWidget : public QMdiArea
{
public:
    enum EDataPlotType { DATAPLOT_MOTORSPEED=0, DATAPLOT_MOTORPOWER,
                         DATAPLOT_LIGHT, DATAPLOT_PIEZO, DATAPLOT_MIC,
                         DATAPLOT_ADC, DATAPLOT_MAX };

private:
    Q_OBJECT

    struct SDataPlotInfo
    {
        CDataPlotSubWindow *subWindow;
        CDataPlotWidget *plotWidget;
        SDataPlotInfo(CDataPlotSubWindow *subw, CDataPlotWidget *w)
            : subWindow(subw), plotWidget(w) { }
        // For QMap
        SDataPlotInfo(void) : subWindow(0), plotWidget(0) { }
    };

    QSize origRobotSize;
    const int motorArrowWidth, motorArrowXSpacing;
    QSize widgetMinSize;
    QPixmap robotPixmap;
    QTimer *dataPlotUpdateTimer;
    QTime runTime;
    QMap<EDataPlotType, SDataPlotInfo> dataPlots;
    QSignalMapper dataPlotClosedSignalMapper;
    QList<CLED *> LEDs;
    QList<CBumper *> bumpers;
    QList<CIRSensor *> IRSensors;
    QMap<EMotor, int> motorPower, motorSpeed;
    QMap<EMotor, EMotorDirection> motorDirection;

    void createDataPlotSubWindow(EDataPlotType plot);

private slots:
    void dataPlotTimedUpdate(void);

protected:
    void paintEvent(QPaintEvent *event);

public:
    explicit CRobotWidget(QWidget *parent = 0);

    QSize minimumSizeHint(void) const { return widgetMinSize; }

    void start(void);
    void stop(void);
    void showDataPlot(EDataPlotType plot);
    void addBumper(CBumper *b);
    void removeBumper(CBumper *b);
    void addLED(CLED *l);
    void removeLED(CLED *l);
    void addIRSensor(CIRSensor *ir);
    void removeIRSensor(CIRSensor *ir);
    void setMotorPower(EMotor m, int p) { motorPower[m] = p; update(); }
    void setMotorSpeed(EMotor m, int s) { motorSpeed[m] = s; update(); }
    void setMotorDirection(EMotor m, EMotorDirection d)
    { motorDirection[m] = d; update(); }

signals:
    void dataPlotClosed(int);
};


#endif // ROBOTWIDGET_H
