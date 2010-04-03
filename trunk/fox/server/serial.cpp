#include <QtCore>

#include "qextserialport.h"
#include "serial.h"

CSerialPort::CSerialPort(QObject *parent) : QObject(parent)
{
    serialPort = new QextSerialPort("/dev/ttyUSB0", QextSerialPort::EventDriven);
    serialPort->setParent(this);
    serialPort->setBaudRate(BAUD38400);
    serialPort->setFlowControl(FLOW_OFF);
    serialPort->setParity(PAR_NONE);
    serialPort->setDataBits(DATA_8);
    serialPort->setStopBits(STOP_1);
    
    if (serialPort->open(QIODevice::ReadWrite) == true)
    {
        connect(serialPort, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
        //        connect(port, SIGNAL(dsrChanged(bool)), this, SLOT(onDsrChanged(bool)));
        if (!(serialPort->lineStatus() & LS_DSR))
            qDebug() << "warning: device is not turned on";
        qDebug() << "listening for data on" << serialPort->portName();
    }
    else
        qDebug() << "device failed to open:" << serialPort->errorString();
    
    serialPort->setDtr(false);
    serialPort->setRts(false);
}

void CSerialPort::onReadyRead()
{
    QByteArray bytes;
    int a = serialPort->bytesAvailable();
    bytes.resize(a);
    serialPort->read(bytes.data(), bytes.size());

    buffer += bytes;

    int end = buffer.indexOf('\n');
    while (end != -1)
    {
        emit textAvailable(buffer.left(end)); // Emit without newline
        buffer.remove(0, end+1);
        end = buffer.indexOf('\n');
    }
/*    if (buffer.contains('\n'))
    {
        foreach(QByteArray line, buffer.split('\n'))
        {
            if (line.size() && line.contains('\n'))
                emit textAvailable(line);
        }

        int nl = buffer.lastIndexOf('\n');
        if (nl != -1)
            buffer.remove(0, nl+1);
    }*/
}

void CSerialPort::disableRTS()
{
    serialPort->setRts(false);
}

void CSerialPort::resetRP6()
{
    serialPort->setRts(true);
    QTimer::singleShot(100, this, SLOT(disableRTS));
}

void CSerialPort::sendCommand(const QString &command)
{
    serialPort->write((command + "\n").toLatin1());
}
