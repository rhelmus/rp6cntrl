#ifndef CONTROL_H
#define CONTROL_H

#include <QObject>

class CSerialPort;
class CTcpServer;

class CControl: public QObject
{
    Q_OBJECT

    CSerialPort *serialPort;
    CTcpServer *tcpServer;

private slots:
    void parseSerial(const QString &text);
    
public:
    CControl(QObject *parent);
};

#endif
