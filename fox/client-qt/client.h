#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>

#include "client_base.h"
#include "pathengine.h"
#include "shared.h"

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QDataStream;
class QDial;
class QFrame;
class QGroupBox;
class QLabel;
class QLCDNumber;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;
class QSignalMapper;
class QSlider;
class QSpinBox;
class QTimer;

class QwtSlider;

class CEditor;
class CNavMap;
class CScannerWidget;
class CSensorPlot;

class CQtClient: public QMainWindow, public CBaseClient
{
    Q_OBJECT
    
    QLineEdit *serverEdit;
    QPushButton *connectButton;
    QList<QWidget *> connectionDependentWidgets;
    QPlainTextEdit *consoleOut;
    QLineEdit *consoleIn;
    QPlainTextEdit *logWidget;
    
    struct SSensorData
    {
        uint32_t total, count;
        SSensorData(void) : total(0), count(0) {}
    };
    
    QMap<ETcpMessage, SSensorData> averagedSensorDataMap;
    QMap<ETcpMessage, int> delayedSensorDataMap;
    
    // Overview widget
    QLCDNumber *motorSpeedLCD[2];
    QLCDNumber *motorDistanceLCD[2];
    QLCDNumber *motorCurrentLCD[2];
    QLabel *motorDirection[2];
    QLCDNumber *lightSensorsLCD[2];
    QLCDNumber *sharpIRSensor;
    QLabel *RC5DeviceLabel, *RC5KeyLabel;
    QCheckBox *RC5ToggleBitBox;
    QCheckBox *ACSCollisionBox[2];
    QSlider *ACSPowerSlider;
    QCheckBox *mainLEDsBox[6];
    QCheckBox *m32LEDsBox[4];
    QLCDNumber *batteryLCD;
    QCheckBox *bumperBox[2];
    QLabel *m32KeyLabel;
    
    // Plots
    CSensorPlot *motorSpeedPlot;
    CSensorPlot *motorDistancePlot;
    CSensorPlot *motorCurrentPlot;
    CSensorPlot *lightSensorsPlot;
    CSensorPlot *ACSPlot;
    CSensorPlot *batteryPlot;
    CSensorPlot *micPlot;
    CSensorPlot *sharpIRPlot;
    
    QSpinBox *micUpdateTimeSpinBox;
    QPushButton *micUpdateToggleButton;
    
    QSignalMapper *driveMapper;
    QwtSlider *driveSpeedSlider[2];
    
    // ACS scanner
    QSpinBox *ACSScanSpeedSpinBox;
    QComboBox *ACSScanPowerComboBox;
    QPushButton *ACSScanButton;
    CScannerWidget *ACSScannerWidget;
    bool isACSScanning;
    bool alternatingACSScan;
    int remainingACSScanCycles; // Remaining cycles before switch
    
    // IR turret
    QDial *servoDial;
    QPushButton *servoButton;
    bool isTurretScanning;
    QSpinBox *turretScanRangeSpinBox[2], *turretScanResolutionSpinBox;
    QSpinBox *turretScanTimeSpinBox, *turretScanDelaySpinBox;
    QPushButton *turretScanButton;
    CScannerWidget *turrentScannerWidget;
    QTimer *turretScanTimer;
    int turretScanEndRange, turretScanResolution, turretScanTime, turretScanDelay;
    int currentScanPosition;
    QList<int> turretScanData;
    
    CEditor *scriptEditor;
    QPushButton *downloadButton;
    QListWidget *localScriptListWidget, *serverScriptListWidget;
    QListWidgetItem *previousScriptItem;
    QString downloadScript;

    // Navigation
    QGroupBox *robotNavControlGroup;
    QPushButton *robotNavSetStartButton, *robotNavSetGoalButton;
    CNavMap *robotNavMap;
    CPathEngine simNavPathEngine;
    bool simNavUpdatePathGrid;
    QList<QPoint> simNavPathList;
    CNavMap *simNavMap;
    QGroupBox *simNavMapGroup;
    QSpinBox *simMoveTimeSpinBox, *simNavWidthSpinBox, *simNavHeightSpinBox;
    QFrame *simNavEditFrame;
    QButtonGroup *simNavEditButtonGroup;
    QPushButton *startSimNavButton, *clearSimNavButton;
    QTimer *simNavTimer;
    
    bool firstStateUpdate;
    int motorDistance[2];
    int destMotorDistance[2];
    EACSPowerState ACSPowerState;
    
    QWidget *createMainTab(void);
    QWidget *createLuaTab(void);
    QWidget *createNavTab(void);
    QWidget *createConsoleTab(void);
    QWidget *createLogTab(void);
    
    QWidget *createOverviewWidget(void);
    QWidget *createSpeedWidget(void);
    QWidget *createDistWidget(void);
    QWidget *createCurrentWidget(void);
    QWidget *createLightWidget(void);
    QWidget *createACSWidget(void);
    QWidget *createBatteryWidget(void);
    QWidget *createMicWidget(void);
    QWidget *createSharpIRWidget(void);
    
    QWidget *createConnectionWidget(void);
    QWidget *createDriveWidget(void);
    QWidget *createScannerWidget(void);
    QWidget *createIRTurretWidget(void);
    
    QWidget *createLocalLuaWidget(void);
    QWidget *createServerLuaWidget(void);
    
    QWidget *createRobotNavWidget(void);
    QWidget *createSimNavWidget(void);
    
    void updateACSScan(const SStateSensors &oldstate, const SStateSensors &newstate);
    void stopACSScan(void);
    void setServo(int pos);
    bool checkScriptSave(void);
    void stopSimNav(void);
    
    virtual void updateConnection(bool connected);
    virtual void tcpError(const QString &error);
    virtual void tcpRobotStateUpdate(const SStateSensors &oldstate,
                                     const SStateSensors &newstate);
    virtual void tcpHandleRobotData(ETcpMessage msg, int data);
    virtual void tcpLuaScripts(const QStringList &list);
    virtual void tcpRequestedScript(const QByteArray &text);
    virtual void tcpHandleLuaMsg(const QString &msg, const QStringList &args);
    virtual void updateDriveSpeed(int left, int right);
    
private slots:
    void updateSensors(void);
    void toggleServerConnection(void);
    void setMicUpdateTime(int value);
    void micPlotToggled(bool checked);
    void driveButtonPressed(int dir) { updateDriving(dir); }
    void stopDriveButtonPressed(void) { stopDrive(); }
    void ACSScanButtonPressed(void);
    void servoButtonPressed(void);
    void turretScanButtonPressed(void);
    void turretScanTimeout(void);
    void localScriptChanged(QListWidgetItem *item);
    void newLocalScriptPressed(void);
    void addLocalScriptPressed(void);
    void removeLocalScriptPressed(void);
    void uploadLocalScriptPressed(void);
    void runLocalScriptPressed(void);
    void uploadRunLocalScriptPressed(void);
    void runServerScriptPressed(void);
    void removeServerScriptPressed(void);
    void downloadServerScriptPressed(void);
    void robotNavStartPressed(void);
    void robotNavSetStartToggled(bool e);
    void robotNavSetGoalToggled(bool e);
    void robotNavSetGridPressed(void);
    void simNavWidthSpinBoxChanged(int w);
    void simNavHeightSpinBoxChanged(int h);
    void simNavEditButtonToggled(bool e);
    void startSimNav(void);
    void simNavTimeout(void);
    void sendConsolePressed(void);
    
protected:
    virtual void appendConsoleOutput(const QString &text);
    virtual void appendLogOutput(const QString &text);

    virtual void closeEvent(QCloseEvent *e);
    
public:
    CQtClient(void);
};

#endif
