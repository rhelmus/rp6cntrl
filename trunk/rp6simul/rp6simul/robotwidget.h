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
                         DATAPLOT_ROBOTADC, DATAPLOT_M32ADC, DATAPLOT_MAX };

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
    QPixmap m32Pixmap;
    QSize origM32Size;
    float m32Scale;
    QPointF m32Positions[SLOT_END];
    float m32Rotations[SLOT_END];
    EM32Slot activeM32Slot;
    bool m32PixmapDirty;
    QTimer *dataPlotUpdateTimer;
    QTime runTime;
    QMap<EDataPlotType, SDataPlotInfo> dataPlots;
    QSignalMapper dataPlotClosedSignalMapper;
    QMap<QString, int> robotADCValues, m32ADCValues;
    QList<CLED *> robotLEDs;
    QList<CBumper *> bumpers;
    QList<CIRSensor *> IRSensors;
    QMap<EMotor, int> motorPower, motorSpeed;
    QMap<EMotor, EMotorDirection> motorDirection;
    QList<CLED *> m32LEDs;
    int beeperPitch;

    void updateM32Pixmap(void);
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
    void setRobotADCValue(const QString &port, int val)
    { robotADCValues[port] = val; }
    void setM32ADCValue(const QString &port, int val)
    { m32ADCValues[port] = val; }
    void addBumper(CBumper *b);
    void removeBumper(CBumper *b);
    void addRobotLED(CLED *l);
    void removeRobotLED(CLED *l);
    void addIRSensor(CIRSensor *ir);
    void removeIRSensor(CIRSensor *ir);
    void setM32Slot(EM32Slot s, const QPointF &p, float r);
    void setM32Scale(float s) { m32Scale = s; m32PixmapDirty = true; }
    void addM32LED(CLED *l);
    void removeM32LED(CLED *l);
    void setActiveM32Slot(EM32Slot s) { activeM32Slot = s; m32PixmapDirty = true; }
    void setMotorPower(EMotor m, int p) { motorPower[m] = p; update(); }
    void setMotorSpeed(EMotor m, int s) { motorSpeed[m] = s; update(); }
    void setMotorDirection(EMotor m, EMotorDirection d)
    { motorDirection[m] = d; update(); }

public slots:
    void setBeeperPitch(int pitch) { beeperPitch = pitch; }

signals:
    void dataPlotClosed(int);
};


#endif // ROBOTWIDGET_H
