#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>

class QextSerialPort;

class CSerialPort: public QObject
{
    Q_OBJECT

    QextSerialPort *serialPort;
    QByteArray buffer;
    
private slots:
    void onReadyRead(void);
    void disableRTS(void);

public:
    CSerialPort(QObject *parent);

    void resetRP6(void);
    void sendCommand(const QString &command);
    void launchRP6(void) { sendCommand("s"); }
    

signals:
    void textAvailable(const QByteArray &text);
};

#endif
