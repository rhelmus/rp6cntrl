#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTcpSocket>

class QCheckBox;
class QLabel;
class QLCDNumber;
class QLineEdit;
class QPlainTextEdit;
class QSlider;

class CQtClient: public QMainWindow
{
    Q_OBJECT
    
    quint32 tcpReadBlockSize;
    QTcpSocket *clientSocket;
    QLineEdit *serverEdit;
    QPlainTextEdit *logWidget;
    
    QLCDNumber *motorSpeedLCD[2];
    QLCDNumber *motorDistanceLCD[2];
    QLCDNumber *motorCurrentLCD[2];
    QLCDNumber *lightSensorsLCD[2];
    QLabel *RC5DeviceLabel, *RC5KeyLabel;
    QCheckBox *RC5ToggleBitBox;
    QCheckBox *ACSCollisionBox[2];
    QSlider *ACSPowerSlider;
    
    QWidget *createOverviewWidget(void);
    QWidget *createSpeedWidget(void);
    QWidget *createDistWidget(void);
    QWidget *createCurrentWidget(void);
    QWidget *createLightWidget(void);
    QWidget *createACSWidget(void);
    QWidget *createBatteryWidget(void);
    QWidget *createMicWidget(void);
    
    void appendLogText(const QString &text);
    
private slots:
    void serverHasData(void);
    void socketError(QAbstractSocket::SocketError error);
    void connectToServer(void);
    
public:
    CQtClient(void);
};

#endif
