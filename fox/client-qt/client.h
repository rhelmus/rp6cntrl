#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>

#include "shared.h"

class QCheckBox;
class QComboBox;
class QDataStream;
class QLabel;
class QLCDNumber;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSignalMapper;
class QSlider;
class QSpinBox;

class QwtSlider;

class CScannerWidget;
class CSensorPlot;

class CQtClient: public QMainWindow
{
    Q_OBJECT
    
    quint32 tcpReadBlockSize;
    QTcpSocket *clientSocket;
    QLineEdit *serverEdit;
    QPlainTextEdit *consoleOut;
    QLineEdit *consoleIn;
    QPlainTextEdit *logWidget;
    
    struct SSensorData
    {
        uint32_t total, count;
        SSensorData(void) {}
        SSensorData(uint32_t t, uint32_t c) : total(t), count(c) {}
    };
    
    QMap<QString, SSensorData> sensorDataMap;
    
    // Overview widget
    QLCDNumber *motorSpeedLCD[2];
    QLCDNumber *motorDistanceLCD[2];
    QLCDNumber *motorCurrentLCD[2];
    QLabel *motorDirection[2];
    QLCDNumber *lightSensorsLCD[2];
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
    
    QSpinBox *micUpdateTimeSpinBox;
    QPushButton *micUpdateToggleButton;
    
    QSignalMapper *driveMapper;
    int currentDriveDirection;
    bool driveTurning;
    int driveForwardSpeed, driveTurnSpeed;
    QwtSlider *driveSpeedSlider[2];
    
    QSpinBox *scanSpeedSpinBox;
    QComboBox *scanPowerComboBox;
    QPushButton *scanButton;
    CScannerWidget *scannerWidget;
    bool isScanning;
    bool alternatingScan;
    int remainingACSCycles; // Remaining cycles before switch
    
    bool firstStateUpdate;
    SStateSensors currentStateSensors;
    int motorDistance[2];
    int destMotorDistance[2];
    SMotorDirections currentMotorDirections;
    EACSPowerState ACSPowerState;
    
    QWidget *createMainTab(void);
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
    
    QWidget *createConnectionWidget(void);
    QWidget *createDriveWidget(void);
    QWidget *createScannerWidget(void);
    
    void appendConsoleText(const QString &text);
    void appendLogText(const QString &text);
    void parseTcp(QDataStream &stream);
    void updateStateSensors(const SStateSensors &state);
    void updateScan(const SStateSensors &oldstate, const SStateSensors &newstate);
    void updateMotorDirections(const SMotorDirections &dir);
    void executeCommand(const QString &cmd);
    void changeDriveSpeedVar(int &speed, int delta);
    
private slots:
    void connectedToServer(void);
    void serverHasData(void);
    void socketError(QAbstractSocket::SocketError error);
    void updateSensors(void);
    void connectToServer(void);
    void setMicUpdateTime(int value);
    void micPlotToggled(bool checked);
    void driveButtonPressed(int dir);
    void stopDriveButtonPressed(void);
    void startScan(void);
    void stopScan(void);
    void sendConsoleCommand(void);
    
public:
    CQtClient(void);
};

#endif
