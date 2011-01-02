#ifndef ANDROID_CLIENT_H
#define ANDROID_CLIENT_H

#include <QMainWindow>

#include "client_base.h"

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

    QLineEdit *serverEdit;
    QPushButton *connectButton;

    QMap<EDisplayType, CStatWidget *> statWidgets;

    QPlainTextEdit *consoleOut;
    QPlainTextEdit *logWidget;

    QWidget *createConnectTab(void);
    QWidget *createStatTab(void);
    QWidget *createConsoleTab(void);
    QWidget *createLogTab(void);

    virtual void updateConnection(bool connected);
    virtual void tcpError(const QString &error);
    virtual void tcpRobotStateUpdate(const SStateSensors &,
                                     const SStateSensors &newstate);
    virtual void tcpHandleRobotData(ETcpMessage msg, int data);
    virtual void updateDriveSpeed(int left, int right) { }

private slots:
    void toggleServerConnection(void);

protected:
    virtual void appendConsoleOutput(const QString &text);
    virtual void appendLogOutput(const QString &text);

public:
    CQtClient(void);
};

#endif
