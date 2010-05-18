#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QQueue>

#include "shared.h"

class QTimer;

class QextSerialPort;

class CSerialPort: public QObject
{
    Q_OBJECT

    QextSerialPort *serialPort;
    QByteArray msgBuffer, textBuffer;
    QQueue<QString> commandQueue;
    QTimer *commandProcessTimer;

private slots:
    void onReadyRead(void);
    void disableRTS(void);
    void processCommandQueue(void);

public:
    CSerialPort(QObject *parent, const QString &port);

    void resetRP6(void);
    void sendCommand(const QString &command);
    void launchRP6(void) { sendCommand("s"); }
    

signals:
    void textAvailable(const QByteArray &text);
    void msgAvailable(ESerialMessage msg, const QByteArray &data);
};

#endif
