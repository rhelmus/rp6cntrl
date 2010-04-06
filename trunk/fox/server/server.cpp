#include <assert.h>

#include <QtCore>

#include "serial.h"
#include "server.h"
#include "shared.h"
#include "tcp.h"

CControl::CControl(QObject *parent) : QObject(parent)
{
    serialPort = new CSerialPort(this);
    connect(serialPort, SIGNAL(textAvailable(const QByteArray &)), this,
            SLOT(handleSerialText(const QByteArray &)));
    connect(serialPort, SIGNAL(msgAvailable(const QByteArray &)), this,
            SLOT(handleSerialMSG(const QByteArray &)));

    tcpServer = new CTcpServer(this);
    connect(tcpServer, SIGNAL(clientTcpReceived(QDataStream &)), this,
            SLOT(parseClientTcp(QDataStream &)));

    initSerial2TcpMap();
}

void CControl::initSerial2TcpMap()
{
    serial2TcpMap.insert(SERIAL_STATE_SENSORS, SSerial2TcpInfo(DATA_BYTE, "state"));
    
    serial2TcpMap.insert(SERIAL_LIGHT_LEFT, SSerial2TcpInfo(DATA_ACD, "lightleft"));
    serial2TcpMap.insert(SERIAL_LIGHT_RIGHT, SSerial2TcpInfo(DATA_ACD, "lightright"));
    
    serial2TcpMap.insert(SERIAL_MOTOR_SPEED_LEFT, SSerial2TcpInfo(DATA_BYTE, "speedleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_SPEED_RIGHT, SSerial2TcpInfo(DATA_BYTE, "speedright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTSPEED_LEFT, SSerial2TcpInfo(DATA_BYTE, "destspeedleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTSPEED_RIGHT, SSerial2TcpInfo(DATA_BYTE, "destspeedright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DIST_LEFT, SSerial2TcpInfo(DATA_WORD, "distleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DIST_RIGHT, SSerial2TcpInfo(DATA_WORD, "distright"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTDIST_LEFT, SSerial2TcpInfo(DATA_WORD, "destdistleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_DESTDIST_RIGHT, SSerial2TcpInfo(DATA_WORD, "destdistright"));
    serial2TcpMap.insert(SERIAL_MOTOR_CURRENT_LEFT, SSerial2TcpInfo(DATA_ACD, "motorcurrentleft"));
    serial2TcpMap.insert(SERIAL_MOTOR_CURRENT_RIGHT, SSerial2TcpInfo(DATA_ACD, "motorcurrentright"));

    serial2TcpMap.insert(SERIAL_BATTERY, SSerial2TcpInfo(DATA_ACD, "battery"));

    serial2TcpMap.insert(SERIAL_ACS_POWER, SSerial2TcpInfo(DATA_BYTE, "acspower"));
}

void CControl::sendSerial2Tcp(ESerialMessage msg, const QByteArray &serialdata)
{
    switch (serial2TcpMap[msg].dataType)
    {
        case DATA_BYTE:
            tcpServer->sendKeyValue(serial2TcpMap[msg].tcpVariable, serialdata[0]);
            break;
        case DATA_WORD:
        {
            uint16_t data = (uint8_t)serialdata[0] + ((uint8_t)serialdata[1] << 8);
            tcpServer->sendKeyValue(serial2TcpMap[msg].tcpVariable, data);
            break;
        }
        case DATA_ACD: // As word, with filtering
        {
            uint16_t data = (uint8_t)serialdata[0] + ((uint8_t)serialdata[1] << 8);
            if (data < 1024)
                tcpServer->sendKeyValue(serial2TcpMap[msg].tcpVariable, data);
            break;
        }
    }
}

void CControl::handleSerialText(const QByteArray &text)
{
//     qDebug() << "parseSerial text: " << text << "\n";
    
//     qDebug() << "parseSerial msg: " << (uint8_t)text[0] << ":" <<
//         (uint8_t)text[1] << ":" << (uint8_t)text[2] << ":" << (uint8_t)text[3];

    if (text == "[READY]")
    {
        serialPort->sendCommand("s");
        QTimer::singleShot(5000, this, SLOT(enableRP6Slave()));
    }
    else
    {
        tcpServer->sendText("rawserial", text);
//         qDebug() << "Raw: " << text;
    }
}

void CControl::handleSerialMSG(const QByteArray &text)
{
    //     qDebug() << "parseSerial text: " << text << "\n";
    
    //     qDebug() << "parseSerial msg: " << (uint8_t)text[0] << ":" <<
    //         (uint8_t)text[1] << ":" << (uint8_t)text[2] << ":" << (uint8_t)text[3];
    
//     qDebug() << "text[0]: " << (int)text[0];
//     qDebug() << "text[1]: " << (int)text[1];
//     qDebug() << "text[2]: " << (int)text[2];
//     qDebug() << "text[3]: " << (int)text[3];
//     qDebug() << "len: " << text.length();
    
    sendSerial2Tcp(static_cast<ESerialMessage>(text[0]), text.mid(1));
}

void CControl::parseClientTcp(QDataStream &stream)
{
    QString msg;
    stream >> msg;
    qDebug() << "Received client msg:" << msg;

    if (msg == "command")
    {
        QString cmd;
        stream >> cmd;
        serialPort->sendCommand(cmd);
    }
}

void CControl::enableRP6Slave()
{
    serialPort->sendCommand("set power 1");
    serialPort->sendCommand("set slave 1");
}
