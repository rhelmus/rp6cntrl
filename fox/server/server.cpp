#include <QtCore>

#include "serial.h"
#include "server.h"
#include "tcp.h"

CControl::CControl(QObject *parent) : QObject(parent)
{
    serialPort = new CSerialPort(this);
    connect(serialPort, SIGNAL(textAvailable(const QString &)), this,
            SLOT(parseSerial(const QString &)));

    tcpServer = new CTcpServer(this);
}

void CControl::parseSerial(const QString &text)
{
    tcpServer->sendText("rawserial", text);
}
