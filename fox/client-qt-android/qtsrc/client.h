#ifndef ANDROID_CLIENT_H
#define ANDROID_CLIENT_H

#include <QMainWindow>

#include "client_base.h"
#include "flickcharm/flickcharm.h"

#include <QRotationSensor>
QTM_USE_NAMESPACE

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

class CStatWidget;

class CQtClient: public QMainWindow, public CBaseClient
{
    Q_OBJECT

    enum EDisplayType
    {
        DISPLAY_SPEED=0,
        DISPLAY_DISTANCE,
        DISPLAY_CURRENT,
        DISPLAY_DIRECTION,

        DISPLAY_LIGHT,
        DISPLAY_ACS,
        DISPLAY_BUMPERS,
        DISPLAY_SHARPIR,

        DISPLAY_BATTERY,
        DISPLAY_RC5,
        DISPLAY_MAIN_LEDS,
        DISPLAY_M32_LEDS,
        DISPLAY_KEYS
    };

    FlickCharm flickCharm; // For 'kinetic scrolling'

    QLineEdit *serverEdit;
    QPushButton *connectButton;

    QMap<EDisplayType, CStatWidget *> statWidgets;

    QPlainTextEdit *consoleOut;
    QLineEdit *consoleIn;
    QPlainTextEdit *logWidget;

    QRotationSensor *accelerometer;
    QLabel *accelLabel;

    QWidget *createConnectTab(void);
    QWidget *createStatTab(void);
    QWidget *createConsoleTab(void);
    QWidget *createLogTab(void);
    QWidget *createDriveTab(void);

    virtual void updateConnection(bool connected);
    virtual void tcpError(const QString &error);
    virtual void tcpRobotStateUpdate(const SStateSensors &,
                                     const SStateSensors &newstate);
    virtual void tcpHandleRobotData(ETcpMessage msg, int data);
    virtual void updateDriveSpeed(int left, int right) { }

private slots:
    void toggleServerConnection(void);
    void sendConsolePressed(void);
    void accelSensorChanged(void);

protected:
    virtual void appendConsoleOutput(const QString &text);
    virtual void appendLogOutput(const QString &text);

public:
    CQtClient(void);
    ~CQtClient(void);
};

#endif
